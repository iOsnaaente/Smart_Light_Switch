#pragma once

#include "lvgl.h"

/**
 * @brief   Componentes reutilizáveis da interface — espelham os componentes
 *          do wireframe (Card, Label, Stepper, PowerBtn, ícones, cabeçalho
 *          com chevron de voltar) para evitar duplicar layout/estilo entre
 *          as 3 telas.
 *
 * Ícones "wifi"/"power"/"gear"/chevrons usam os símbolos nativos do LVGL
 * (LV_SYMBOL_*) — mais nítidos e sem custo extra de desenho. "sun"/"bulb"
 * não têm equivalente nativo e carregam significado (rotulam Natural/Alvo
 * nos cartões), por isso são desenhados à mão replicando as formas simples
 * do wireframe (anel+ponto / anel+base).
 */

#ifdef __cplusplus
extern "C" {
#endif

/* ---- ícones desenhados à mão (sun/bulb) ----------------------------------- */

lv_obj_t *ui_icon_sun_create(lv_obj_t *parent, int32_t size, lv_color_t color);
lv_obj_t *ui_icon_bulb_create(lv_obj_t *parent, int32_t size, lv_color_t color);

/* ---- containers/labels básicos --------------------------------------------- */

// Cartão branco com borda hairline e cantos arredondados (Card do wireframe)
lv_obj_t *ui_card_create(lv_obj_t *parent);

// Rótulo pequeno, maiúsculo, cor muted (Label do wireframe)
lv_obj_t *ui_caption_create(lv_obj_t *parent, const char *text);

// Atualiza o texto de uma legenda já criada, preservando a transformação para maiúsculas
// (necessário ao trocar PT/EN em runtime — ui_caption_create só roda na criação)
void ui_caption_set_text(lv_obj_t *label, const char *text);

// Botão quadrado 42x42 com símbolo central — usado nos chevrons de ajuste de alvo (Btn do wireframe)
lv_obj_t *ui_square_btn_create(lv_obj_t *parent, const char *symbol, bool accent);

/* ---- cabeçalho de tela com chevron de voltar (AutoScreen/SettingsScreen) --- */

typedef struct {
    lv_obj_t *root;
    lv_obj_t *back_btn;     // alvo de toque para voltar à tela principal
    lv_obj_t *title_label;
    lv_obj_t *trailing;     // slot à direita (switch / ícone de engrenagem)
} ui_screen_header_t;

void ui_screen_header_create(lv_obj_t *parent, const char *title, ui_screen_header_t *out);

/* ---- stepper de intensidade (− valor% +) ----------------------------------- */

typedef struct {
    lv_obj_t *root;
    lv_obj_t *btn_minus;
    lv_obj_t *btn_plus;
    lv_obj_t *value_label;
} ui_stepper_t;

void ui_stepper_create(lv_obj_t *parent, ui_stepper_t *out);
void ui_stepper_set_value(const ui_stepper_t *stepper, int value_pct);

/* ---- botão de força (Ligada/Desligada) -------------------------------------- */

typedef struct {
    lv_obj_t *root;
    lv_obj_t *icon;
    lv_obj_t *label;
} ui_power_btn_t;

void ui_power_btn_create(lv_obj_t *parent, ui_power_btn_t *out);
void ui_power_btn_set_on(const ui_power_btn_t *btn, bool on, const char *on_text, const char *off_text);

/* ---- segmentado de 2 opções (Seg do wireframe — usado por Idioma PT/EN) ----- */

typedef struct {
    lv_obj_t *root;
    lv_obj_t *btn_a;
    lv_obj_t *label_a;
    lv_obj_t *btn_b;
    lv_obj_t *label_b;
} ui_seg2_t;

void ui_seg2_create(lv_obj_t *parent, const char *text_a, const char *text_b, ui_seg2_t *out);
void ui_seg2_set_active(const ui_seg2_t *seg, bool a_active);

#ifdef __cplusplus
}
#endif
