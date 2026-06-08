#include "comm_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "app_events.h"
#include "app_modes.h"
#include "event_bus.h"
#include "ble_setup.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"

static const char *TAG = "COMM_MANAGER";

/* Mesmo pino documentado em board_config.h como PIN_NUM_PROVISION_RESET.
 * Definido localmente para que o componente "communication" não precise
 * depender do "board_config.h" (que arrasta o stack de display/esp_lcd). */
static constexpr gpio_num_t PIN_NUM_PROVISION_RESET = GPIO_NUM_18;

static comm_manager_config_t s_config = {};
static bool s_initialized = false;
static TaskHandle_t s_task_handle = nullptr;

/* ---- helpers ------------------------------------------------------------ */

static esp_err_t init_nvs(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

/* Botão de reset de provisionamento: mantido pressionado (nível baixo, pull-up
 * interno) durante o boot apaga as credenciais Wi-Fi salvas e força a
 * reabertura do pareamento BLE. */
static bool provision_reset_requested(void) {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << PIN_NUM_PROVISION_RESET);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    vTaskDelay(pdMS_TO_TICKS(20));
    bool pressed = (gpio_get_level(PIN_NUM_PROVISION_RESET) == 0);
    if (pressed) {
        ESP_LOGW(TAG, "Botão de reset de provisionamento pressionado no boot: limpando credenciais Wi-Fi");
    }
    return pressed;
}

/* Liga/desliga o pareamento BLE sob pedido da UI (toque na linha "Wi-Fi" de
 * Ajustes) ou de um comando externo equivalente — eco real via
 * BLE_PROVISION_STATUS, publicado por ble_setup_start/stop. */
static void on_ble_provision_command(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
    event_ble_provision_command_t *evt = (event_ble_provision_command_t *)event_data;
    if (evt->enable) {
        ble_setup_start();
    } else {
        ble_setup_stop();
    }
}

/* ---- task de orquestração ----------------------------------------------- */

static void comm_manager_task(void *arg) {
    if (provision_reset_requested()) {
        wifi_manager_clear_credentials();
    }

    if (!wifi_manager_has_credentials()) {
        if (!s_config.enable_ble) {
            ESP_LOGE(TAG, "Sem credenciais Wi-Fi salvas e provisionamento BLE desabilitado: nada a fazer");
            s_task_handle = nullptr;
            vTaskDelete(NULL);
            return;
        }

        ESP_LOGI(TAG, "Nenhuma credencial Wi-Fi salva: iniciando provisionamento BLE");
        ble_setup_start();

        while (!wifi_manager_has_credentials()) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        ESP_LOGI(TAG, "Credenciais Wi-Fi recebidas via BLE e persistidas na NVS");
    }

    ESP_LOGI(TAG, "Conectando ao Wi-Fi...");
    wifi_manager_start();

    while (!wifi_manager_is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGI(TAG, "Wi-Fi conectado");

    if (s_config.enable_mqtt) {
        ESP_LOGI(TAG, "Iniciando cliente MQTT...");
        mqtt_client_start();
    }

    ESP_LOGI(TAG, "Sequência de conectividade concluída");
    s_task_handle = nullptr;
    vTaskDelete(NULL);
}

/* ---- API pública --------------------------------------------------------- */

esp_err_t comm_manager_init(const comm_manager_config_t *config) {
    if (s_initialized) {
        return ESP_OK;
    }
    if (config != nullptr) {
        s_config = *config;
    }

    esp_err_t err = init_nvs();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar NVS: %s", esp_err_to_name(err));
        return err;
    }

    err = event_bus_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar event_bus: %s", esp_err_to_name(err));
        return err;
    }

    app_modes_init(APP_MODE_MANUAL);

    wifi_manager_config_t wifi_cfg = {};
    wifi_cfg.start_ap_fallback = (s_config.wifi_ap_fallback_ssid != nullptr);
    wifi_cfg.ap_ssid = s_config.wifi_ap_fallback_ssid;
    wifi_cfg.ap_pass = s_config.wifi_ap_fallback_pass;
    err = wifi_manager_init(&wifi_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar wifi_manager: %s", esp_err_to_name(err));
        return err;
    }

    if (s_config.enable_ble) {
        err = ble_setup_init();
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Falha ao inicializar BLE provisioning: %s", esp_err_to_name(err));
        }
        event_bus_register(SMART_SWITCH_EVENT_BLE_PROVISION_COMMAND, &on_ble_provision_command, nullptr);
    }

    if (s_config.enable_mqtt) {
        if (s_config.mqtt_broker_uri == nullptr) {
            ESP_LOGW(TAG, "enable_mqtt=true mas mqtt_broker_uri não foi informado; MQTT permanecerá desligado");
        } else {
            mqtt_client_config_t mqtt_cfg = {};
            mqtt_cfg.broker_uri = s_config.mqtt_broker_uri;
            mqtt_cfg.mqtt_username = s_config.mqtt_username;
            mqtt_cfg.mqtt_password = s_config.mqtt_password;
            mqtt_cfg.state_heartbeat_ms = 30000;
            mqtt_cfg.telemetry_interval_ms = 7000;
            err = mqtt_client_init(&mqtt_cfg);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Falha ao inicializar mqtt_client: %s", esp_err_to_name(err));
                return err;
            }
        }
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Comm manager inicializado (ble=%d mqtt=%d http=%d)",
             (int)s_config.enable_ble, (int)s_config.enable_mqtt, (int)s_config.enable_http);
    return ESP_OK;
}

esp_err_t comm_manager_start(void) {
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_task_handle != nullptr) {
        return ESP_OK;
    }

    BaseType_t ok = xTaskCreate(comm_manager_task, "comm_manager", 6144, nullptr, 5, &s_task_handle);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar a task de orquestração de comunicação");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

esp_err_t comm_manager_stop(void) {
    if (s_config.enable_mqtt) {
        mqtt_client_stop();
    }
    wifi_manager_stop();
    if (s_config.enable_ble) {
        ble_setup_stop();
    }
    if (s_task_handle != nullptr) {
        vTaskDelete(s_task_handle);
        s_task_handle = nullptr;
    }
    return ESP_OK;
}
