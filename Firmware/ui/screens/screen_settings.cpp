#include "screen_settings.h"
#include "../ui_theme.h"
#include "../board_config.h"

/* ============================================================================
 * Helpers internos
 * ========================================================================== */

static lv_obj_t *bare_obj(lv_obj_t *parent) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

/**
 * @brief   Linha de ajuste — espelha o helper Row() do wireframe: altura 46,
 *          hairline inferior (exceto a última) e 2 grupos [ícone+rótulo] <->
 *          [conteúdo extra] separados por space-between (= "marginLeft:auto").
 *          Devolve os 3 containers para o chamador montar cada lado e expor
 *          a linha inteira como alvo de toque, quando fizer sentido.
 */
static void row_create(lv_obj_t *parent, bool last, lv_obj_t **out_row, lv_obj_t **out_lead, lv_obj_t **out_right) {
    lv_obj_t *row = bare_obj(parent);
    lv_obj_set_size(row, LV_PCT(100), 46);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    if (!last) {
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_width(row, UI_HAIRLINE_W, 0);
        lv_obj_set_style_border_color(row, ui_color_hairline(), 0);
    }

    lv_obj_t *lead = bare_obj(row);
    lv_obj_set_size(lead, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(lead, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(lead, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(lead, 11, 0);

    lv_obj_t *right = bare_obj(row);
    lv_obj_set_size(right, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(right, 7, 0);

    *out_row = row;
    *out_lead = lead;
    *out_right = right;
}

// Ícone via glifo nativo (LV_SYMBOL_*) em cor muted — usado para "wifi"/"gear",
// que não têm desenho próprio como sun/bulb (ui_icon_*_create).
static lv_obj_t *row_glyph_icon_create(lv_obj_t *parent, const char *symbol) {
    lv_obj_t *icon = lv_label_create(parent);
    lv_obj_set_style_text_color(icon, ui_color_muted(), 0);
    lv_label_set_text(icon, symbol);
    return icon;
}

// Rótulo da linha — texto simples (não é Label/legenda: sem versalete, cor ink)
static lv_obj_t *row_label_create(lv_obj_t *parent, const char *text) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(label, ui_color_ink(), 0);
    lv_label_set_text(label, text);
    return label;
}

// Valor à direita — texto pequeno muted (ip/versão), espelha o <span> do "right"
static lv_obj_t *row_value_create(lv_obj_t *parent, const char *text) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(label, ui_color_muted(), 0);
    lv_label_set_text(label, text);
    return label;
}

static lv_obj_t *row_chevron_create(lv_obj_t *parent) {
    lv_obj_t *chevron = lv_label_create(parent);
    lv_obj_set_style_text_color(chevron, ui_color_muted(), 0);
    lv_label_set_text(chevron, LV_SYMBOL_RIGHT);
    return chevron;
}

/* ============================================================================
 * Criação
 * ========================================================================== */

void screen_settings_create(screen_settings_t *out) {
    const ui_strings_t *s = ui_theme_strings(UI_LANG_PT);

    lv_obj_t *root = lv_obj_create(nullptr);
    lv_obj_remove_style_all(root);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(root, LCD_WIDTH, LCD_HEIGHT);
    lv_obj_set_style_bg_color(root, ui_color_paper(), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(root, ui_color_ink(), 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    /* ---- Cabeçalho: voltar + título + engrenagem (decorativa) ----------- */
    ui_screen_header_t header;
    ui_screen_header_create(root, s->settings, &header);
    row_glyph_icon_create(header.trailing, LV_SYMBOL_SETTINGS);

    /* ---- Corpo: lista de linhas de ajuste -------------------------------- */
    lv_obj_t *body = bare_obj(root);
    lv_obj_set_width(body, LV_PCT(100));
    lv_obj_set_flex_grow(body, 1);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_top(body, 2, 0);
    lv_obj_set_style_pad_left(body, 14, 0);
    lv_obj_set_style_pad_right(body, 14, 0);

    // Idioma — troca PT<->EN ao tocar no segmento; o wireframe reaproveita o
    // glifo de wifi como ícone desta linha (não há um ícone de "idioma" no
    // conjunto original), reproduzido aqui tal como especificado.
    lv_obj_t *lang_row, *lang_lead, *lang_right;
    row_create(body, false, &lang_row, &lang_lead, &lang_right);
    row_glyph_icon_create(lang_lead, LV_SYMBOL_WIFI);
    lv_obj_t *language_label = row_label_create(lang_lead, s->language);
    ui_seg2_t language_seg;
    ui_seg2_create(lang_right, "PT", "EN", &language_seg);

    // Brilho da tela — barrinha de progresso somente leitura (espelha o
    // indicador estático do wireframe; preenchimento real vem de _update)
    lv_obj_t *bright_row, *bright_lead, *bright_right;
    row_create(body, false, &bright_row, &bright_lead, &bright_right);
    ui_icon_sun_create(bright_lead, 16, ui_color_muted());
    lv_obj_t *screen_bright_label = row_label_create(bright_lead, s->screen_bright);

    lv_obj_t *bright_track = bare_obj(bright_right);
    lv_obj_set_size(bright_track, 58, 6);
    lv_obj_set_style_radius(bright_track, 3, 0);
    lv_obj_set_style_clip_corner(bright_track, true, 0);
    lv_obj_set_style_bg_color(bright_track, ui_color_track(), 0);
    lv_obj_set_style_bg_opa(bright_track, LV_OPA_COVER, 0);

    lv_obj_t *screen_bright_fill = bare_obj(bright_track);
    lv_obj_set_size(screen_bright_fill, LV_PCT(0), LV_PCT(100));
    lv_obj_set_style_radius(screen_bright_fill, 3, 0);
    lv_obj_set_style_bg_color(screen_bright_fill, ui_color_accent(), 0);
    lv_obj_set_style_bg_opa(screen_bright_fill, LV_OPA_COVER, 0);

    // Calibrar sensor — linha inteira tocável, dispara a rotina de calibração
    lv_obj_t *calib_row, *calib_lead, *calib_right;
    row_create(body, false, &calib_row, &calib_lead, &calib_right);
    ui_icon_bulb_create(calib_lead, 16, ui_color_muted());
    lv_obj_t *calib_label = row_label_create(calib_lead, s->calib);
    row_chevron_create(calib_right);

    // Wi-Fi — mostra o IP atual (ou "—" desconectado); linha inteira tocável
    lv_obj_t *wifi_row, *wifi_lead, *wifi_right;
    row_create(body, false, &wifi_row, &wifi_lead, &wifi_right);
    row_glyph_icon_create(wifi_lead, LV_SYMBOL_WIFI);
    lv_obj_t *wifi_label = row_label_create(wifi_lead, s->wifi);
    lv_obj_t *wifi_value_label = row_value_create(wifi_right, "—");
    row_chevron_create(wifi_right);

    // Potência — mesma leitura (event_bus POWER_UPDATE) que alimenta a
    // telemetria MQTT do backend; só exibição, sem toque.
    lv_obj_t *power_row, *power_lead, *power_right;
    row_create(body, false, &power_row, &power_lead, &power_right);
    ui_icon_bulb_create(power_lead, 16, ui_color_muted());
    lv_obj_t *power_label = row_label_create(power_lead, s->power);
    lv_obj_t *power_value_label = row_value_create(power_right, "0.0 W");

    // Sobre — versão fixa do firmware; última linha (sem hairline / "last")
    lv_obj_t *about_row, *about_lead, *about_right;
    row_create(body, true, &about_row, &about_lead, &about_right);
    row_glyph_icon_create(about_lead, LV_SYMBOL_SETTINGS);
    lv_obj_t *about_label = row_label_create(about_lead, s->about);
    row_value_create(about_right, "v1.0.3");

    out->root                = root;
    out->header              = header;
    out->language_label      = language_label;
    out->language_seg        = language_seg;
    out->screen_bright_label = screen_bright_label;
    out->screen_bright_fill  = screen_bright_fill;
    out->calib_label         = calib_label;
    out->calib_row           = calib_row;
    out->wifi_label          = wifi_label;
    out->wifi_value_label    = wifi_value_label;
    out->wifi_row            = wifi_row;
    out->power_label         = power_label;
    out->power_value_label   = power_value_label;
    out->about_label         = about_label;
}

/* ============================================================================
 * Atualização a partir do estado (ui_runtime_state_t)
 * ========================================================================== */

void screen_settings_update(const screen_settings_t *screen, const ui_runtime_state_t *state) {
    const ui_strings_t *s = ui_theme_strings(state->lang);

    lv_label_set_text(screen->header.title_label, s->settings);

    lv_label_set_text(screen->language_label, s->language);
    ui_seg2_set_active(&screen->language_seg, state->lang == UI_LANG_PT);

    lv_label_set_text(screen->screen_bright_label, s->screen_bright);
    lv_obj_set_width(screen->screen_bright_fill, LV_PCT(state->screen_brightness_pct));

    lv_label_set_text(screen->calib_label, s->calib);

    lv_label_set_text(screen->wifi_label, s->wifi);
    if (state->ble_pairing_active) {
        lv_label_set_text(screen->wifi_value_label, s->pairing);
    } else {
        lv_label_set_text(screen->wifi_value_label, state->wifi_connected ? state->ip_addr : s->disconnected);
    }

    lv_label_set_text(screen->power_label, s->power);
    lv_label_set_text_fmt(screen->power_value_label, "%.1f W", state->active_power_w);

    lv_label_set_text(screen->about_label, s->about);
}
