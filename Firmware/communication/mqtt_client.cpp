#include "mqtt_manager.h"

#include <cstdio>
#include <cstring>

#include "freertos/FreeRTOS.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"

/* Header do componente esp-mqtt (esp-idf). Nosso header público foi nomeado
 * "mqtt_manager.h" propositalmente para não colidir com este "mqtt_client.h"
 * do esp-idf (quote-includes resolvem primeiro para o diretório local). */
#include "mqtt_client.h"

#include "cJSON.h"

#include "app_events.h"
#include "app_modes.h"
#include "event_bus.h"
#include "device_identity.h"
#include "wifi_manager.h"

static const char *TAG = "MQTT_CLIENT";

/* "sp" não tem um sensor de tensão de referência dedicado: publicamos a
 * tensão nominal da rede como valor de "setpoint" de tensão. */
#define MQTT_NOMINAL_MAINS_VOLTAGE_V    220.0f
#define MQTT_LUX_SCALE                  1000.0f

typedef struct {
    bool relay_on;
    bool automatic_mode;
    uint8_t dimmer;
} mqtt_device_state_t;

typedef struct {
    float lux;
    float natural_lux;
    float voltage_rms;
    float current_rms;
    float active_power_w;
} mqtt_device_telemetry_t;

static mqtt_client_config_t s_config = {};
static esp_mqtt_client_handle_t s_client = nullptr;
static bool s_initialized = false;
static bool s_started = false;
static bool s_connected = false;

static char s_device_id[24];
static int32_t s_user_id = 0;

static char s_topic_command[96];
static char s_topic_state[96];
static char s_topic_telemetry[96];

static mqtt_device_state_t s_state = { false, false, 0 };
static mqtt_device_telemetry_t s_telemetry = {};

static esp_timer_handle_t s_state_timer = nullptr;
static esp_timer_handle_t s_telemetry_timer = nullptr;

/* ---- publicação --------------------------------------------------------- */

esp_err_t mqtt_client_publish_state(void) {
    if (s_client == nullptr || !s_connected) {
        return ESP_ERR_INVALID_STATE;
    }

    char payload[160];
    int len = snprintf(payload, sizeof(payload),
        "{\"relay\":%s,\"automatic_mode\":%s,\"dimmer\":%u}",
        s_state.relay_on ? "true" : "false",
        s_state.automatic_mode ? "true" : "false",
        (unsigned)s_state.dimmer);

    int msg_id = esp_mqtt_client_publish(s_client, s_topic_state, payload, len, 1, false);
    if (msg_id < 0) {
        ESP_LOGW(TAG, "Falha ao publicar estado");
        return ESP_FAIL;
    }
    ESP_LOGD(TAG, "Estado publicado: %s", payload);
    return ESP_OK;
}

esp_err_t mqtt_client_publish_telemetry(void) {
    if (s_client == nullptr || !s_connected) {
        return ESP_ERR_INVALID_STATE;
    }

    char payload[224];
    int len = snprintf(payload, sizeof(payload),
        "{\"lux\":%.1f,\"natural_lux\":%.1f,\"sp\":%.1f,\"mv\":%.1f,\"current\":%.2f,\"power\":%.1f}",
        s_telemetry.lux,
        s_telemetry.natural_lux,
        MQTT_NOMINAL_MAINS_VOLTAGE_V,
        s_telemetry.voltage_rms,
        s_telemetry.current_rms,
        s_telemetry.active_power_w);

    int msg_id = esp_mqtt_client_publish(s_client, s_topic_telemetry, payload, len, 0, false);
    if (msg_id < 0) {
        ESP_LOGW(TAG, "Falha ao publicar telemetria");
        return ESP_FAIL;
    }
    ESP_LOGD(TAG, "Telemetria publicada: %s", payload);
    return ESP_OK;
}

static void state_timer_cb(void *arg) {
    mqtt_client_publish_state();
}

static void telemetry_timer_cb(void *arg) {
    mqtt_client_publish_telemetry();
}

/* ---- comandos recebidos do backend -------------------------------------- */

