#include "ui_manager.h"

#include <cstdint>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "lvgl_port.h"
#include "ui_state.h"
#include "ui_theme.h"
#include "ui_widgets.h"
#include "screens/screen_main.h"
#include "screens/screen_auto.h"
#include "screens/screen_settings.h"

#include "app_events.h"
#include "app_modes.h"
#include "event_bus.h"

static const char *TAG = "UI_MANAGER";

// Passos dos ajustes feitos por toque — não espelham nada de fora porque
// nascem na UI; o sistema só vê o resultado (comando/SETPOINT publicado).
static constexpr int   UI_BRIGHTNESS_STEP_PCT = 5;
static constexpr float UI_TARGET_STEP_LUX     = 50.0f;
static constexpr float UI_TARGET_MIN_LUX      = 0.0f;
static constexpr float UI_TARGET_MAX_LUX      = UI_AMBIENT_LUX_SCALE;

static screen_main_t      s_main;
static screen_auto_t      s_auto;
static screen_settings_t  s_settings;
static ui_runtime_state_t s_state = {};
static bool s_initialized = false;

/* ============================================================================
 * Helpers internos
 * ========================================================================== */

// Repassa o estado atual às 3 telas de uma vez. Quem chama precisa já estar de
// posse do lock do LVGL: toques disparam de dentro de lv_timer_handler() (que
// já trava por fora) e os handlers do event_bus, mais abaixo, travam na mão
// antes de chamar isto — porque correm na task dedicada do event_bus.
static void refresh_all_screens(void) {
    screen_main_update(&s_main, &s_state);
    screen_auto_update(&s_auto, &s_state);
    screen_settings_update(&s_settings, &s_state);
}

/* ============================================================================
 * Toques — disparados de dentro de lv_timer_handler(), já sob lvgl_port_lock
 * ========================================================================== */

// Navegação entre telas: user_data carrega o destino (lv_obj_t*). Reaproveitado
// pelos 4 caminhos (cartão/barra de status -> Auto/Ajustes; voltar -> Principal).
static void on_navigate(lv_event_t *e) {
    lv_obj_t *target = (lv_obj_t *)lv_event_get_user_data(e);
    lv_screen_load(target);
}

// +/- de intensidade (tela Principal): user_data carrega a direção (-1/+1).
// Só publica o comando — quem confirma e redesenha é o eco via DIMMER_UPDATE,
// o mesmo caminho que um cliente MQTT/BLE externo percorreria.
static void on_brightness_step(lv_event_t *e) {
    int dir = (int)(intptr_t)lv_event_get_user_data(e);
    int new_pct = LV_CLAMP(0, (int)s_state.brightness_pct + dir * UI_BRIGHTNESS_STEP_PCT, 100);

    event_dimmer_command_t cmd = { .value = (uint8_t)new_pct };
    event_bus_post(SMART_SWITCH_EVENT_DIMMER_COMMAND, &cmd, sizeof(cmd), pdMS_TO_TICKS(100));
}

// +/- do alvo de iluminação (tela Auto): user_data carrega a direção (-1/+1).
// Aqui a UI é dona do valor — atualiza e redesenha na hora — e só então
// publica SETPOINT_CHANGED para propagar a decisão ao resto do sistema.
static void on_target_step(lv_event_t *e) {
    int dir = (int)(intptr_t)lv_event_get_user_data(e);
    s_state.target_lux = LV_CLAMP(UI_TARGET_MIN_LUX, s_state.target_lux + dir * UI_TARGET_STEP_LUX, UI_TARGET_MAX_LUX);
    refresh_all_screens();

    event_setpoint_changed_t evt = { .setpoint = s_state.target_lux };
    event_bus_post(SMART_SWITCH_EVENT_SETPOINT_CHANGED, &evt, sizeof(evt), pdMS_TO_TICKS(100));
}

// Botão de força (tela Principal): publica o inverso do estado atual; o eco
// via DIMMER_UPDATE confirma e redesenha (mesmo caminho do passo de intensidade).
static void on_power_toggle(lv_event_t *e) {
    (void)e;
    event_relay_command_t cmd = { .relay_on = !s_state.relay_on };
    event_bus_post(SMART_SWITCH_EVENT_RELAY_COMMAND, &cmd, sizeof(cmd), pdMS_TO_TICKS(100));
}

