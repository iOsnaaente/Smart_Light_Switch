#include "ui_widgets.h"
#include "ui_theme.h"

#include <ctype.h>

/* ============================================================================
 * Helpers internos
 * ========================================================================== */

// lv_obj "limpo": sem estilo de tema (borda/sombra/padding) nem rolagem —
// base para os elementos internos dos ícones desenhados à mão e containers
// puramente estruturais (não devem herdar a aparência do tema "default").
static lv_obj_t *bare_obj(lv_obj_t *parent) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

// Círculo vazado (somente contorno) — "anel" dos ícones de sol/lâmpada
static lv_obj_t *ring_create(lv_obj_t *parent, int32_t diameter, int32_t border_w, lv_color_t color) {
    lv_obj_t *obj = bare_obj(parent);
    lv_obj_set_size(obj, diameter, diameter);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, border_w, 0);
    lv_obj_set_style_border_color(obj, color, 0);
    return obj;
}

// Círculo preenchido — "ponto" central do ícone de sol
static lv_obj_t *disc_create(lv_obj_t *parent, int32_t diameter, lv_color_t color) {
    lv_obj_t *obj = bare_obj(parent);
    lv_obj_set_size(obj, diameter, diameter);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(obj, color, 0);
    return obj;
}

// Caixa-alta preservando UTF-8 (ASCII + Latin-1 Supplement) — suficiente
// para "Lâmpada"->"LÂMPADA", o único acento usado nas legendas em maiúsculas
// do wireframe (Label = text-transform: uppercase no CSS original).
static void label_set_text_upper(lv_obj_t *label, const char *src) {
    char buf[48];
    size_t o = 0;
    for (size_t i = 0; src[i] != '\0' && o + 2 < sizeof(buf);) {
        unsigned char c = (unsigned char)src[i];
        if (c < 0x80) {
            buf[o++] = (char)toupper(c);
            i += 1;
        } else if (c == 0xC3 && src[i + 1] != '\0') {
            unsigned char c2 = (unsigned char)src[i + 1];
            // à-þ (0xA0-0xBE), exceto ÷ (0xB7): segundo byte -0x20 -> À-Þ
            buf[o++] = (char)c;
            buf[o++] = (char)((c2 >= 0xA0 && c2 <= 0xBE && c2 != 0xB7) ? (c2 - 0x20) : c2);
            i += 2;
        } else {
            buf[o++] = (char)c;
            i += 1;
        }
    }
    buf[o] = '\0';
    lv_label_set_text(label, buf);
}

/* ============================================================================
 * Ícones desenhados à mão (sun / bulb — sem equivalente em LV_SYMBOL_*)
 * ========================================================================== */

lv_obj_t *ui_icon_sun_create(lv_obj_t *parent, int32_t size, lv_color_t color) {
    lv_obj_t *root = bare_obj(parent);
    lv_obj_set_size(root, size, size);

    int32_t border = (size >= 32) ? 2 : 1;

    // anel: inset 16% do contêiner -> diâmetro 68%, centralizado
    lv_obj_t *ring = ring_create(root, (int32_t)(size * 0.68f + 0.5f), border, color);
    lv_obj_center(ring);

    // ponto: inset 34% -> diâmetro 32%, preenchido e centralizado
    lv_obj_t *dot = disc_create(root, (int32_t)(size * 0.32f + 0.5f), color);
    lv_obj_center(dot);

    return root;
}

lv_obj_t *ui_icon_bulb_create(lv_obj_t *parent, int32_t size, lv_color_t color) {
    lv_obj_t *root = bare_obj(parent);
    lv_obj_set_size(root, size, size);

    int32_t border = (size >= 32) ? 2 : 1;

    // bulbo: anel de 64% posicionado a partir de (18%, 4%) do contêiner
    lv_obj_t *ring = ring_create(root, (int32_t)(size * 0.64f + 0.5f), border, color);
    lv_obj_set_pos(ring, (int32_t)(size * 0.18f + 0.5f), (int32_t)(size * 0.04f + 0.5f));

    // base: retângulo 32%x16%, ancorado no canto inferior-esquerdo (34%, 2%)
    lv_obj_t *base = bare_obj(root);
    lv_obj_set_size(base, (int32_t)(size * 0.32f + 0.5f), (int32_t)(size * 0.16f + 0.5f));
    lv_obj_set_style_radius(base, 2, 0);
    lv_obj_set_style_bg_opa(base, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(base, color, 0);
    lv_obj_align(base, LV_ALIGN_BOTTOM_LEFT,
                 (int32_t)(size * 0.34f + 0.5f), -(int32_t)(size * 0.02f + 0.5f));

    return root;
}

/* ============================================================================
 * Containers/labels básicos (Card / Label)
 * ========================================================================== */

lv_obj_t *ui_card_create(lv_obj_t *parent) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(card, ui_color_card(), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, ui_color_hairline(), 0);
    lv_obj_set_style_border_width(card, UI_HAIRLINE_W, 0);
    lv_obj_set_style_radius(card, UI_RADIUS_CARD, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    return card;
}

lv_obj_t *ui_caption_create(lv_obj_t *parent, const char *text) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(label, ui_color_muted(), 0);
    lv_obj_set_style_text_letter_space(label, 1, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_CLIP);
    label_set_text_upper(label, text);
    return label;
}

