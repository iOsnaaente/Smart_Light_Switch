#include "ui_manager.h"

#include <cstdint>
#include <cstring>
#include <time.h>

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
#include "../board_config.h"

static const char *TAG = "UI_MANAGER";

static constexpr float UI_TARGET_STEP_LUX = 50.0f;
static constexpr float UI_TARGET_MIN_LUX  = 0.0f;
static constexpr float UI_TARGET_MAX_LUX  = UI_AMBIENT_LUX_SCALE;

/* Altura da barra superior: toques nessa faixa não contam como toque no SP */
static constexpr int32_t TOPBAR_H = 54;

static screen_main_t      s_main;
static screen_auto_t      s_auto;
static screen_settings_t  s_settings;
static ui_runtime_state_t s_state = {};
static bool               s_initialized = false;

/* ── Estado de toque na tela principal ─────────────────────────────────────
 * Só usado para saber se o dedo está pressionado (ver apply_touch_y) — não
 * há mais arrasto/swipe, a posição é sempre lida em ABSOLUTO. */
static bool s_dragging = false;

/* ============================================================================
 * Helpers internos
 * ========================================================================== */

static void refresh_all_screens(void) {
    screen_main_update(&s_main, &s_state);
    screen_auto_update(&s_auto, &s_state);
    screen_settings_update(&s_settings, &s_state);
}

/* ============================================================================
 * [DEBUG] Log de toque
 * ========================================================================== */

static const char *screen_name_of(const lv_obj_t *screen) {
    if (screen == s_main.root)     return "Principal";
    if (screen == s_auto.root)     return "Modo Automático";
    if (screen == s_settings.root) return "Ajustes";
    return "?";
}

static const char *widget_name_of(const lv_obj_t *obj) {
    if (obj == nullptr)                        return "(nenhum)";
    if (obj == s_main.root)                    return "fundo tela Principal";
    if (obj == s_main.cfg_btn)                 return "botão Configurações";
    if (obj == s_auto.root)                    return "fundo tela Auto";
    if (obj == s_auto.header.back_btn)         return "Auto: voltar";
    if (obj == s_auto.auto_toggle)             return "Auto: interruptor";
    if (obj == s_auto.target_minus_btn)        return "Auto: alvo [-]";
    if (obj == s_auto.target_plus_btn)         return "Auto: alvo [+]";
    if (obj == s_auto.sensitivity_slider)      return "Auto: sensibilidade";
    if (obj == s_settings.root)                return "fundo tela Ajustes";
    if (obj == s_settings.header.back_btn)     return "Ajustes: voltar";
    if (obj == s_settings.language_seg.btn_a)  return "Ajustes: idioma PT";
    if (obj == s_settings.language_seg.btn_b)  return "Ajustes: idioma EN";
    if (obj == s_settings.calib_row)           return "Ajustes: calibrar sensor";
    if (obj == s_settings.wifi_row)            return "Ajustes: Wi-Fi (BLE pairing)";
    return "outro widget";
}

static void on_indev_press_debug(lv_event_t *e) {
    lv_obj_t *target = (lv_obj_t *)lv_event_get_param(e);
    lv_obj_t *screen = (target != nullptr) ? lv_obj_get_screen(target) : lv_screen_active();
    ESP_LOGI(TAG, "[DEBUG] tela='%s' widget='%s' (%p)",
             screen_name_of(screen), widget_name_of(target), (void *)target);
}

/* ============================================================================
 * Callbacks de toque — chamados de dentro de lv_timer_handler(), já sob lock
 * ========================================================================== */

/* Navegação simples entre telas — user_data = tela destino */
static void on_navigate(lv_event_t *e) {
    lv_obj_t *target = (lv_obj_t *)lv_event_get_user_data(e);
    lv_screen_load(target);
}

/* ── Toque absoluto na tela principal ──────────────────────────────────────
 * Só toque: sem deslizamento/swipe. O valor do SP vem direto do ADC BRUTO do
 * XPT2046 (lvgl_port_get_raw_touch) — não da coordenada de tela já calibrada
 * pelo LVGL. Limiares HARDCODED no valor bruto (não em TOUCH_RAW_Y_MIN/MAX):
 *   ry <= TOUCH_RAW_Y_FULL_ON  (350) -> SP = 100% (liga no máximo)
 *   ry >= TOUCH_RAW_Y_FULL_OFF (750) -> SP = 0%   (desliga)
 *   entre os dois                    -> interpola linear 100% -> 0%
 * [NOTA] Essa janela (350-750) é mais estreita que o range completo calibrado
 * em board_config.h (TOUCH_RAW_Y_MIN..MAX) — de propósito: o marcador/barra
 * (screen_main_update) segue o toque em tempo real porque recalcula a cada
 * PRESSING, mas a % de brilho resultante NÃO precisa corresponder à posição
 * "justa" da barra na tela (ex.: toque em ry=650 pode resultar em ~10% de
 * brilho mesmo a barra estando visualmente em outra fração da tela) — isso
 * é esperado, não é bug.
 * Mesma correção de eixo (SWAP_XY/INVERT_Y) de touch_read_cb (lvgl_port.cpp)
 * é aplicada aqui para não duplicar a calibração com sentidos divergentes
 * caso board_config.h mude. Eixo X não entra no cálculo.
 * ========================================================================== */
