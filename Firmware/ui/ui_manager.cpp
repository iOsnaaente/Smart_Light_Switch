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

/* Limiar de deslizamento: 60% da largura da tela */
static constexpr int32_t SWIPE_THRESHOLD = LCD_WIDTH * 6 / 10;
/* Altura da barra superior: toques nessa faixa não iniciam arrasto */
static constexpr int32_t TOPBAR_H = 54;

static screen_main_t      s_main;
static screen_auto_t      s_auto;
static screen_settings_t  s_settings;
static ui_runtime_state_t s_state = {};
static bool               s_initialized = false;

/* ── Estado de arrasto/deslizamento na tela principal ─────────────────────── */
static lv_point_t s_drag_start;       // ponto de toque inicial
static lv_point_t s_drag_last;        // último ponto de toque (para cálculo do dx final)
static float      s_drag_start_lux;   // target_lux no momento em que o toque começou
static bool       s_dragging = false; // está rastreando um toque?
static bool       s_cfg_zone = false; // toque iniciou na zona do botão de config?

/* Zona fixa do botão de configurações: coordenadas BRUTAS do XPT2046.
 * Se o toque de pressão inicial cair dentro deste retângulo, navega para
 * a tela de Ajustes ao soltar — independente de LVGL hit-testing. */
static constexpr int CFG_RAW_X_MIN = 150;
static constexpr int CFG_RAW_X_MAX = 250;
static constexpr int CFG_RAW_Y_MIN = 100;
static constexpr int CFG_RAW_Y_MAX = 200;

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

/* ── Arrasto/deslizamento na tela principal ─────────────────────────────── */

static void on_main_pressed(lv_event_t *e) {
    /* Verifica a zona fixa do botão de config via coordenadas BRUTAS do ADC
     * (bypassa o hit-test do LVGL, que pode falhar com o topbar sobreposto). */
    int raw_x, raw_y;
    lvgl_port_get_raw_touch(&raw_x, &raw_y);
    if (raw_x >= CFG_RAW_X_MIN && raw_x <= CFG_RAW_X_MAX &&
        raw_y >= CFG_RAW_Y_MIN && raw_y <= CFG_RAW_Y_MAX) {
        s_cfg_zone = true;
        s_dragging  = false;
        return;
    }
    s_cfg_zone = false;

    lv_indev_t *indev = lv_indev_active();
    lv_point_t pt;
    lv_indev_get_point(indev, &pt);

    /* Ignora toques na barra superior (onde fica o cfg_btn visual) */
    if (pt.y < TOPBAR_H) return;

    s_drag_start     = pt;
    s_drag_last      = pt;
    s_drag_start_lux = s_state.target_lux;
    s_dragging       = true;

    /* Atualiza SP imediatamente ao toque:
     * SP = (1 – y/H) × LUX_SCALE, com y=0 no topo e y=LCD_HEIGHT no fundo. */
    float new_lux = (float)(LCD_HEIGHT - pt.y) * UI_AMBIENT_LUX_SCALE / LCD_HEIGHT;
    if (new_lux < 0.0f)                  new_lux = 0.0f;
    if (new_lux > UI_AMBIENT_LUX_SCALE)  new_lux = UI_AMBIENT_LUX_SCALE;
    s_state.target_lux = new_lux;
    s_state.relay_on   = (new_lux > 0.0f);
    screen_main_update(&s_main, &s_state);
}

static void on_main_pressing(lv_event_t *e) {
    if (s_cfg_zone || !s_dragging) return;
    lv_indev_t *indev = lv_indev_active();
    lv_point_t pt;
    lv_indev_get_point(indev, &pt);
    s_drag_last = pt;

    /* Atualiza SP em tempo real conforme o dedo se move */
    float new_lux = (float)(LCD_HEIGHT - pt.y) * UI_AMBIENT_LUX_SCALE / LCD_HEIGHT;
    if (new_lux < 0.0f)                  new_lux = 0.0f;
    if (new_lux > UI_AMBIENT_LUX_SCALE)  new_lux = UI_AMBIENT_LUX_SCALE;
    s_state.target_lux = new_lux;
    s_state.relay_on   = (new_lux > 0.0f);
    screen_main_update(&s_main, &s_state);
}

static void on_main_released(lv_event_t *e) {
    /* Toque na zona do botão de config → navega para Ajustes */
    if (s_cfg_zone) {
        s_cfg_zone = false;
        s_dragging  = false;
        ESP_LOGI(TAG, "Zona config: abre Ajustes");
        lv_screen_load(s_settings.root);
        return;
    }

    if (!s_dragging) return;
    s_dragging = false;

    int32_t dx  = s_drag_last.x - s_drag_start.x;
    int32_t adx = dx < 0 ? -dx : dx;

    if (adx > SWIPE_THRESHOLD) {
        /* ── Deslizamento detectado: cancela o ajuste de SP ─────────────── */
        s_state.target_lux = s_drag_start_lux;
        s_state.relay_on   = (s_drag_start_lux > 0.0f);
        screen_main_update(&s_main, &s_state);

        if (dx < 0) {
            ESP_LOGI(TAG, "Deslize <- : abre Ajustes");
            lv_screen_load(s_settings.root);
        } else {
            ESP_LOGI(TAG, "Deslize -> : desliga lâmpada");
            event_relay_command_t cmd = { .relay_on = false };
            event_bus_post(SMART_SWITCH_EVENT_RELAY_COMMAND, &cmd, sizeof(cmd), pdMS_TO_TICKS(100));
        }
    } else {
        /* ── Arrasto vertical: confirma novo SP via SETPOINT_CHANGED ─────── */
        event_setpoint_changed_t evt = { .setpoint = s_state.target_lux };
        event_bus_post(SMART_SWITCH_EVENT_SETPOINT_CHANGED, &evt, sizeof(evt), pdMS_TO_TICKS(100));
        ESP_LOGI(TAG, "SP confirmado: %.0f lux", s_state.target_lux);
    }
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
        lv_label_set_text(s_main.clock_label, "--:--");
    } else {
        lv_label_set_text_fmt(s_main.clock_label, "%02d:%02d",
                              tm_info.tm_hour, tm_info.tm_min);
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

static void on_ble_provision_status(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
    event_ble_provision_status_t *evt = (event_ble_provision_status_t *)event_data;
    if (!lvgl_port_lock(100)) {
        return;
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

    /* Botão de configurações (chip engrenagem no topo) → Ajustes */
    lv_obj_add_event_cb(s_main.cfg_btn, on_navigate, LV_EVENT_CLICKED, s_settings.root);

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