// Interruptor mestre (tela Auto): app_modes_set já publica MODE_CHANGED quando
// o modo muda de fato — on_mode_changed (abaixo) fecha o ciclo a partir do
// estado real. Por isso não tocamos s_state.mode nem redesenhamos aqui.
static void on_mode_toggle(lv_event_t *e) {
    bool checked = lv_obj_has_state((lv_obj_t *)lv_event_get_target(e), LV_STATE_CHECKED);
    app_modes_set(checked ? APP_MODE_AUTOMATIC : APP_MODE_MANUAL);
}

// Slider de sensibilidade (tela Auto): ajuste hoje é só local da UI — o
// sistema ainda não tem um conceito de "sensibilidade automática" para receber.
static void on_sensitivity_changed(lv_event_t *e) {
    lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
    s_state.sensitivity_pct = (uint8_t)lv_slider_get_value(slider);
}

// Linha "Wi-Fi" (tela Ajustes): alterna o pareamento BLE — liga para abrir
// a janela de provisionamento (enviar novo SSID/senha/user_id) e liga de
// novo para cancelar. O eco real chega via BLE_PROVISION_STATUS (on_ble_
// provision_status, abaixo): só então o rótulo muda para "pareando…".
static void on_wifi_row_tap(lv_event_t *e) {
    (void)e;
    event_ble_provision_command_t cmd = { .enable = !s_state.ble_pairing_active };
    event_bus_post(SMART_SWITCH_EVENT_BLE_PROVISION_COMMAND, &cmd, sizeof(cmd), pdMS_TO_TICKS(100));
}

// Segmento PT/EN (tela Ajustes): user_data carrega o idioma escolhido;
// redesenhar tudo de uma vez retexta as 3 telas no idioma novo.
static void on_language_selected(lv_event_t *e) {
    ui_lang_t lang = (ui_lang_t)(intptr_t)lv_event_get_user_data(e);
    if (s_state.lang == lang) {
        return;
    }
    s_state.lang = lang;
    refresh_all_screens();
}

/* ============================================================================
 * event_bus — corre na task dedicada do event_bus: precisa travar o LVGL
 * antes de tocar em qualquer lv_obj_* (lvgl_port_lock é recursivo, então
 * continuaria seguro mesmo se um dia algum destes rodasse sob a task do LVGL).
 * ========================================================================== */

static void on_ldr_update(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
    event_ldr_update_t *evt = (event_ldr_update_t *)event_data;
    if (!lvgl_port_lock(100)) {
        return;
    }
    // Mesma aproximação de mqtt_client.cpp (MQTT_LUX_SCALE/UI_AMBIENT_LUX_SCALE):
    // sem um modelo de correção da contribuição da lâmpada, a leitura do LDR
    // também serve como "natural_lux" — refinar quando o controlador publicar
    // essa decomposição via event_bus.
    s_state.natural_lux = evt->normalized * UI_AMBIENT_LUX_SCALE;
    refresh_all_screens();
    lvgl_port_unlock();
}

static void on_dimmer_update(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
    event_dimmer_update_t *evt = (event_dimmer_update_t *)event_data;
    if (!lvgl_port_lock(100)) {
        return;
    }
    s_state.brightness_pct = (uint8_t)(evt->duty * 100.0f + 0.5f);
    s_state.relay_on = evt->duty > 0.001f;
    refresh_all_screens();
    lvgl_port_unlock();
}

static void on_mode_changed(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
    event_mode_changed_t *evt = (event_mode_changed_t *)event_data;
    if (!lvgl_port_lock(100)) {
        return;
    }
    s_state.mode = evt->mode;
    refresh_all_screens();
    lvgl_port_unlock();
}

static void on_setpoint_changed(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
    event_setpoint_changed_t *evt = (event_setpoint_changed_t *)event_data;
    if (!lvgl_port_lock(100)) {
        return;
    }
    s_state.target_lux = evt->setpoint;
    refresh_all_screens();
    lvgl_port_unlock();
}

static void on_ble_provision_status(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
    event_ble_provision_status_t *evt = (event_ble_provision_status_t *)event_data;
    if (!lvgl_port_lock(100)) {
        return;
    }
    s_state.ble_pairing_active = evt->active;
    refresh_all_screens();
    lvgl_port_unlock();
}

static void on_net_status(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
    event_net_status_t *evt = (event_net_status_t *)event_data;
    if (!lvgl_port_lock(100)) {
        return;
    }
    s_state.wifi_connected = evt->wifi_connected;
    strncpy(s_state.ip_addr, evt->ip_addr, sizeof(s_state.ip_addr) - 1);
    refresh_all_screens();
    lvgl_port_unlock();
}

