#include "ble_setup.h"

#include <cstring>
#include <cstdlib>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"
#include "esp_bt_main.h"
#include "esp_log.h"

#include "cJSON.h"

#include "device_identity.h"
#include "wifi_manager.h"

static const char *TAG = "BLE_SETUP";

#define BLE_PROV_APP_ID             0x40
#define BLE_PROV_SVC_INST_ID        0
#define BLE_PROV_CHAR_VAL_LEN_MAX   256
#define BLE_PROV_WIFI_TEST_TIMEOUT_MS  20000

/* UUIDs 128-bit customizados (representação em little-endian / "byte order do ar")
 * Service:  7a0a0001-cd11-4ab5-9c7c-1122d3e4f5a6
 * Cred:     7a0a0002-cd11-4ab5-9c7c-1122d3e4f5a6
 * Status:   7a0a0003-cd11-4ab5-9c7c-1122d3e4f5a6 */
static uint8_t s_service_uuid[ESP_UUID_LEN_128] = {
    0xa6, 0xf5, 0xe4, 0xd3, 0x22, 0x11, 0x7c, 0x9c, 0xb5, 0x4a, 0x11, 0xcd, 0x01, 0x00, 0x0a, 0x7a,
};
static uint8_t s_char_cred_uuid[ESP_UUID_LEN_128] = {
    0xa6, 0xf5, 0xe4, 0xd3, 0x22, 0x11, 0x7c, 0x9c, 0xb5, 0x4a, 0x11, 0xcd, 0x02, 0x00, 0x0a, 0x7a,
};
static uint8_t s_char_status_uuid[ESP_UUID_LEN_128] = {
    0xa6, 0xf5, 0xe4, 0xd3, 0x22, 0x11, 0x7c, 0x9c, 0xb5, 0x4a, 0x11, 0xcd, 0x03, 0x00, 0x0a, 0x7a,
};

enum {
    IDX_SVC = 0,
    IDX_CHAR_CRED_DECL,
    IDX_CHAR_CRED_VAL,
    IDX_CHAR_STATUS_DECL,
    IDX_CHAR_STATUS_VAL,
    IDX_CHAR_STATUS_CFG,
    BLE_PROV_IDX_NB,
};

static const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t  char_prop_write               = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t  char_prop_read_notify         = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t  ccc_default[2]                = {0x00, 0x00};
static const char     status_idle_json[]            = "{\"status\":\"idle\"}";

#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))

static const esp_gatts_attr_db_t s_gatt_db[BLE_PROV_IDX_NB] = {
    [IDX_SVC] =
        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
          sizeof(s_service_uuid), sizeof(s_service_uuid), s_service_uuid}},

    [IDX_CHAR_CRED_DECL] =
        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
          CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},

    [IDX_CHAR_CRED_VAL] =
        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, s_char_cred_uuid, ESP_GATT_PERM_WRITE,
          BLE_PROV_CHAR_VAL_LEN_MAX, 0, NULL}},

    [IDX_CHAR_STATUS_DECL] =
        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
          CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},

    [IDX_CHAR_STATUS_VAL] =
        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, s_char_status_uuid, ESP_GATT_PERM_READ,
          BLE_PROV_CHAR_VAL_LEN_MAX, sizeof(status_idle_json) - 1, (uint8_t *)status_idle_json}},

    [IDX_CHAR_STATUS_CFG] =
        {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
          sizeof(uint16_t), sizeof(ccc_default), (uint8_t *)ccc_default}},
};

