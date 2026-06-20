#include "wifi_manager.h"

#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "app_events.h"
#include "event_bus.h"
#include "ble_setup.h"

static const char *TAG = "WIFI_MANAGER";

#define WIFI_CFG_NVS_NAMESPACE  "wifi_cfg"
#define WIFI_CFG_KEY_SSID       "ssid"
#define WIFI_CFG_KEY_PASS       "pass"
#define WIFI_CFG_KEY_UID        "uid"

#define WIFI_MAX_RETRY_BEFORE_BLE_FALLBACK    6
#define WIFI_BACKOFF_BASE_MS        1000
#define WIFI_BACKOFF_MAX_MS         60000

static wifi_manager_config_t s_config = {};
static bool s_initialized = false;
static bool s_started = false;

static esp_netif_t *s_sta_netif = nullptr;

static EventGroupHandle_t s_event_group = nullptr;
static const int WIFI_CONNECTED_BIT = BIT0;

static wifi_manager_state_t s_state = WIFI_MANAGER_STATE_IDLE;
static uint32_t s_retry_count = 0;
static esp_timer_handle_t s_reconnect_timer = nullptr;

static int8_t s_last_rssi = 0;
static char s_last_ip[16] = "0.0.0.0";

/* ---- estado interno auxiliar ---------------------------------------- */

static void publish_net_status(bool connected) {
    event_net_status_t evt = {};
    evt.wifi_connected = connected;
    evt.mqtt_connected = false; /* o mqtt_client publica seu próprio status; aqui refletimos só o link wifi */
    evt.rssi = wifi_manager_get_rssi();
    strncpy(evt.ip_addr, s_last_ip, sizeof(evt.ip_addr) - 1);
    event_bus_post(SMART_SWITCH_EVENT_NET_STATUS, &evt, sizeof(evt), pdMS_TO_TICKS(100));
}

static uint32_t backoff_delay_ms(uint32_t retry) {
    uint64_t delay = (uint64_t)WIFI_BACKOFF_BASE_MS << (retry > 6 ? 6 : retry);
    if (delay > WIFI_BACKOFF_MAX_MS) {
        delay = WIFI_BACKOFF_MAX_MS;
    }
    return (uint32_t)delay;
}

static void reconnect_timer_cb(void *arg) {
    ESP_LOGI(TAG, "Tentando reconectar ao Wi-Fi (tentativa %u)", (unsigned)s_retry_count + 1);
    esp_wifi_connect();
}

static void start_ble_fallback(void) {
    if (!s_config.enable_ble_fallback || s_state == WIFI_MANAGER_STATE_BLE_FALLBACK) {
        return;
    }

    ESP_LOGW(TAG, "Falhas consecutivas de reconexão: reabrindo provisionamento BLE (STA continua tentando em segundo plano)");
    s_state = WIFI_MANAGER_STATE_BLE_FALLBACK;

    publish_net_status(false);
    ble_setup_start();
}

