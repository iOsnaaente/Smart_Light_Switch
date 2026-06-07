#pragma once

#include "lvgl.h"
#include "ui_state.h"

/**
 * @brief   Constantes visuais e tabela de strings PT/EN da interface.
 *
 * Espelha o sistema de design dos wireframes (ui/wireframes/wireframe-screens.jsx):
 * mesma paleta (--accent/--ink/--paper/--card/--track/--hair/--natural/--muted),
 * mesmo raio de canto (--r: "arredondado" = 14px) e mesma tabela STR.pt/STR.en.
 */

// Mantém consistência com MQTT_LUX_SCALE (communication/mqtt_client.cpp): os
// dois lugares aproximam lux a partir da leitura normalizada do LDR. Duplicado
// localmente porque communication/ não pode incluir board_config.h (arrasta
// esp_lcd) e ui/ não deveria depender de communication/ só por uma constante —
// se a escala for recalibrada, atualize os dois lugares.
static constexpr float UI_AMBIENT_LUX_SCALE = 1000.0f;

// Raios e espaçamentos (escala 1x dos wireframes — tela física 240x320)
static constexpr int32_t UI_RADIUS_CARD   = 14;
static constexpr int32_t UI_RADIUS_CHIP   = 10;
static constexpr int32_t UI_PAD_SCREEN    = 12;
static constexpr int32_t UI_GAP           = 9;
static constexpr int32_t UI_HAIRLINE_W    = 1;

// Fontes habilitadas via sdkconfig (CONFIG_LV_FONT_MONTSERRAT_14/28)
#define UI_FONT_BODY  (&lv_font_montserrat_14)
#define UI_FONT_HERO  (&lv_font_montserrat_28)

/* ---- paleta (equivalente às custom properties CSS dos wireframes) -------- */

static inline lv_color_t ui_color_accent(void)   { return lv_color_hex(0x9B7BD4); }
static inline lv_color_t ui_color_ink(void)      { return lv_color_hex(0x23262B); }
static inline lv_color_t ui_color_paper(void)    { return lv_color_hex(0xF5F6F8); }
static inline lv_color_t ui_color_card(void)     { return lv_color_hex(0xFFFFFF); }
static inline lv_color_t ui_color_track(void)    { return lv_color_hex(0xECEEF1); }
static inline lv_color_t ui_color_hairline(void) { return lv_color_hex(0xE2E5EA); }
static inline lv_color_t ui_color_natural(void)  { return lv_color_hex(0xECD9A3); }
static inline lv_color_t ui_color_muted(void)    { return lv_color_hex(0x8A9099); }
static inline lv_color_t ui_color_white(void)    { return lv_color_hex(0xFFFFFF); }

/* ---- strings PT/EN (espelha STR.pt/STR.en do wireframe) ------------------ */

typedef struct {
    const char *room;
    const char *on;
    const char *off;
    const char *mode_auto;
    const char *mode_manual;
    const char *natural;
    const char *lamp;
    const char *target;
    const char *ambient;
    const char *bright;
    const char *lux;
    const char *auto_mode_title;
    const char *settings;
    const char *language;
    const char *screen_bright;
    const char *calib;
    const char *about;
    const char *sensitivity;
    const char *med;
    const char *compensating;
    const char *connected;
    const char *disconnected;
    const char *when_dark;
    const char *wifi;
} ui_strings_t;

static inline const ui_strings_t *ui_theme_strings(ui_lang_t lang) {
    static const ui_strings_t pt = {
        .room = "Sala de Estar", .on = "Ligada", .off = "Desligada",
        .mode_auto = "AUTO", .mode_manual = "MANUAL",
        .natural = "Natural", .lamp = "Lâmpada", .target = "Alvo", .ambient = "Ambiente",
        .bright = "Intensidade", .lux = "lx", .auto_mode_title = "Modo Automático", .settings = "Ajustes",
        .language = "Idioma", .screen_bright = "Brilho da tela", .calib = "Calibrar sensor",
        .about = "Sobre", .sensitivity = "Sensibilidade", .med = "média", .compensating = "compensando",
        .connected = "conectado", .disconnected = "—",
        .when_dark = "Quando escurece, a lâmpada completa até o alvo.",
        .wifi = "Wi-Fi",
    };
    static const ui_strings_t en = {
        .room = "Living Room", .on = "On", .off = "Off",
        .mode_auto = "AUTO", .mode_manual = "MANUAL",
        .natural = "Natural", .lamp = "Lamp", .target = "Target", .ambient = "Ambient",
        .bright = "Brightness", .lux = "lx", .auto_mode_title = "Auto Mode", .settings = "Settings",
        .language = "Language", .screen_bright = "Screen brightness", .calib = "Calibrate sensor",
        .about = "About", .sensitivity = "Sensitivity", .med = "medium", .compensating = "compensating",
        .connected = "connected", .disconnected = "—",
        .when_dark = "When it gets dark, the lamp fills up to target.",
        .wifi = "Wi-Fi",
    };
    return (lang == UI_LANG_EN) ? &en : &pt;
}