static esp_ble_adv_params_t s_adv_params = {
    .adv_int_min        = 0x40,
    .adv_int_max        = 0x80,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static esp_ble_adv_data_t s_adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = false,
    .min_interval        = 0x20,
    .max_interval        = 0x40,
    .appearance          = 0x00,
    .manufacturer_len    = 0,
    .p_manufacturer_data = NULL,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = 0,
    .p_service_uuid      = NULL,
    .flag                = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static uint16_t s_handle_table[BLE_PROV_IDX_NB];
static esp_gatt_if_t s_gatts_if = ESP_GATT_IF_NONE;
static uint16_t s_conn_id = 0xFFFF;
static bool s_notify_enabled = false;
static bool s_running = false;
static bool s_stack_initialized = false;
static char s_device_name[24];
static char s_device_id[24];

typedef struct {
    char ssid[33];
    char password[65];
    int32_t user_id;
} ble_prov_credentials_t;

/* ---- helpers ----------------------------------------------------------- */

static void update_status_value(const char *status, bool include_device_id) {
    char json[160];
    if (include_device_id) {
        snprintf(json, sizeof(json), "{\"status\":\"%s\",\"device_id\":\"%s\"}", status, s_device_id);
    } else {
        snprintf(json, sizeof(json), "{\"status\":\"%s\"}", status);
    }

    esp_ble_gatts_set_attr_value(s_handle_table[IDX_CHAR_STATUS_VAL], strlen(json), (const uint8_t *)json);

    if (s_notify_enabled && s_gatts_if != ESP_GATT_IF_NONE && s_conn_id != 0xFFFF) {
        esp_ble_gatts_send_indicate(s_gatts_if, s_conn_id, s_handle_table[IDX_CHAR_STATUS_VAL],
                                    strlen(json), (uint8_t *)json, false);
    }

    ESP_LOGI(TAG, "Status de provisionamento: %s", json);
}

static void provisioning_task(void *arg) {
    ble_prov_credentials_t *creds = (ble_prov_credentials_t *)arg;

    update_status_value("connecting", false);

    esp_err_t err = wifi_manager_connect_with(creds->ssid, creds->password, BLE_PROV_WIFI_TEST_TIMEOUT_MS);
    if (err == ESP_OK) {
        wifi_manager_save_credentials(creds->ssid, creds->password, creds->user_id);
        update_status_value("connected", true);

        /* Dá tempo para o app ler/receber a notificação antes de encerrar o BLE. */
        vTaskDelay(pdMS_TO_TICKS(2500));
        ble_setup_stop();
    } else {
        update_status_value("failed", false);
    }

    free(creds);
    vTaskDelete(NULL);
}

static void handle_credentials_write(const uint8_t *value, uint16_t len) {
    if (value == nullptr || len == 0) {
        return;
    }

    char *buf = (char *)malloc(len + 1);
    if (buf == nullptr) {
        return;
    }
    memcpy(buf, value, len);
    buf[len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    free(buf);

    if (root == nullptr) {
        ESP_LOGW(TAG, "Payload de credenciais não é um JSON válido");
        update_status_value("failed", false);
        return;
    }

    const cJSON *ssid_item = cJSON_GetObjectItemCaseSensitive(root, "ssid");
    const cJSON *pass_item = cJSON_GetObjectItemCaseSensitive(root, "password");
    const cJSON *uid_item  = cJSON_GetObjectItemCaseSensitive(root, "user_id");

    if (!cJSON_IsString(ssid_item) || ssid_item->valuestring[0] == '\0') {
        ESP_LOGW(TAG, "JSON de credenciais sem 'ssid' válido");
        cJSON_Delete(root);
        update_status_value("failed", false);
        return;
    }

    ble_prov_credentials_t *creds = (ble_prov_credentials_t *)calloc(1, sizeof(ble_prov_credentials_t));
    if (creds == nullptr) {
        cJSON_Delete(root);
        return;
    }

    strncpy(creds->ssid, ssid_item->valuestring, sizeof(creds->ssid) - 1);
    if (cJSON_IsString(pass_item)) {
        strncpy(creds->password, pass_item->valuestring, sizeof(creds->password) - 1);
    }
    creds->user_id = cJSON_IsNumber(uid_item) ? (int32_t)uid_item->valuedouble : 0;

    cJSON_Delete(root);

    ESP_LOGI(TAG, "Credenciais recebidas via BLE: SSID='%s' user_id=%d", creds->ssid, (int)creds->user_id);

    if (xTaskCreate(provisioning_task, "ble_prov_task", 6144, creds, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar task de provisionamento");
        free(creds);
    }
}

/* ---- callbacks GAP / GATTS --------------------------------------------- */

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(&s_adv_params);
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Falha ao iniciar advertising");
            } else {
                ESP_LOGI(TAG, "Advertising BLE iniciado como '%s'", s_device_name);
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            ESP_LOGI(TAG, "Advertising BLE parado");
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGD(TAG, "Parâmetros de conexão atualizados, status=%d", param->update_conn_params.status);
            break;
        default:
            break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT: {
            s_gatts_if = gatts_if;

            esp_err_t err = esp_ble_gap_set_device_name(s_device_name);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Falha ao definir nome do dispositivo: %s", esp_err_to_name(err));
            }

            err = esp_ble_gap_config_adv_data(&s_adv_data);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Falha ao configurar adv data: %s", esp_err_to_name(err));
            }

            err = esp_ble_gatts_create_attr_tab(s_gatt_db, gatts_if, BLE_PROV_IDX_NB, BLE_PROV_SVC_INST_ID);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Falha ao criar tabela de atributos: %s", esp_err_to_name(err));
            }
            break;
        }

        case ESP_GATTS_CREAT_ATTR_TAB_EVT:
            if (param->add_attr_tab.status != ESP_GATT_OK) {
                ESP_LOGE(TAG, "Criação da tabela de atributos falhou: 0x%x", param->add_attr_tab.status);
            } else if (param->add_attr_tab.num_handle != BLE_PROV_IDX_NB) {
                ESP_LOGE(TAG, "Número de handles inesperado: %d (esperado %d)",
                         param->add_attr_tab.num_handle, BLE_PROV_IDX_NB);
            } else {
                memcpy(s_handle_table, param->add_attr_tab.handles, sizeof(s_handle_table));
                esp_ble_gatts_start_service(s_handle_table[IDX_SVC]);
            }
            break;

        case ESP_GATTS_CONNECT_EVT:
            s_conn_id = param->connect.conn_id;
            ESP_LOGI(TAG, "Cliente BLE conectado, conn_id=%d", s_conn_id);
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(TAG, "Cliente BLE desconectado, motivo=0x%x", param->disconnect.reason);
            s_conn_id = 0xFFFF;
            s_notify_enabled = false;
            if (s_running) {
                esp_ble_gap_start_advertising(&s_adv_params);
            }
            break;

        case ESP_GATTS_WRITE_EVT:
            if (param->write.is_prep) {
                break;
            }
            if (param->write.handle == s_handle_table[IDX_CHAR_CRED_VAL]) {
                handle_credentials_write(param->write.value, param->write.len);
            } else if (param->write.handle == s_handle_table[IDX_CHAR_STATUS_CFG] && param->write.len == 2) {
                uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
                s_notify_enabled = (descr_value == 0x0001);
                ESP_LOGI(TAG, "Notificações de status %s", s_notify_enabled ? "habilitadas" : "desabilitadas");
            }
            if (param->write.need_rsp) {
                esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
            }
            break;

        case ESP_GATTS_MTU_EVT:
            ESP_LOGD(TAG, "MTU negociado: %d", param->mtu.mtu);
            break;

        default:
            break;
    }
}