static constexpr int TOUCH_RAW_Y_FULL_ON  = 350; // bruto <= 350 -> 100%
static constexpr int TOUCH_RAW_Y_FULL_OFF = 750; // bruto >= 750 -> 0%

static void apply_touch_y(void) {
    int raw_x = 0, raw_y = 0;
    lvgl_port_get_raw_touch(&raw_x, &raw_y);

    int ry = TOUCH_SWAP_XY ? raw_x : raw_y;
    if (TOUCH_INVERT_Y) {
        ry = (TOUCH_RAW_Y_MIN + TOUCH_RAW_Y_MAX) - ry;
    }

    float new_lux;
    if (ry <= TOUCH_RAW_Y_FULL_ON) {
        new_lux = UI_AMBIENT_LUX_SCALE;
    } else if (ry >= TOUCH_RAW_Y_FULL_OFF) {
        new_lux = 0.0f;
    } else {
        float frac = (float)(ry - TOUCH_RAW_Y_FULL_ON) / (float)(TOUCH_RAW_Y_FULL_OFF - TOUCH_RAW_Y_FULL_ON); // 0..1
        new_lux = (1.0f - frac) * UI_AMBIENT_LUX_SCALE;
    }

    s_state.target_lux = new_lux;
    s_state.relay_on    = (new_lux > 0.0f);
    screen_main_update(&s_main, &s_state);
}

static void on_main_pressed(lv_event_t *e) {
    lv_indev_t *indev = lv_indev_active();
    lv_point_t pt;
    lv_indev_get_point(indev, &pt);

    /* Ignora toques na barra superior (onde fica o cfg_btn visual) — isso é
     * uma zona de LAYOUT (em coordenada de tela já calibrada), diferente do
     * cálculo de SP em si, que usa o bruto. */
    if (pt.y < TOPBAR_H) return;

    s_dragging = true;
    apply_touch_y();
}

static void on_main_pressing(lv_event_t *e) {
    if (!s_dragging) return;
    apply_touch_y();
}

static void on_main_released(lv_event_t *e) {
    if (!s_dragging) return;
    s_dragging = false;

    /* DIMMER_COMMAND é postado aqui no event_bus (não MQTT) — main.cpp's
     * on_dimmer_command consome direto e aplica no TRIAC, então o MV reage
     * na hora, mesmo com Wi-Fi/MQTT fora do ar. Mapeamento é proporcional
     * (SP em lux -> % do dimmer); em modo AUTOMÁTICO o lamp_control_task
     * sobrescreve isso a cada ciclo com base no LDR, então não há conflito.
     * SETPOINT_CHANGED continua sendo postado à parte só para
     * telemetria/sincronismo com app+backend via MQTT. */
    uint8_t duty_pct = (uint8_t)(s_state.target_lux * 100.0f / UI_AMBIENT_LUX_SCALE);
    event_dimmer_command_t dim_cmd = { .value = duty_pct };
    event_bus_post(SMART_SWITCH_EVENT_DIMMER_COMMAND, &dim_cmd, sizeof(dim_cmd), pdMS_TO_TICKS(100));

    event_setpoint_changed_t evt = { .setpoint = s_state.target_lux };
    event_bus_post(SMART_SWITCH_EVENT_SETPOINT_CHANGED, &evt, sizeof(evt), pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "SP confirmado: %.0f lux (MV local: %u%%)", s_state.target_lux, duty_pct);
}

/* ── Tela Modo Automático ─────────────────────────────────────────────────── */

static void on_target_step(lv_event_t *e) {
    int dir = (int)(intptr_t)lv_event_get_user_data(e);
    s_state.target_lux = LV_CLAMP(UI_TARGET_MIN_LUX,
                                   s_state.target_lux + dir * UI_TARGET_STEP_LUX,
                                   UI_TARGET_MAX_LUX);
    refresh_all_screens();
    event_setpoint_changed_t evt = { .setpoint = s_state.target_lux };
    event_bus_post(SMART_SWITCH_EVENT_SETPOINT_CHANGED, &evt, sizeof(evt), pdMS_TO_TICKS(100));
}

