#pragma once

#include "lvgl.h"
#include "../ui_state.h"

/**
 * @brief   Tela principal — design "superfície de luz" (MainPage.html):
 *          fundo escuro, camadas de gradiente azul (acionamento) e âmbar
 *          (luz) reveladas de baixo para cima conforme a intensidade sobe,
 *          barra superior com chip Wi-Fi + relógio + engrenagem, marcadores
 *          de nível e readout centralizado com o valor em %.
 *
 *          Gesto vertical: arraste → ajusta intensidade em tempo real.
 *          Gesto horizontal (>60% da largura):
 *            deslize esquerda → abre Ajustes
 *            deslize direita  → desliga a lâmpada completamente
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t *root;

    /* Camadas de gradiente (reveladas de baixo para cima) */
    lv_obj_t *drive_layer;       // container azul; altura ∝ % de acionamento
    lv_obj_t *drive_paint;       // gradiente LCD_HEIGHT px (clippado pela layer)
    lv_obj_t *light_layer;       // container âmbar; altura ∝ % de intensidade
    lv_obj_t *light_paint;       // gradiente LCD_HEIGHT px (clippado pela layer)

    /* Bloom no topo (opacidade ∝ intensidade) */
    lv_obj_t *glow;

    /* Barra superior */
    lv_obj_t *wifi_dot;          // bolinha verde (visível quando conectado)
    lv_obj_t *clock_label;       // "HH:MM"
    lv_obj_t *cfg_btn;           // chip tocável → Ajustes

    /* Marcador MV — manipulated variable: saída real do dimmer/TRIAC (linha azul) */
    lv_obj_t *drive_marker;      // container posicionado em abs y
    lv_obj_t *drive_tag;         // "MV XX%"

    /* Marcador de luz (linha âmbar) */
    lv_obj_t *light_marker;      // container posicionado em abs y

    /* Readout central */
    lv_obj_t *readout_kicker;    // "SETPOINT"
    lv_obj_t *big_val;           // número grande (ex.: "72") — SP em lux
    lv_obj_t *big_pct;           // sufixo "lx"
    lv_obj_t *mv_val;            // "MV: XX%" — saída real do dimmer/TRIAC
    lv_obj_t *pv_val;            // "PV: XXX lx" — leitura ao vivo do sensor LDR
} screen_main_t;

void screen_main_create(screen_main_t *out);
void screen_main_update(const screen_main_t *screen, const ui_runtime_state_t *state);

#ifdef __cplusplus
}
#endif
