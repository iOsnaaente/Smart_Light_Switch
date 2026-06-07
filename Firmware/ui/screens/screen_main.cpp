#include "screen_main.h"
#include "../ui_theme.h"
#include "../board_config.h"

/* ============================================================================
 * Helpers internos
 * ========================================================================== */

// lv_obj "limpo" — base para containers puramente estruturais (linhas/colunas
// flex sem aparência própria), igual ao bare_obj de ui_widgets.cpp.
static lv_obj_t *bare_obj(lv_obj_t *parent) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

/**
 * @brief   Cartão de dado "Natural"/"Alvo" — espelha o helper tile() do
 *          wireframe: ícone (12px, cor muted) + legenda, depois valor em
 *          destaque ("340") seguido do sufixo " lx" em cor muted.
 */
static lv_obj_t *data_tile_create(lv_obj_t *parent, bool sun_icon, const char *caption_text,
                                  lv_obj_t **out_caption_label, lv_obj_t **out_value_label) {
    lv_obj_t *card = ui_card_create(parent);
    lv_obj_set_flex_grow(card, 1);
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(card, 9, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(card, 4, 0);

    lv_obj_t *head = bare_obj(card);
    lv_obj_set_size(head, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(head, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(head, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(head, 5, 0);
    if (sun_icon) {
        ui_icon_sun_create(head, 12, ui_color_muted());
    } else {
        ui_icon_bulb_create(head, 12, ui_color_muted());
    }
    *out_caption_label = ui_caption_create(head, caption_text);

    lv_obj_t *value_row = bare_obj(card);
    lv_obj_set_size(value_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(value_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(value_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);

    lv_obj_t *value_label = lv_label_create(value_row);
    lv_obj_set_style_text_font(value_label, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(value_label, ui_color_ink(), 0);
    lv_label_set_text(value_label, "---");

    lv_obj_t *suffix_label = lv_label_create(value_row);
    lv_obj_set_style_text_font(suffix_label, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(suffix_label, ui_color_muted(), 0);
    lv_label_set_text(suffix_label, " lx");

    *out_value_label = value_label;
    return card;
}

/* ============================================================================
 * Criação
 * ========================================================================== */

void screen_main_create(screen_main_t *out) {
    const ui_strings_t *s = ui_theme_strings(UI_LANG_PT);

    lv_obj_t *root = lv_obj_create(nullptr);
    lv_obj_remove_style_all(root);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(root, LCD_WIDTH, LCD_HEIGHT);
    lv_obj_set_style_bg_color(root, ui_color_paper(), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(root, ui_color_ink(), 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    /* ---- StatusBar: wifi à esquerda, indicador de modo à direita -------- */
    // Toque em qualquer ponto da barra abre os Ajustes — espelha o padrão de
    // "status -> configurações rápidas" comum em painéis touch (o wireframe
    // não anima transições, então o gatilho de navegação é uma escolha nossa).
    lv_obj_t *status_bar = bare_obj(root);
    lv_obj_set_size(status_bar, LV_PCT(100), 30);
    lv_obj_set_style_pad_left(status_bar, 14, 0);
    lv_obj_set_style_pad_right(status_bar, 14, 0);
    lv_obj_set_style_border_side(status_bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(status_bar, UI_HAIRLINE_W, 0);
    lv_obj_set_style_border_color(status_bar, ui_color_hairline(), 0);
    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *status_left = bare_obj(status_bar);
    lv_obj_set_size(status_left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(status_left, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(status_left, 6, 0);

    lv_obj_t *wifi_icon = lv_label_create(status_left);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wifi_icon, ui_color_muted(), 0);

    lv_obj_t *wifi_label = lv_label_create(status_left);
    lv_obj_set_style_text_font(wifi_label, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(wifi_label, ui_color_muted(), 0);
    lv_label_set_text(wifi_label, s->disconnected);

    lv_obj_t *status_right = bare_obj(status_bar);
    lv_obj_set_size(status_right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(status_right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_right, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(status_right, 5, 0);

    lv_obj_t *mode_dot = bare_obj(status_right);
    lv_obj_set_size(mode_dot, 6, 6);
    lv_obj_set_style_radius(mode_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(mode_dot, ui_color_accent(), 0);
    lv_obj_set_style_bg_opa(mode_dot, LV_OPA_COVER, 0);

    lv_obj_t *mode_label = lv_label_create(status_right);
    lv_obj_set_style_text_font(mode_label, UI_FONT_BODY, 0);
    lv_obj_set_style_text_letter_space(mode_label, 1, 0);
    lv_label_set_text(mode_label, s->mode_auto);
    // cor/visibilidade do ponto dependem do modo atual -> recalculados em _update

    /* ---- Corpo: cartão da sala, cartões de dados e botão de força ------- */
    lv_obj_t *body = bare_obj(root);
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_flex_grow(body, 1);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(body, UI_PAD_SCREEN, 0);
    lv_obj_set_style_pad_row(body, UI_GAP, 0);

    // Cartão da sala: nome (canto superior esquerdo, posição absoluta) + stepper
    // centralizado. Toque abre o Modo Automático — é o cartão mais proeminente
    // e a tela cujo conteúdo ele resume (alvo/sensibilidade daquele ambiente).
    lv_obj_t *room_card = ui_card_create(body);
    lv_obj_set_width(room_card, LV_PCT(100));
    lv_obj_set_flex_grow(room_card, 1);
    lv_obj_set_flex_flow(room_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(room_card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(room_card, 8, 0);

    lv_obj_t *room_label = ui_caption_create(room_card, s->room);
    lv_obj_add_flag(room_label, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_pos(room_label, 12, 10);

    ui_stepper_t stepper;
    ui_stepper_create(room_card, &stepper);

    // Linha de cartões de dados: Natural (sol) / Alvo (lâmpada) — somente
    // leitura, ambos os ícones em cor muted (assim aparecem no wireframe).
    lv_obj_t *tile_row = bare_obj(body);
    lv_obj_set_width(tile_row, LV_PCT(100));
    lv_obj_set_height(tile_row, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(tile_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(tile_row, UI_GAP, 0);

    lv_obj_t *natural_caption = nullptr, *natural_value = nullptr;
    lv_obj_t *target_caption  = nullptr, *target_value  = nullptr;
    data_tile_create(tile_row, true,  s->natural, &natural_caption, &natural_value);
    data_tile_create(tile_row, false, s->target,  &target_caption,  &target_value);

    // Botão de força — preenchido (ligada) / contornado (desligada)
    ui_power_btn_t power_btn;
    ui_power_btn_create(body, &power_btn);
    ui_power_btn_set_on(&power_btn, true, s->on, s->off);

    out->root                = root;
    out->status_bar          = status_bar;
    out->status_wifi_label   = wifi_label;
    out->status_mode_dot     = mode_dot;
    out->status_mode_label   = mode_label;
    out->room_card           = room_card;
    out->room_label          = room_label;
    out->stepper             = stepper;
    out->natural_caption     = natural_caption;
    out->natural_value       = natural_value;
    out->target_caption      = target_caption;
    out->target_value        = target_value;
    out->power_btn           = power_btn;
}

/* ============================================================================
 * Atualização a partir do estado (ui_runtime_state_t)
 * ========================================================================== */

void screen_main_update(const screen_main_t *screen, const ui_runtime_state_t *state) {
    const ui_strings_t *s = ui_theme_strings(state->lang);

    /* status bar ---------------------------------------------------------- */
    lv_label_set_text(screen->status_wifi_label, state->wifi_connected ? s->connected : s->disconnected);

    // O wireframe só prevê "auto"/"manual"; tratamos ECO como uma variante
    // automática (a lâmpada continua sendo conduzida pelo controlador, não
    // pelo toque do usuário) para fins de cor/rótulo do indicador.
    const bool auto_like = (state->mode != APP_MODE_MANUAL);
    lv_label_set_text(screen->status_mode_label, auto_like ? s->mode_auto : s->mode_manual);
    lv_obj_set_style_text_color(screen->status_mode_label, auto_like ? ui_color_accent() : ui_color_muted(), 0);
    if (auto_like) {
        lv_obj_remove_flag(screen->status_mode_dot, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(screen->status_mode_dot, LV_OBJ_FLAG_HIDDEN);
    }

    /* cartão da sala ------------------------------------------------------- */
    ui_caption_set_text(screen->room_label, s->room);
    ui_stepper_set_value(&screen->stepper, state->brightness_pct);

    /* cartões Natural / Alvo ------------------------------------------------ */
    ui_caption_set_text(screen->natural_caption, s->natural);
    ui_caption_set_text(screen->target_caption, s->target);
    lv_label_set_text_fmt(screen->natural_value, "%d", (int)(state->natural_lux + 0.5f));
    lv_label_set_text_fmt(screen->target_value, "%d", (int)(state->target_lux + 0.5f));

    /* botão de força -------------------------------------------------------- */
    ui_power_btn_set_on(&screen->power_btn, state->relay_on, s->on, s->off);
}