static void on_mode_toggle(lv_event_t *e) {
    bool checked = lv_obj_has_state((lv_obj_t *)lv_event_get_target(e), LV_STATE_CHECKED);
    app_modes_set(checked ? APP_MODE_AUTOMATIC : APP_MODE_MANUAL);
}

static void on_sensitivity_changed(lv_event_t *e) {
    lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
    s_state.sensitivity_pct = (uint8_t)lv_slider_get_value(slider);
}

/* ── Tela Ajustes ─────────────────────────────────────────────────────────── */

static void on_wifi_row_tap(lv_event_t *e) {
    (void)e;
    event_ble_provision_command_t cmd = { .enable = !s_state.ble_pairing_active };
    event_bus_post(SMART_SWITCH_EVENT_BLE_PROVISION_COMMAND, &cmd, sizeof(cmd), pdMS_TO_TICKS(100));
}

static void on_language_selected(lv_event_t *e) {
    ui_lang_t lang = (ui_lang_t)(intptr_t)lv_event_get_user_data(e);
    if (s_state.lang == lang) return;
    s_state.lang = lang;
    refresh_all_screens();
}

/* ── Timer do relógio (roda dentro de lv_timer_handler, lock já garantido) ── */

static void clock_tick_cb(lv_timer_t *timer) {
    time_t now = time(nullptr);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    if (tm_info.tm_year + 1900 < 2020) {
        lv_label_set_text(s_main.clock_label, "--:--:--");
    } else {
        lv_label_set_text_fmt(s_main.clock_label, "%02d:%02d:%02d",
                              tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec);
    }
}

/* ============================================================================
 * Handlers do event_bus — correm na task do event_bus, precisam do lock
 * ========================================================================== */

static void on_ldr_update(void *arg, esp_event_base_t base, int32_t id, void *data) {
    event_ldr_update_t *evt = (event_ldr_update_t *)data;
    if (!lvgl_port_lock(100)) return;
    s_state.natural_lux = evt->normalized * UI_AMBIENT_LUX_SCALE;
    refresh_all_screens();
    lvgl_port_unlock();
}

static void on_dimmer_update(void *arg, esp_event_base_t base, int32_t id, void *data) {
    event_dimmer_update_t *evt = (event_dimmer_update_t *)data;
    if (!lvgl_port_lock(100)) return;
    s_state.brightness_pct = (uint8_t)(evt->duty * 100.0f + 0.5f);
    s_state.relay_on = evt->duty > 0.001f;
    refresh_all_screens();
    lvgl_port_unlock();
}

static void on_mode_changed(void *arg, esp_event_base_t base, int32_t id, void *data) {
    event_mode_changed_t *evt = (event_mode_changed_t *)data;
    if (!lvgl_port_lock(100)) return;
    s_state.mode = evt->mode;
    refresh_all_screens();
    lvgl_port_unlock();
}

static void on_setpoint_changed(void *arg, esp_event_base_t base, int32_t id, void *data) {
    event_setpoint_changed_t *evt = (event_setpoint_changed_t *)data;
    if (!lvgl_port_lock(100)) return;
    s_state.target_lux = evt->setpoint;
    refresh_all_screens();
    lvgl_port_unlock();
}

static void on_power_update(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
    event_power_update_t *evt = (event_power_update_t *)event_data;
    if (!lvgl_port_lock(100)) {
        return;
    }
    // Mesma fonte (event_bus POWER_UPDATE) que mqtt_client.cpp usa pra
    // alimentar a telemetria do backend — UI e backend sempre em sincronia.
    s_state.active_power_w = evt->active_power_w;
    refresh_all_screens();
    lvgl_port_unlock();
}

static void on_ble_provision_status(void *arg, esp_event_base_t base, int32_t id, void *data) {
    event_ble_provision_status_t *evt = (event_ble_provision_status_t *)data;
    if (!lvgl_port_lock(100)) return;
    s_state.ble_pairing_active = evt->active;
    refresh_all_screens();
    lvgl_port_unlock();
}

static void on_net_status(void *arg, esp_event_base_t base, int32_t id, void *data) {
    event_net_status_t *evt = (event_net_status_t *)data;
    if (!lvgl_port_lock(100)) return;
    s_state.wifi_connected = evt->wifi_connected;
    strncpy(s_state.ip_addr, evt->ip_addr, sizeof(s_state.ip_addr) - 1);
    refresh_all_screens();
    lvgl_port_unlock();
}