static void handle_command_payload(const char *data, int data_len) {
    char *buf = (char *)malloc(data_len + 1);
    if (buf == nullptr) {
        return;
    }
    memcpy(buf, data, data_len);
    buf[data_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (root == nullptr) {
        ESP_LOGW(TAG, "Comando MQTT com JSON inválido");
        return;
    }

    const cJSON *relay_item    = cJSON_GetObjectItemCaseSensitive(root, "relay");
    const cJSON *mode_item     = cJSON_GetObjectItemCaseSensitive(root, "mode");
    const cJSON *dimmer_item   = cJSON_GetObjectItemCaseSensitive(root, "dimmer");
    const cJSON *setpoint_item = cJSON_GetObjectItemCaseSensitive(root, "setpoint");

    if (cJSON_IsBool(relay_item)) {
        bool relay_on = cJSON_IsTrue(relay_item);
        ESP_LOGI(TAG, "Comando recebido: relay=%s", relay_on ? "true" : "false");
        s_state.relay_on = relay_on;
        event_relay_command_t evt = { .relay_on = relay_on };
        event_bus_post(SMART_SWITCH_EVENT_RELAY_COMMAND, &evt, sizeof(evt), pdMS_TO_TICKS(100));
        mqtt_client_publish_state();
    }

    if (cJSON_IsString(mode_item)) {
        ESP_LOGI(TAG, "Comando recebido: mode=%s", mode_item->valuestring);
        if (strcmp(mode_item->valuestring, "auto") == 0) {
            app_modes_set(APP_MODE_AUTOMATIC);
        } else if (strcmp(mode_item->valuestring, "manual") == 0) {
            app_modes_set(APP_MODE_MANUAL);
        } else {
            ESP_LOGW(TAG, "Valor de 'mode' desconhecido: %s", mode_item->valuestring);
        }
    }

    if (cJSON_IsNumber(dimmer_item)) {
        int value = (int)dimmer_item->valuedouble;
        if (value < 0) value = 0;
        if (value > 100) value = 100;
        ESP_LOGI(TAG, "Comando recebido: dimmer=%d", value);
        s_state.dimmer = (uint8_t)value;
        event_dimmer_command_t evt = { .value = (uint8_t)value };
        event_bus_post(SMART_SWITCH_EVENT_DIMMER_COMMAND, &evt, sizeof(evt), pdMS_TO_TICKS(100));
        mqtt_client_publish_state();
    }

    if (cJSON_IsNumber(setpoint_item)) {
        float setpoint = (float)setpoint_item->valuedouble;
        ESP_LOGI(TAG, "Comando recebido: setpoint=%.1f", setpoint);
        event_setpoint_changed_t evt = { .setpoint = setpoint };
        event_bus_post(SMART_SWITCH_EVENT_SETPOINT_CHANGED, &evt, sizeof(evt), pdMS_TO_TICKS(100));
    }

    cJSON_Delete(root);
}

/* ---- callback do cliente esp-mqtt ---------------------------------------- */

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Conectado ao broker MQTT");
            s_connected = true;
            esp_mqtt_client_subscribe(s_client, s_topic_command, 1);
            /* Ressincroniza o estado conhecido com o backend ao (re)conectar. */
            mqtt_client_publish_state();
            mqtt_client_publish_telemetry();
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Desconectado do broker MQTT");
            s_connected = false;
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "Inscrito em '%s'", s_topic_command);
            break;

        case MQTT_EVENT_DATA:
            if (event->topic_len > 0 && strncmp(event->topic, s_topic_command, event->topic_len) == 0) {
                handle_command_payload(event->data, event->data_len);
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGW(TAG, "Erro no cliente MQTT");
            break;

        default:
            break;
    }
}

/* ---- handlers do event_bus (estado/telemetria locais) --------------------- */

static void on_mode_changed(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
    event_mode_changed_t *evt = (event_mode_changed_t *)event_data;
    s_state.automatic_mode = (evt->mode != APP_MODE_MANUAL);
    mqtt_client_publish_state();
}

static void on_dimmer_update(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
    event_dimmer_update_t *evt = (event_dimmer_update_t *)event_data;
    uint8_t dimmer = (uint8_t)(evt->duty * 100.0f + 0.5f);
    bool relay_on = evt->duty > 0.001f;
    if (dimmer != s_state.dimmer || relay_on != s_state.relay_on) {
        s_state.dimmer = dimmer;
        s_state.relay_on = relay_on;
        mqtt_client_publish_state();
    }
}

static void on_ldr_update(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
    event_ldr_update_t *evt = (event_ldr_update_t *)event_data;
    s_telemetry.lux = evt->normalized * MQTT_LUX_SCALE;
    /* Sem um modelo de correção da contribuição da lâmpada ainda publicado
     * pelo controle, usamos a leitura do LDR também como aproximação do
     * componente "natural" (a refinar quando o controlador publicar essa
     * decomposição via event_bus). */
    s_telemetry.natural_lux = s_telemetry.lux;
}

