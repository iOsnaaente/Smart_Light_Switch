#include "screen_auto.h"
#include "../ui_theme.h"
#include "../board_config.h"

#include <cstdio>

/* ============================================================================
 * Helpers internos
 * ========================================================================== */

static lv_obj_t *bare_obj(lv_obj_t *parent) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

// "Natural + Lâmpada"/"Natural + Lamp" — concatena duas entradas da tabela de
// strings; cabe folgado no buffer de 48B de ui_caption_create/label_set_text_upper.
static void format_mix_caption(char *out, size_t out_len, const ui_strings_t *s) {
    snprintf(out, out_len, "%s + %s", s->natural, s->lamp);
}

/* ============================================================================
 * Criação
 * ========================================================================== */

void screen_auto_create(screen_auto_t *out) {
    const ui_strings_t *s = ui_theme_strings(UI_LANG_PT);

    lv_obj_t *root = lv_obj_create(nullptr);
    lv_obj_remove_style_all(root);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(root, LCD_WIDTH, LCD_HEIGHT);
    lv_obj_set_style_bg_color(root, ui_color_paper(), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(root, ui_color_ink(), 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    /* ---- Cabeçalho: voltar + título + interruptor mestre ---------------- */
    ui_screen_header_t header;
    ui_screen_header_create(root, s->auto_mode_title, &header);

    // Interruptor automático<->manual — restilizado via partes nativas do
    // lv_switch: INDICATOR é a "pílula" (cinza/destaque conforme o estado) e
    // KNOB é o círculo branco. pad_all(-3, KNOB) encolhe a alça 44x25 -> 19x19
    // com 3px de respiro, batendo nos pixels de "top:3, right:3, w:19, h:19".
    lv_obj_t *toggle = lv_switch_create(header.trailing);
    lv_obj_remove_style_all(toggle);
    lv_obj_set_size(toggle, 44, 25);
    lv_obj_set_style_radius(toggle, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(toggle, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(toggle, ui_color_track(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(toggle, ui_color_accent(), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_radius(toggle, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(toggle, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_bg_color(toggle, ui_color_white(), LV_PART_KNOB);
    lv_obj_set_style_pad_all(toggle, -3, LV_PART_KNOB);

    /* ---- Corpo: explicação, alvo, mistura e sensibilidade --------------- */
    lv_obj_t *body = bare_obj(root);
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_flex_grow(body, 1);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_top(body, 12, 0);
    lv_obj_set_style_pad_bottom(body, 12, 0);
    lv_obj_set_style_pad_left(body, 14, 0);
    lv_obj_set_style_pad_right(body, 14, 0);
    lv_obj_set_style_pad_row(body, 11, 0);

    lv_obj_t *desc_label = lv_label_create(body);
    lv_obj_set_width(desc_label, LV_PCT(100));
    lv_obj_set_style_text_font(desc_label, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(desc_label, ui_color_muted(), 0);
    lv_obj_set_style_text_line_space(desc_label, 3, 0);
    lv_label_set_long_mode(desc_label, LV_LABEL_LONG_MODE_WRAP);
    lv_label_set_text(desc_label, s->when_dark);

    /* ---- Cartão do alvo: legenda+valor à esquerda, +/- à direita -------- */
    lv_obj_t *target_card = ui_card_create(body);
    lv_obj_set_width(target_card, LV_PCT(100));
    lv_obj_set_height(target_card, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(target_card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(target_card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *target_left = bare_obj(target_card);
    lv_obj_set_size(target_left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(target_left, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *target_caption = ui_caption_create(target_left, s->target);

    lv_obj_t *target_value_row = bare_obj(target_left);
    lv_obj_set_size(target_value_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(target_value_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(target_value_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);

    lv_obj_t *target_value = lv_label_create(target_value_row);
    lv_obj_set_style_text_font(target_value, UI_FONT_HERO, 0);
    lv_obj_set_style_text_color(target_value, ui_color_ink(), 0);
    lv_label_set_text(target_value, "---");

    lv_obj_t *target_unit = lv_label_create(target_value_row);
    lv_obj_set_style_text_font(target_unit, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(target_unit, ui_color_muted(), 0);
    lv_label_set_text_fmt(target_unit, " %s", s->lux);

    lv_obj_t *target_btn_row = bare_obj(target_card);
    lv_obj_set_size(target_btn_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(target_btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(target_btn_row, 9, 0);

    lv_obj_t *target_minus_btn = ui_square_btn_create(target_btn_row, LV_SYMBOL_DOWN, false);
    lv_obj_t *target_plus_btn  = ui_square_btn_create(target_btn_row, LV_SYMBOL_UP, true);

    /* ---- Barra de mistura Natural + Lâmpada ------------------------------ */
    lv_obj_t *mix_section = bare_obj(body);
    lv_obj_set_width(mix_section, LV_PCT(100));
    lv_obj_set_height(mix_section, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(mix_section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(mix_section, 6, 0);

    char mix_caption_buf[40];
    format_mix_caption(mix_caption_buf, sizeof(mix_caption_buf), s);
    lv_obj_t *mix_caption = ui_caption_create(mix_section, mix_caption_buf);

    lv_obj_t *mix_bar = bare_obj(mix_section);
    lv_obj_set_size(mix_bar, LV_PCT(100), 22);
    lv_obj_set_style_radius(mix_bar, UI_RADIUS_CARD, 0);
    lv_obj_set_style_clip_corner(mix_bar, true, 0);
    lv_obj_set_style_border_width(mix_bar, UI_HAIRLINE_W, 0);
    lv_obj_set_style_border_color(mix_bar, ui_color_hairline(), 0);
    lv_obj_set_style_bg_color(mix_bar, ui_color_track(), 0);
    lv_obj_set_style_bg_opa(mix_bar, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(mix_bar, LV_FLEX_FLOW_ROW);

    // Larguras dos 2 primeiros segmentos são recalculadas em _update (LV_PCT);
    // o terceiro só cresce (flex_grow) para preencher o que sobrar — sem
    // precisar saber sua largura, igual ao "flex:1" do wireframe.
    lv_obj_t *mix_natural_seg = bare_obj(mix_bar);
    lv_obj_set_size(mix_natural_seg, 0, LV_PCT(100));
    lv_obj_set_style_bg_color(mix_natural_seg, ui_color_natural(), 0);
    lv_obj_set_style_bg_opa(mix_natural_seg, LV_OPA_COVER, 0);

    lv_obj_t *mix_lamp_seg = bare_obj(mix_bar);
    lv_obj_set_size(mix_lamp_seg, 0, LV_PCT(100));
    lv_obj_set_style_bg_color(mix_lamp_seg, ui_color_accent(), 0);
    lv_obj_set_style_bg_opa(mix_lamp_seg, LV_OPA_COVER, 0);

    lv_obj_t *mix_track_seg = bare_obj(mix_bar);
    lv_obj_set_size(mix_track_seg, LV_SIZE_CONTENT, LV_PCT(100));
    lv_obj_set_flex_grow(mix_track_seg, 1);

    /* ---- Sensibilidade: legenda+nível, depois slider arrastável ---------- */
    lv_obj_t *sens_section = bare_obj(body);
    lv_obj_set_width(sens_section, LV_PCT(100));
    lv_obj_set_height(sens_section, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(sens_section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(sens_section, 8, 0);

    lv_obj_t *sens_head = bare_obj(sens_section);
    lv_obj_set_size(sens_head, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(sens_head, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(sens_head, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);

    lv_obj_t *sensitivity_caption = ui_caption_create(sens_head, s->sensitivity);

    lv_obj_t *sensitivity_value = lv_label_create(sens_head);
    lv_obj_set_style_text_font(sensitivity_value, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(sensitivity_value, ui_color_ink(), 0);
    lv_label_set_text(sensitivity_value, s->med);

    // Trilha fina (6px) com alça grande (18px): a altura total do controle é
    // a da alça, e o pad vertical do MAIN encolhe a área desenhada da trilha
    // para 18-6-6=6px centralizados — técnica padrão do LVGL p/ "slider fino".
    lv_obj_t *slider = lv_slider_create(sens_section);
    lv_obj_remove_style_all(slider);
    lv_obj_set_size(slider, LV_PCT(100), 18);
    lv_obj_set_style_pad_top(slider, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(slider, 6, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, 3, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, ui_color_track(), LV_PART_MAIN);
    lv_obj_set_style_radius(slider, 3, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, ui_color_accent(), LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_bg_color(slider, ui_color_card(), LV_PART_KNOB);
    lv_obj_set_style_border_width(slider, 2, LV_PART_KNOB);
    lv_obj_set_style_border_color(slider, ui_color_accent(), LV_PART_KNOB);
    lv_obj_set_style_shadow_width(slider, 4, LV_PART_KNOB);
    lv_obj_set_style_shadow_offset_y(slider, 1, LV_PART_KNOB);
    lv_obj_set_style_shadow_color(slider, lv_color_black(), LV_PART_KNOB);
    lv_obj_set_style_shadow_opa(slider, LV_OPA_20, LV_PART_KNOB);
    lv_slider_set_range(slider, 0, 100);

    out->root                = root;
    out->header              = header;
    out->auto_toggle         = toggle;
    out->desc_label          = desc_label;
    out->target_caption      = target_caption;
    out->target_value        = target_value;
    out->target_minus_btn    = target_minus_btn;
    out->target_plus_btn     = target_plus_btn;
    out->mix_caption         = mix_caption;
    out->mix_natural_seg     = mix_natural_seg;
    out->mix_lamp_seg        = mix_lamp_seg;
    out->sensitivity_caption = sensitivity_caption;
    out->sensitivity_value   = sensitivity_value;
    out->sensitivity_slider  = slider;
}

/* ============================================================================
 * Atualização a partir do estado (ui_runtime_state_t)
 * ========================================================================== */

void screen_auto_update(const screen_auto_t *screen, const ui_runtime_state_t *state) {
    const ui_strings_t *s = ui_theme_strings(state->lang);

    lv_label_set_text(screen->header.title_label, s->auto_mode_title);
    lv_obj_set_state(screen->auto_toggle, LV_STATE_CHECKED, state->mode != APP_MODE_MANUAL);
    lv_label_set_text(screen->desc_label, s->when_dark);

    ui_caption_set_text(screen->target_caption, s->target);
    lv_label_set_text_fmt(screen->target_value, "%d", (int)(state->target_lux + 0.5f));

    char mix_caption_buf[40];
    format_mix_caption(mix_caption_buf, sizeof(mix_caption_buf), s);
    ui_caption_set_text(screen->mix_caption, mix_caption_buf);

    // Heurística da barra Natural+Lâmpada: a luz natural cobre uma fração do
    // alvo; o que falta é parcialmente coberto pela lâmpada, proporcional ao
    // quanto ela está acesa (brightness_pct). Não existe (ainda) um modelo de
    // lux do LED, então isto é a aproximação mais honesta com os dados que
    // realmente temos — o restante (track) fica para quem ainda falta cobrir.
    const float target = (state->target_lux > 1.0f) ? state->target_lux : 1.0f;
    float natural_frac = state->natural_lux / target;
    if (natural_frac < 0.0f) natural_frac = 0.0f;
    if (natural_frac > 1.0f) natural_frac = 1.0f;
    const float lamp_frac = (1.0f - natural_frac) * ((float)state->brightness_pct / 100.0f);

    lv_obj_set_width(screen->mix_natural_seg, LV_PCT((int)(natural_frac * 100.0f + 0.5f)));
    lv_obj_set_width(screen->mix_lamp_seg, LV_PCT((int)(lamp_frac * 100.0f + 0.5f)));

    ui_caption_set_text(screen->sensitivity_caption, s->sensitivity);
    // O wireframe só define uma string de nível ("média"/"medium"); sem
    // baixa/alta na tabela, mantemos o texto fixo e deixamos o slider (valor
    // numérico real) como a representação que de fato responde ao toque.
    lv_label_set_text(screen->sensitivity_value, s->med);
    lv_slider_set_value(screen->sensitivity_slider, state->sensitivity_pct, LV_ANIM_OFF);
}