/* ---- handlers de eventos do esp_wifi / esp_netif --------------------- */

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                s_state = WIFI_MANAGER_STATE_CONNECTING;
                esp_wifi_connect();
                break;

            case WIFI_EVENT_STA_DISCONNECTED: {
                bool was_connected = (s_state == WIFI_MANAGER_STATE_CONNECTED);
                xEventGroupClearBits(s_event_group, WIFI_CONNECTED_BIT);
                strncpy(s_last_ip, "0.0.0.0", sizeof(s_last_ip));

                if (was_connected) {
                    ESP_LOGW(TAG, "Conexão Wi-Fi perdida");
                    publish_net_status(false);
                }

                if (s_state == WIFI_MANAGER_STATE_BLE_FALLBACK) {
                    /* Já estamos no modo de provisionamento BLE; continua tentando o STA em segundo plano. */
                    s_retry_count++;
                    uint32_t delay_ms = backoff_delay_ms(s_retry_count);
                    esp_timer_stop(s_reconnect_timer);
                    esp_timer_start_once(s_reconnect_timer, (uint64_t)delay_ms * 1000ULL);
                    break;
                }

                s_state = WIFI_MANAGER_STATE_CONNECTING;
                s_retry_count++;

                if (s_retry_count >= WIFI_MAX_RETRY_BEFORE_BLE_FALLBACK) {
                    start_ble_fallback();
                    break;
                }

                uint32_t delay_ms = backoff_delay_ms(s_retry_count);
                ESP_LOGI(TAG, "Reconectando em %u ms (tentativa %u/%u)", (unsigned)delay_ms, (unsigned)s_retry_count, (unsigned)WIFI_MAX_RETRY_BEFORE_BLE_FALLBACK);
                esp_timer_stop(s_reconnect_timer);
                esp_timer_start_once(s_reconnect_timer, (uint64_t)delay_ms * 1000ULL);
                break;
            }

            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            snprintf(s_last_ip, sizeof(s_last_ip), IPSTR, IP2STR(&event->ip_info.ip));

            ESP_LOGI(TAG, "Conectado! IP: %s", s_last_ip);
            s_state = WIFI_MANAGER_STATE_CONNECTED;
            s_retry_count = 0;
            esp_timer_stop(s_reconnect_timer);

            xEventGroupSetBits(s_event_group, WIFI_CONNECTED_BIT);
            publish_net_status(true);

            /* Não encerramos o BLE aqui de forma incondicional: se a conexão
             * foi recuperada por uma tentativa em segundo plano enquanto o
             * provisionamento estava aberto (WIFI_MANAGER_STATE_BLE_FALLBACK),
             * ble_setup pode estar no meio de notificar o app sobre um
             * resultado de provisionamento — quem decide encerrar o rádio BLE
             * é sempre o próprio ble_setup (ble_setup_stop), depois de
             * concluir a notificação via GATT. */
        }
    }
}

/* ---- API pública ------------------------------------------------------ */

esp_err_t wifi_manager_init(const wifi_manager_config_t *config) {
    if (s_initialized) {
        return ESP_OK;
    }

    if (config != nullptr) {
        s_config = *config;
    }

    s_event_group = xEventGroupCreate();
    if (s_event_group == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    s_sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&init_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init falhou: %s", esp_err_to_name(err));
        return err;
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr, nullptr));

    const esp_timer_create_args_t timer_args = {
        .callback = &reconnect_timer_cb,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "wifi_reconnect",
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &s_reconnect_timer));

    s_initialized = true;
    ESP_LOGI(TAG, "Wi-Fi manager inicializado");
    return ESP_OK;
}

esp_err_t wifi_manager_start(void) {
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_started) {
        return ESP_OK;
    }

    char ssid[33] = {0};
    char pass[65] = {0};

    nvs_handle_t handle;
    esp_err_t err = nvs_open(WIFI_CFG_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Sem credenciais salvas na NVS");
        return ESP_ERR_NOT_FOUND;
    }

    size_t ssid_len = sizeof(ssid);
    size_t pass_len = sizeof(pass);
    esp_err_t err_ssid = nvs_get_str(handle, WIFI_CFG_KEY_SSID, ssid, &ssid_len);
    esp_err_t err_pass = nvs_get_str(handle, WIFI_CFG_KEY_PASS, pass, &pass_len);
    nvs_close(handle);

    if (err_ssid != ESP_OK || err_pass != ESP_OK || strlen(ssid) == 0) {
        ESP_LOGW(TAG, "Credenciais incompletas na NVS");
        return ESP_ERR_NOT_FOUND;
    }

    wifi_config_t wifi_cfg = {};
    strncpy((char *)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char *)wifi_cfg.sta.password, pass, sizeof(wifi_cfg.sta.password) - 1);
    wifi_cfg.sta.threshold.authmode = strlen(pass) > 0 ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;

    s_retry_count = 0;
    s_state = WIFI_MANAGER_STATE_CONNECTING;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    err = esp_wifi_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_start falhou: %s", esp_err_to_name(err));
        return err;
    }

    s_started = true;
    ESP_LOGI(TAG, "Conectando ao SSID '%s'...", ssid);
    return ESP_OK;
}

