#pragma once

#include "lvgl.h"
#include "../ui_state.h"
#include "../ui_widgets.h"

/**
 * @brief   Tela "Ajustes" — idioma, brilho da tela, calibração do sensor,
 *          rede Wi-Fi e versão do firmware.
 *
 * Mesma filosofia de screen_main/screen_auto: tela "burra" — só monta a
 * árvore e redesenha o estado recebido; o ui_manager decide o que cada
 * toque (idioma, calibrar, Wi-Fi, voltar) significa via lv_obj_add_event_cb.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t *root;             // tela própria, alvo de lv_screen_load()
    ui_screen_header_t header;  // back_btn / title_label; trailing = engrenagem (decorativa)

    lv_obj_t *language_label;   // "Idioma"/"Language"
    ui_seg2_t language_seg;     // PT/EN — toque alterna o idioma da interface

    lv_obj_t *screen_bright_label; // "Brilho da tela"/"Screen brightness"
    lv_obj_t *screen_bright_fill;  // barra de progresso (somente leitura, espelha screen_brightness_pct)

    lv_obj_t *calib_label;      // "Calibrar sensor"/"Calibrate sensor"
    lv_obj_t *calib_row;        // toque -> inicia rotina de calibração do LDR

    lv_obj_t *wifi_label;       // "Wi-Fi"
    lv_obj_t *wifi_value_label; // IP atual (conectado) ou "—" (desconectado)
    lv_obj_t *wifi_row;         // toque -> detalhes de rede

    lv_obj_t *power_label;       // "Potência"/"Power"
    lv_obj_t *power_value_label; // potência ativa atual (mesma fonte do backend: event_bus POWER_UPDATE)

    lv_obj_t *about_label;      // "Sobre"/"About" (versão é texto fixo do firmware)
} screen_settings_t;

void screen_settings_create(screen_settings_t *out);
void screen_settings_update(const screen_settings_t *screen, const ui_runtime_state_t *state);

#ifdef __cplusplus
}
#endif