static void on_power_update(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
    event_power_update_t *evt = (event_power_update_t *)event_data;
    s_telemetry.voltage_rms = evt->voltage_rms;
    s_telemetry.current_rms = evt->current_rms;
    s_telemetry.active_power_w = evt->active_power_w;
}

/* ---- API pública ----------------------------------------------------------- */

esp_err_t mqtt_client_init(const mqtt_client_config_t *config) {
    if (s_initialized) {
        return ESP_OK;
    }
    if (config == nullptr || config->broker_uri == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    s_config = *config;
    if (s_config.state_heartbeat_ms == 0) {
        s_config.state_heartbeat_ms = 30000;
    }
    if (s_config.telemetry_interval_ms == 0) {
        s_config.telemetry_interval_ms = 7000;
    }

    device_identity_get_id(s_device_id, sizeof(s_device_id));

    event_bus_register(SMART_SWITCH_EVENT_MODE_CHANGED, &on_mode_changed, nullptr);
    event_bus_register(SMART_SWITCH_EVENT_DIMMER_UPDATE, &on_dimmer_update, nullptr);
    event_bus_register(SMART_SWITCH_EVENT_LDR_UPDATE, &on_ldr_update, nullptr);
    event_bus_register(SMART_SWITCH_EVENT_POWER_UPDATE, &on_power_update, nullptr);

    const esp_timer_create_args_t state_timer_args = {
        .callback = &state_timer_cb,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "mqtt_state_hb",
    };
    ESP_ERROR_CHECK(esp_timer_create(&state_timer_args, &s_state_timer));

    const esp_timer_create_args_t telemetry_timer_args = {
        .callback = &telemetry_timer_cb,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "mqtt_telemetry",
    };
    ESP_ERROR_CHECK(esp_timer_create(&telemetry_timer_args, &s_telemetry_timer));

    s_initialized = true;
    ESP_LOGI(TAG, "MQTT client inicializado (device_id=%s, broker=%s)", s_device_id, s_config.broker_uri);
    return ESP_OK;
}

esp_err_t mqtt_client_start(void) {
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_started) {
        return ESP_OK;
    }

    if (wifi_manager_load_user_id(&s_user_id) != ESP_OK) {
        ESP_LOGW(TAG, "user_id não encontrado na NVS, usando 0");
        s_user_id = 0;
    }

    snprintf(s_topic_command, sizeof(s_topic_command), "devices/%d/%s/command", (int)s_user_id, s_device_id);
    snprintf(s_topic_state, sizeof(s_topic_state), "devices/%d/%s/state", (int)s_user_id, s_device_id);
    snprintf(s_topic_telemetry, sizeof(s_topic_telemetry), "devices/%d/%s/telemetry", (int)s_user_id, s_device_id);

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = s_config.broker_uri;
    mqtt_cfg.session.keepalive = 60;
    mqtt_cfg.network.reconnect_timeout_ms = 5000;

    if (s_config.mqtt_username != nullptr && s_config.mqtt_username[0] != '\0') {
        mqtt_cfg.credentials.username = s_config.mqtt_username;
        mqtt_cfg.credentials.authentication.password = s_config.mqtt_password;
        ESP_LOGI(TAG, "Autenticação MQTT habilitada (usuário=%s)", s_config.mqtt_username);
    }

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_client == nullptr) {
        ESP_LOGE(TAG, "Falha ao criar cliente MQTT");
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(s_client, MQTT_EVENT_ANY, &mqtt_event_handler, nullptr);

    esp_err_t err = esp_mqtt_client_start(s_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar cliente MQTT: %s", esp_err_to_name(err));
        return err;
    }

    esp_timer_start_periodic(s_state_timer, (uint64_t)s_config.state_heartbeat_ms * 1000ULL);
    esp_timer_start_periodic(s_telemetry_timer, (uint64_t)s_config.telemetry_interval_ms * 1000ULL);

    s_started = true;
    ESP_LOGI(TAG, "MQTT client conectando a '%s' (tópicos base: devices/%d/%s/...)",
             s_config.broker_uri, (int)s_user_id, s_device_id);
    return ESP_OK;
}

esp_err_t mqtt_client_stop(void) {
    if (!s_started) {
        return ESP_OK;
    }

    esp_timer_stop(s_state_timer);
    esp_timer_stop(s_telemetry_timer);

    if (s_client != nullptr) {
        esp_mqtt_client_stop(s_client);
        esp_mqtt_client_destroy(s_client);
        s_client = nullptr;
    }

    s_connected = false;
    s_started = false;
    return ESP_OK;
}

bool mqtt_client_is_connected(void) {
    return s_connected;
}