esp_err_t wifi_manager_stop(void) {
    if (!s_started) {
        return ESP_OK;
    }
    esp_timer_stop(s_reconnect_timer);
    esp_wifi_disconnect();
    esp_wifi_stop();
    s_started = false;
    s_state = WIFI_MANAGER_STATE_IDLE;
    xEventGroupClearBits(s_event_group, WIFI_CONNECTED_BIT);
    return ESP_OK;
}

bool wifi_manager_is_connected(void) {
    if (s_event_group == nullptr) {
        return false;
    }
    return (xEventGroupGetBits(s_event_group) & WIFI_CONNECTED_BIT) != 0;
}

wifi_manager_state_t wifi_manager_get_state(void) {
    return s_state;
}

int8_t wifi_manager_get_rssi(void) {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        s_last_rssi = ap_info.rssi;
    }
    return s_last_rssi;
}

esp_err_t wifi_manager_get_ip(char *out_ip, size_t out_len) {
    if (out_ip == nullptr || out_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    strncpy(out_ip, s_last_ip, out_len - 1);
    out_ip[out_len - 1] = '\0';
    return ESP_OK;
}

bool wifi_manager_has_credentials(void) {
    nvs_handle_t handle;
    if (nvs_open(WIFI_CFG_NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        return false;
    }

    char ssid[33] = {0};
    size_t ssid_len = sizeof(ssid);
    esp_err_t err = nvs_get_str(handle, WIFI_CFG_KEY_SSID, ssid, &ssid_len);
    nvs_close(handle);

    return (err == ESP_OK && strlen(ssid) > 0);
}

esp_err_t wifi_manager_save_credentials(const char *ssid, const char *password, int32_t user_id) {
    if (ssid == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(WIFI_CFG_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_str(handle, WIFI_CFG_KEY_SSID, ssid);
    if (err == ESP_OK) {
        err = nvs_set_str(handle, WIFI_CFG_KEY_PASS, password ? password : "");
    }
    if (err == ESP_OK) {
        err = nvs_set_i32(handle, WIFI_CFG_KEY_UID, user_id);
    }
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    nvs_close(handle);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Credenciais Wi-Fi salvas na NVS (SSID '%s', user_id=%d)", ssid, (int)user_id);
    } else {
        ESP_LOGE(TAG, "Falha ao salvar credenciais na NVS: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t wifi_manager_clear_credentials(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(WIFI_CFG_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return err;
    }
    nvs_erase_all(handle);
    err = nvs_commit(handle);
    nvs_close(handle);

    ESP_LOGW(TAG, "Credenciais Wi-Fi removidas da NVS");
    return err;
}

esp_err_t wifi_manager_load_user_id(int32_t *out_user_id) {
    if (out_user_id == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(WIFI_CFG_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_get_i32(handle, WIFI_CFG_KEY_UID, out_user_id);
    nvs_close(handle);
    return err;
}

esp_err_t wifi_manager_connect_with(const char *ssid, const char *password, uint32_t timeout_ms) {
    if (!s_initialized || ssid == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Testando conexão com SSID '%s'...", ssid);

    wifi_config_t wifi_cfg = {};
    strncpy((char *)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char *)wifi_cfg.sta.password, password ? password : "", sizeof(wifi_cfg.sta.password) - 1);
    wifi_cfg.sta.threshold.authmode = (password && strlen(password) > 0) ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;

    xEventGroupClearBits(s_event_group, WIFI_CONNECTED_BIT);
    s_retry_count = 0;
    s_state = WIFI_MANAGER_STATE_CONNECTING;

    esp_wifi_disconnect();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));

    if (!s_started) {
        esp_err_t err = esp_wifi_start();
        if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
            return err;
        }
        s_started = true;
    } else {
        esp_wifi_connect();
    }

    EventBits_t bits = xEventGroupWaitBits(s_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(timeout_ms));
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Conexão de teste bem-sucedida (IP %s)", s_last_ip);
        return ESP_OK;
    }

    ESP_LOGW(TAG, "Conexão de teste falhou/timeout para SSID '%s'", ssid);
    return ESP_ERR_TIMEOUT;
}
