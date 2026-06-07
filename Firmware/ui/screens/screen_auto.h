#pragma once

#include "lvgl.h"
#include "../ui_state.h"
#include "../ui_widgets.h"

/**
 * @brief   Tela "Modo Automático" — interruptor mestre, alvo de iluminação
 *          (lx) ajustável, mistura Natural+Lâmpada e sensibilidade.
 *
 * Mesma filosofia de screen_main: tela "burra" — só monta a árvore e
 * redesenha o estado recebido; o ui_manager decide o que cada toque
 * (interruptor, +/-, voltar) significa via lv_obj_add_event_cb.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_obj_t *root;             // tela própria, alvo de lv_screen_load()
    ui_screen_header_t header;  // back_btn / title_label; trailing = interruptor
    lv_obj_t *auto_toggle;      // interruptor mestre (automático <-> manual)

    lv_obj_t *desc_label;       // "Quando escurece, a lâmpada completa até o alvo."

    lv_obj_t *target_caption;   // "Alvo"
    lv_obj_t *target_value;     // valor numérico do alvo, em lx
    lv_obj_t *target_minus_btn; // diminui o alvo
    lv_obj_t *target_plus_btn;  // aumenta o alvo

    lv_obj_t *mix_caption;      // "Natural + Lâmpada"
    lv_obj_t *mix_natural_seg;  // segmento da barra: contribuição da luz natural
    lv_obj_t *mix_lamp_seg;     // segmento da barra: contribuição da lâmpada

    lv_obj_t *sensitivity_caption; // "Sensibilidade"
    lv_obj_t *sensitivity_value;   // descrição textual do nível atual
    lv_obj_t *sensitivity_slider;  // ajuste arrastável (sensitivity_pct)
} screen_auto_t;

void screen_auto_create(screen_auto_t *out);
void screen_auto_update(const screen_auto_t *screen, const ui_runtime_state_t *state);

#ifdef __cplusplus
}
#endif