/* ---- API pública -------------------------------------------------------- */

esp_err_t ble_setup_init(void) {
    if (s_stack_initialized) {
        return ESP_OK;
    }

    device_identity_get_id(s_device_id, sizeof(s_device_id));
    device_identity_get_ble_name(s_device_name, sizeof(s_device_name));

    esp_err_t err = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "esp_bt_controller_mem_release: %s", esp_err_to_name(err));
    }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    err = esp_bt_controller_init(&bt_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_controller_init falhou: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_controller_enable falhou: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_bluedroid_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_bluedroid_init falhou: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_bluedroid_enable();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_bluedroid_enable falhou: %s", esp_err_to_name(err));
        return err;
    }

    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_event_handler));
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(BLE_PROV_APP_ID));
    esp_ble_gatt_set_local_mtu(200);

    s_stack_initialized = true;
    ESP_LOGI(TAG, "BLE provisioning inicializado (device_id=%s, nome='%s')", s_device_id, s_device_name);
    return ESP_OK;
}

esp_err_t ble_setup_start(void) {
    if (!s_stack_initialized) {
        esp_err_t err = ble_setup_init();
        if (err != ESP_OK) {
            return err;
        }
    }

    if (s_running) {
        return ESP_OK;
    }

    s_conn_id = 0xFFFF;
    s_notify_enabled = false;
    update_status_value("idle", false);

    s_running = true;
    ESP_LOGI(TAG, "Iniciando provisionamento BLE como '%s'", s_device_name);
    return ESP_OK;
}

esp_err_t ble_setup_stop(void) {
    if (!s_running && !s_stack_initialized) {
        return ESP_OK;
    }

    s_running = false;
    esp_ble_gap_stop_advertising();

    if (s_gatts_if != ESP_GATT_IF_NONE) {
        esp_ble_gatts_app_unregister(s_gatts_if);
        s_gatts_if = ESP_GATT_IF_NONE;
    }

    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();

    s_stack_initialized = false;
    ESP_LOGI(TAG, "BLE provisioning encerrado, rádio/memória liberados");
    return ESP_OK;
}

bool ble_setup_is_running(void) {
    return s_running;
}