void ui_caption_set_text(lv_obj_t *label, const char *text) {
    label_set_text_upper(label, text);
}

lv_obj_t *ui_square_btn_create(lv_obj_t *parent, const char *symbol, bool accent) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(btn, 42, 42);
    lv_obj_set_style_radius(btn, UI_RADIUS_CARD, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    if (accent) {
        lv_obj_set_style_bg_color(btn, ui_color_accent(), 0);
        lv_obj_set_style_border_width(btn, 0, 0);
    } else {
        lv_obj_set_style_bg_color(btn, ui_color_card(), 0);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_set_style_border_color(btn, ui_color_ink(), 0);
    }

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, symbol);
    lv_obj_set_style_text_color(label, accent ? ui_color_white() : ui_color_ink(), 0);
    lv_obj_center(label);
    return btn;
}

/* ============================================================================
 * Cabeçalho de tela: chevron de voltar + título (colados à esquerda) + slot à direita
 * ========================================================================== */

void ui_screen_header_create(lv_obj_t *parent, const char *title, ui_screen_header_t *out) {
    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_remove_style_all(root);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(root, LV_PCT(100), 42);
    lv_obj_set_style_pad_left(root, 14, 0);
    lv_obj_set_style_pad_right(root, 14, 0);
    lv_obj_set_style_border_side(root, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(root, UI_HAIRLINE_W, 0);
    lv_obj_set_style_border_color(root, ui_color_hairline(), 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_ROW);
    // space-between com 2 grupos: [chevron+título] (juntos, gap 10) <-> [trailing]
    // — espelha o "marginLeft: auto" do wireframe (título cola no chevron, não centraliza)
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *left_group = bare_obj(root);
    lv_obj_set_size(left_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(left_group, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left_group, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(left_group, 10, 0);

    lv_obj_t *back = bare_obj(left_group);
    lv_obj_set_size(back, 28, 28);
    lv_obj_add_flag(back, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *back_icon = lv_label_create(back);
    lv_label_set_text(back_icon, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(back_icon, ui_color_ink(), 0);
    lv_obj_center(back_icon);

    lv_obj_t *title_label = lv_label_create(left_group);
    lv_obj_set_style_text_font(title_label, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(title_label, ui_color_ink(), 0);
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_MODE_CLIP);
    lv_label_set_text(title_label, title);

    lv_obj_t *trailing = bare_obj(root);
    lv_obj_set_size(trailing, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    out->root = root;
    out->back_btn = back;
    out->title_label = title_label;
    out->trailing = trailing;
}

/* ============================================================================
 * Stepper de intensidade: [−] NN% [+]
 * ========================================================================== */

void ui_stepper_create(lv_obj_t *parent, ui_stepper_t *out) {
    lv_obj_t *root = bare_obj(parent);
    lv_obj_set_size(root, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(root, 12, 0);

    lv_obj_t *btn_minus = lv_button_create(root);
    lv_obj_remove_style_all(btn_minus);
    lv_obj_set_size(btn_minus, 44, 44);
    lv_obj_set_style_radius(btn_minus, UI_RADIUS_CARD, 0);
    lv_obj_set_style_bg_color(btn_minus, ui_color_card(), 0);
    lv_obj_set_style_bg_opa(btn_minus, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_minus, 2, 0);
    lv_obj_set_style_border_color(btn_minus, ui_color_ink(), 0);
    lv_obj_t *minus_label = lv_label_create(btn_minus);
    lv_label_set_text(minus_label, LV_SYMBOL_MINUS);
    lv_obj_set_style_text_color(minus_label, ui_color_ink(), 0);
    lv_obj_center(minus_label);

    lv_obj_t *value_label = lv_label_create(root);
    lv_obj_set_style_text_font(value_label, UI_FONT_HERO, 0);
    lv_obj_set_style_text_color(value_label, ui_color_ink(), 0);
    lv_obj_set_style_text_align(value_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(value_label, 80);
    lv_label_set_long_mode(value_label, LV_LABEL_LONG_MODE_CLIP);
    lv_label_set_text(value_label, "--%");

    lv_obj_t *btn_plus = lv_button_create(root);
    lv_obj_remove_style_all(btn_plus);
    lv_obj_set_size(btn_plus, 44, 44);
    lv_obj_set_style_radius(btn_plus, UI_RADIUS_CARD, 0);
    lv_obj_set_style_bg_color(btn_plus, ui_color_accent(), 0);
    lv_obj_set_style_bg_opa(btn_plus, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn_plus, 0, 0);
    lv_obj_t *plus_label = lv_label_create(btn_plus);
    lv_label_set_text(plus_label, LV_SYMBOL_PLUS);
    lv_obj_set_style_text_color(plus_label, ui_color_white(), 0);
    lv_obj_center(plus_label);

    out->root = root;
    out->btn_minus = btn_minus;
    out->btn_plus = btn_plus;
    out->value_label = value_label;
}

void ui_stepper_set_value(const ui_stepper_t *stepper, int value_pct) {
    lv_label_set_text_fmt(stepper->value_label, "%d%%", value_pct);
}

/* ============================================================================
 * Botão de força: ícone + "Ligada"/"Desligada" — alterna preenchido/contorno
 * ========================================================================== */

void ui_power_btn_create(lv_obj_t *parent, ui_power_btn_t *out) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(btn, LV_PCT(100), 46);
    lv_obj_set_style_radius(btn, UI_RADIUS_CARD, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(btn, 8, 0);

    lv_obj_t *icon = lv_label_create(btn);
    lv_label_set_text(icon, LV_SYMBOL_POWER);

    lv_obj_t *label = lv_label_create(btn);
    lv_obj_set_style_text_font(label, UI_FONT_BODY, 0);
    lv_obj_set_style_text_letter_space(label, 1, 0);

    out->root = btn;
    out->icon = icon;
    out->label = label;

    ui_power_btn_set_on(out, false, "", "");
}

void ui_power_btn_set_on(const ui_power_btn_t *btn, bool on, const char *on_text, const char *off_text) {
    lv_color_t fg = on ? ui_color_white() : ui_color_ink();

    lv_obj_set_style_bg_color(btn->root, on ? ui_color_accent() : ui_color_card(), 0);
    lv_obj_set_style_border_width(btn->root, on ? 0 : 2, 0);
    lv_obj_set_style_border_color(btn->root, ui_color_ink(), 0);

    lv_obj_set_style_text_color(btn->icon, fg, 0);
    lv_obj_set_style_text_color(btn->label, fg, 0);
    lv_label_set_text(btn->label, on ? on_text : off_text);
}

/* ============================================================================
 * Segmentado de 2 opções: pílula com contorno, opção ativa em fundo sólido
 * (Seg do wireframe — único uso é o seletor de idioma PT/EN em Ajustes)
 * ========================================================================== */

static lv_obj_t *seg2_option_create(lv_obj_t *parent, const char *text, lv_obj_t **out_label) {
    lv_obj_t *opt = bare_obj(parent);
    lv_obj_set_size(opt, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(opt, 11, 0);
    lv_obj_set_style_pad_ver(opt, 4, 0);

    lv_obj_t *label = lv_label_create(opt);
    lv_obj_set_style_text_font(label, UI_FONT_BODY, 0);
    lv_obj_set_style_text_letter_space(label, 1, 0);
    lv_label_set_text(label, text);

    *out_label = label;
    return opt;
}

void ui_seg2_create(lv_obj_t *parent, const char *text_a, const char *text_b, ui_seg2_t *out) {
    lv_obj_t *root = bare_obj(parent);
    lv_obj_set_size(root, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_radius(root, UI_RADIUS_CARD, 0);
    lv_obj_set_style_clip_corner(root, true, 0);
    lv_obj_set_style_border_width(root, 2, 0);
    lv_obj_set_style_border_color(root, ui_color_ink(), 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_ROW);

    lv_obj_t *label_a = nullptr;
    lv_obj_t *label_b = nullptr;
    lv_obj_t *btn_a = seg2_option_create(root, text_a, &label_a);
    lv_obj_t *btn_b = seg2_option_create(root, text_b, &label_b);

    out->root = root;
    out->btn_a = btn_a;
    out->label_a = label_a;
    out->btn_b = btn_b;
    out->label_b = label_b;

    ui_seg2_set_active(out, true);
}

void ui_seg2_set_active(const ui_seg2_t *seg, bool a_active) {
    const struct { lv_obj_t *btn; lv_obj_t *label; bool active; } sides[2] = {
        { seg->btn_a, seg->label_a, a_active },
        { seg->btn_b, seg->label_b, !a_active },
    };
    for (int i = 0; i < 2; i++) {
        lv_obj_set_style_bg_opa(sides[i].btn, sides[i].active ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(sides[i].btn, ui_color_accent(), 0);
        lv_obj_set_style_text_color(sides[i].label, sides[i].active ? ui_color_white() : ui_color_ink(), 0);
    }
}
