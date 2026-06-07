#pragma once

#include "lvgl.h"
#include "../ui_state.h"
#include "../ui_widgets.h"

/**
 * @brief   Tela principal — variante "MainC" do wireframe (cartão da sala com
 *          stepper de intensidade + cartões Natural/Alvo + botão de força).
 *
 * Expõe os widgets tocáveis para o ui_manager prender os event callbacks
 * (lv_obj_add_event_cb) e decidir o que cada toque significa — esta tela só
 * monta a árvore e redesenha o estado recebido (telas "burras", ui_state.h).
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t *root;              // tela própria, alvo de lv_screen_load()

    lv_obj_t *status_bar;        // toque -> abrir tela de ajustes
    lv_obj_t *status_wifi_label; // "conectado"/"—"
    lv_obj_t *status_mode_dot;   // bolinha cor-de-destaque (visível só em modo automático)
    lv_obj_t *status_mode_label; // "AUTO"/"MANUAL"

    lv_obj_t *room_card;         // toque -> abrir tela de modo automático
    lv_obj_t *room_label;        // nome do ambiente ("Sala de Estar"/"Living Room")
    ui_stepper_t stepper;        // intensidade: btn_minus/value_label/btn_plus

    lv_obj_t *natural_caption;   // legenda do cartão "Natural" (texto fixo PT/EN)
    lv_obj_t *natural_value;     // valor numérico do cartão "Natural" (lx)
    lv_obj_t *target_caption;    // legenda do cartão "Alvo"/"Target"
    lv_obj_t *target_value;      // valor numérico do cartão "Alvo" (lx)

    ui_power_btn_t power_btn;    // liga/desliga a lâmpada
} screen_main_t;

void screen_main_create(screen_main_t *out);
void screen_main_update(const screen_main_t *screen, const ui_runtime_state_t *state);

#ifdef __cplusplus
}
#endif