/* ============================================================================
 * API pública
 * ========================================================================== */

esp_err_t ui_manager_init(void) {
    if (s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // Estado inicial: o que já é "verdade" agora (modo corrente do sistema),
    // mais valores plausíveis para o que só existe na UI até o 1º evento
    // chegar (alvo/sensibilidade/brilho da tela não têm fonte externa).
    s_state.mode                  = app_modes_get();
    s_state.target_lux            = 500.0f;
    s_state.sensitivity_pct       = 50;
    s_state.screen_brightness_pct = 100;
    s_state.lang                  = UI_LANG_PT;

    screen_main_create(&s_main);
    screen_auto_create(&s_auto);
    screen_settings_create(&s_settings);
    refresh_all_screens();

    /* ---- navegação entre as 3 telas --------------------------------------- */
    lv_obj_add_event_cb(s_main.room_card,           on_navigate, LV_EVENT_CLICKED, s_auto.root);
    lv_obj_add_event_cb(s_main.status_bar,          on_navigate, LV_EVENT_CLICKED, s_settings.root);
    lv_obj_add_event_cb(s_auto.header.back_btn,     on_navigate, LV_EVENT_CLICKED, s_main.root);
    lv_obj_add_event_cb(s_settings.header.back_btn, on_navigate, LV_EVENT_CLICKED, s_main.root);

    /* ---- tela Principal: intensidade (stepper) e força (power_btn) -------- */
    lv_obj_add_event_cb(s_main.stepper.btn_minus, on_brightness_step, LV_EVENT_CLICKED, (void *)(intptr_t)(-1));
    lv_obj_add_event_cb(s_main.stepper.btn_plus,  on_brightness_step, LV_EVENT_CLICKED, (void *)(intptr_t)(+1));
    lv_obj_add_event_cb(s_main.power_btn.root,    on_power_toggle,    LV_EVENT_CLICKED, nullptr);

    /* ---- tela Modo Automático: interruptor, alvo e sensibilidade ---------- */
    lv_obj_add_event_cb(s_auto.auto_toggle,        on_mode_toggle,         LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(s_auto.target_minus_btn,   on_target_step,         LV_EVENT_CLICKED,       (void *)(intptr_t)(-1));
    lv_obj_add_event_cb(s_auto.target_plus_btn,    on_target_step,         LV_EVENT_CLICKED,       (void *)(intptr_t)(+1));
    lv_obj_add_event_cb(s_auto.sensitivity_slider, on_sensitivity_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    /* ---- tela Ajustes: idioma e Wi-Fi (-> pareamento BLE) -----------------
     * calib_row fica de fora por ora: ainda não existe rotina de calibração.
     * Continua visível (espelhando o wireframe), só não reage ao toque ainda. */
    lv_obj_add_event_cb(s_settings.language_seg.btn_a, on_language_selected, LV_EVENT_CLICKED, (void *)(intptr_t)UI_LANG_PT);
    lv_obj_add_event_cb(s_settings.language_seg.btn_b, on_language_selected, LV_EVENT_CLICKED, (void *)(intptr_t)UI_LANG_EN);
    lv_obj_add_event_cb(s_settings.wifi_row, on_wifi_row_tap, LV_EVENT_CLICKED, nullptr);

    /* ---- assina o event_bus: mantém s_state em sincronia com o sistema ---- */
    event_bus_register(SMART_SWITCH_EVENT_LDR_UPDATE,           &on_ldr_update,           nullptr);
    event_bus_register(SMART_SWITCH_EVENT_DIMMER_UPDATE,        &on_dimmer_update,        nullptr);
    event_bus_register(SMART_SWITCH_EVENT_MODE_CHANGED,         &on_mode_changed,         nullptr);
    event_bus_register(SMART_SWITCH_EVENT_SETPOINT_CHANGED,     &on_setpoint_changed,     nullptr);
    event_bus_register(SMART_SWITCH_EVENT_NET_STATUS,           &on_net_status,           nullptr);
    event_bus_register(SMART_SWITCH_EVENT_BLE_PROVISION_STATUS, &on_ble_provision_status, nullptr);

    s_initialized = true;
    ESP_LOGI(TAG, "telas montadas, toques ligados e event_bus assinado");
    return ESP_OK;
}

esp_err_t ui_manager_start(void) {
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    lv_screen_load(s_main.root);
    return lvgl_port_start();
}