/* ============================================================================
 * API pública
 * ========================================================================== */

esp_err_t ui_manager_init(void) {
    if (s_initialized) return ESP_ERR_INVALID_STATE;

    s_state.mode                  = app_modes_get();
    s_state.target_lux            = 500.0f;
    s_state.sensitivity_pct       = 50;
    s_state.screen_brightness_pct = 100;
    s_state.lang                  = UI_LANG_PT;

    screen_main_create(&s_main);
    screen_auto_create(&s_auto);
    screen_settings_create(&s_settings);
    refresh_all_screens();

    /* [DEBUG] log bruto de toque */
    lv_indev_add_event_cb(lv_indev_get_next(nullptr), on_indev_press_debug,
                          LV_EVENT_PRESSED, nullptr);

    /* ── Tela principal: arrasto/deslizamento ─────────────────────────────── */
    lv_obj_add_event_cb(s_main.root, on_main_pressed,  LV_EVENT_PRESSED,  nullptr);
    lv_obj_add_event_cb(s_main.root, on_main_pressing, LV_EVENT_PRESSING, nullptr);
    lv_obj_add_event_cb(s_main.root, on_main_released, LV_EVENT_RELEASED, nullptr);

    /* Botão de configurações (chip engrenagem no topo) — DESATIVADO por
     * enquanto: fica visível na tela, mas sem handler de clique/navegação.
     * Reativar bastaria descomentar a linha abaixo. */
    // lv_obj_add_event_cb(s_main.cfg_btn, on_navigate, LV_EVENT_CLICKED, s_settings.root);

    /* ── Tela Modo Automático ─────────────────────────────────────────────── */
    lv_obj_add_event_cb(s_auto.header.back_btn,   on_navigate,           LV_EVENT_CLICKED,       s_main.root);
    lv_obj_add_event_cb(s_auto.auto_toggle,        on_mode_toggle,        LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(s_auto.target_minus_btn,   on_target_step,        LV_EVENT_CLICKED,       (void *)(intptr_t)(-1));
    lv_obj_add_event_cb(s_auto.target_plus_btn,    on_target_step,        LV_EVENT_CLICKED,       (void *)(intptr_t)(+1));
    lv_obj_add_event_cb(s_auto.sensitivity_slider, on_sensitivity_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    /* ── Tela Ajustes ─────────────────────────────────────────────────────── */
    lv_obj_add_event_cb(s_settings.header.back_btn,    on_navigate,          LV_EVENT_CLICKED, s_main.root);
    lv_obj_add_event_cb(s_settings.language_seg.btn_a, on_language_selected, LV_EVENT_CLICKED, (void *)(intptr_t)UI_LANG_PT);
    lv_obj_add_event_cb(s_settings.language_seg.btn_b, on_language_selected, LV_EVENT_CLICKED, (void *)(intptr_t)UI_LANG_EN);
    lv_obj_add_event_cb(s_settings.wifi_row,           on_wifi_row_tap,      LV_EVENT_CLICKED, nullptr);

    /* ── Relógio: timer LVGL a cada 1 s (roda sob lock, seguro p/ lv_label_*) */
    lv_timer_t *clock_timer = lv_timer_create(clock_tick_cb, 1000, nullptr);
    clock_tick_cb(clock_timer);  // atualiza imediatamente na inicialização

    /* ── event_bus: mantém s_state em sincronia com o sistema ─────────────── */
    event_bus_register(SMART_SWITCH_EVENT_LDR_UPDATE,           &on_ldr_update,           nullptr);
    event_bus_register(SMART_SWITCH_EVENT_DIMMER_UPDATE,        &on_dimmer_update,        nullptr);
    event_bus_register(SMART_SWITCH_EVENT_MODE_CHANGED,         &on_mode_changed,         nullptr);
    event_bus_register(SMART_SWITCH_EVENT_SETPOINT_CHANGED,     &on_setpoint_changed,     nullptr);
    event_bus_register(SMART_SWITCH_EVENT_POWER_UPDATE,         &on_power_update,         nullptr);
    event_bus_register(SMART_SWITCH_EVENT_NET_STATUS,           &on_net_status,           nullptr);
    event_bus_register(SMART_SWITCH_EVENT_BLE_PROVISION_STATUS, &on_ble_provision_status, nullptr);

    s_initialized = true;
    ESP_LOGI(TAG, "telas montadas, gestos ligados, relógio iniciado");
    return ESP_OK;
}

esp_err_t ui_manager_start(void) {
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    lv_screen_load(s_main.root);
    return lvgl_port_start();
}
