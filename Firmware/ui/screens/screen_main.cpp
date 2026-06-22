#include "screen_main.h"
#include "../ui_theme.h"
#include "../board_config.h"

/* ============================================================================
 * Paleta específica do design "superfície de luz" (MainPage.html)
 * ========================================================================== */

static inline lv_color_t col_off_bg()       { return lv_color_hex(0x100D09); }
static inline lv_color_t col_amber_top()    { return lv_color_hex(0xFFE9B8); }
static inline lv_color_t col_amber_bot()    { return lv_color_hex(0x7E4A12); }
static inline lv_color_t col_blue_top()     { return lv_color_hex(0xCFE6FF); }
static inline lv_color_t col_blue_bot()     { return lv_color_hex(0x15335C); }
static inline lv_color_t col_glow_top()     { return lv_color_hex(0xFFECC8); }
static inline lv_color_t col_ink_light()    { return lv_color_hex(0xFFF6E6); } // texto sobre fundo escuro
static inline lv_color_t col_ink_dark()     { return lv_color_hex(0x2A1C08); } // texto sobre fundo âmbar claro
static inline lv_color_t col_chip_bg()      { return lv_color_hex(0x0E0B07); }
static inline lv_color_t col_chip_border()  { return lv_color_hex(0x3A2E1E); }
static inline lv_color_t col_drive_tag_bg() { return lv_color_hex(0x122240); }
static inline lv_color_t col_drive_tag_fg() { return lv_color_hex(0xCFE3FF); }
static inline lv_color_t col_green_dot()    { return lv_color_hex(0x5BD08C); }
static inline lv_color_t col_marker_light() { return lv_color_hex(0xFFFFFF); }
static inline lv_color_t col_marker_drive() { return lv_color_hex(0xA0C8FF); }

static constexpr int32_t TOPBAR_H   = 54;
static constexpr int32_t CHIP_H     = 34;
static constexpr int32_t MARKER_H   = 2;
static constexpr int32_t GLOW_H     = LCD_HEIGHT * 40 / 100;

/* ============================================================================
 * Helpers internos
 * ========================================================================== */

static lv_obj_t *bare(lv_obj_t *parent) {
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    // Remove SCROLLABLE e CLICKABLE: esses objetos são puramente visuais.
    // Sem isso, o LVGL roteia o toque para o filho mais próximo CLICKABLE,
    // que absorve o evento — ele nunca chega ao s_main.root, onde ficam os
    // callbacks on_main_pressed / on_main_pressing que atualizam a tela.
    // O cfg_btn re-adiciona CLICKABLE explicitamente após chip_create().
    lv_obj_remove_flag(o, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
    return o;
}

/* Cria uma camada de gradiente (container que clippa seu paint filho). */
static lv_obj_t *layer_create(lv_obj_t *parent) {
    lv_obj_t *l = bare(parent);
    lv_obj_add_flag(l, LV_OBJ_FLAG_IGNORE_LAYOUT);
    /* Por padrão o LVGL clippa filhos aos limites do pai — é exatamente o que
     * queremos: o paint tem altura LCD_HEIGHT mas só a porção dentro da layer
     * fica visível, revelando o gradiente de baixo para cima. */
    return l;
}

/* Cria o paint (gradiente de altura total) dentro de uma layer. */
static lv_obj_t *paint_create(lv_obj_t *layer, lv_color_t top, lv_color_t bot) {
    lv_obj_t *p = bare(layer);
    lv_obj_add_flag(p, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(p, LCD_WIDTH, LCD_HEIGHT);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(p, top, 0);
    lv_obj_set_style_bg_grad_color(p, bot, 0);
    lv_obj_set_style_bg_grad_dir(p, LV_GRAD_DIR_VER, 0);
    return p;
}

/* Chip arredondado semi-transparente da barra superior. */
static lv_obj_t *chip_create(lv_obj_t *parent, int32_t w) {
    lv_obj_t *c = bare(parent);
    lv_obj_set_size(c, w, CHIP_H);
    lv_obj_set_style_radius(c, CHIP_H / 2, 0);
    lv_obj_set_style_bg_opa(c, LV_OPA_70, 0);
    lv_obj_set_style_bg_color(c, col_chip_bg(), 0);
    lv_obj_set_style_border_width(c, 1, 0);
    lv_obj_set_style_border_color(c, col_chip_border(), 0);
    lv_obj_set_style_border_opa(c, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(c, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(c, 6, 0);
    lv_obj_set_style_pad_hor(c, 10, 0);
    return c;
}


/* ============================================================================
 * Criação
 * ========================================================================== */

void screen_main_create(screen_main_t *out) {
    /* ---- raiz: fundo escuro uniforme (off state) -------------------------- */
    lv_obj_t *root = lv_obj_create(nullptr);
    lv_obj_remove_style_all(root);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(root, LCD_WIDTH, LCD_HEIGHT);
    lv_obj_set_style_bg_color(root, col_off_bg(), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);

    /* ---- camada de acionamento (azul) — fica atrás da âmbar -------------- */
    lv_obj_t *drive_layer = layer_create(root);
    lv_obj_set_size(drive_layer, LCD_WIDTH, 0);           // começa em 0; atualizado em _update
    lv_obj_set_pos(drive_layer, 0, LCD_HEIGHT);
    lv_obj_t *drive_paint = paint_create(drive_layer, col_blue_top(), col_blue_bot());
    lv_obj_set_pos(drive_paint, 0, 0);                    // pos ajustada em _update

    /* ---- camada de luz (âmbar) — em cima da azul ------------------------- */
    lv_obj_t *light_layer = layer_create(root);
    lv_obj_set_size(light_layer, LCD_WIDTH, 0);
    lv_obj_set_pos(light_layer, 0, LCD_HEIGHT);
    lv_obj_t *light_paint = paint_create(light_layer, col_amber_top(), col_amber_bot());
    lv_obj_set_pos(light_paint, 0, 0);

    /* ---- bloom no topo (glow) ------------------------------------------- */
    lv_obj_t *glow = bare(root);
    lv_obj_add_flag(glow, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(glow, LCD_WIDTH, GLOW_H);
    lv_obj_set_pos(glow, 0, 0);
    lv_obj_set_style_bg_opa(glow, LV_OPA_TRANSP, 0);     // animado em _update
    lv_obj_set_style_bg_color(glow, col_glow_top(), 0);
    lv_obj_set_style_bg_grad_color(glow, col_off_bg(), 0);
    lv_obj_set_style_bg_grad_dir(glow, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(glow, 0, 0);

    /* ---- marcador de acionamento (linha azul + pill à direita) ------------ */
    lv_obj_t *drive_marker = bare(root);
    lv_obj_add_flag(drive_marker, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(drive_marker, LCD_WIDTH, CHIP_H);
    lv_obj_set_pos(drive_marker, 0, 0);

    /* linha horizontal centrada verticalmente */
    lv_obj_t *drive_line = bare(drive_marker);
    lv_obj_add_flag(drive_line, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(drive_line, LCD_WIDTH, MARKER_H);
    lv_obj_set_pos(drive_line, 0, (CHIP_H - MARKER_H) / 2);
    lv_obj_set_style_bg_opa(drive_line, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(drive_line, col_marker_drive(), 0);

    /* pill à direita */
    lv_obj_t *drive_pill = bare(drive_marker);
    lv_obj_add_flag(drive_pill, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(drive_pill, LV_SIZE_CONTENT, CHIP_H - 10);
    lv_obj_set_style_radius(drive_pill, (CHIP_H - 10) / 2, 0);
    lv_obj_set_style_bg_opa(drive_pill, LV_OPA_80, 0);
    lv_obj_set_style_bg_color(drive_pill, col_drive_tag_bg(), 0);
    lv_obj_set_style_pad_hor(drive_pill, 8, 0);
    lv_obj_set_style_pad_ver(drive_pill, 3, 0);
    lv_obj_set_style_border_width(drive_pill, 1, 0);
    lv_obj_set_style_border_color(drive_pill, col_marker_drive(), 0);
    lv_obj_set_style_border_opa(drive_pill, LV_OPA_40, 0);
    lv_obj_align(drive_pill, LV_ALIGN_RIGHT_MID, -14, 0);

    lv_obj_t *drive_tag = lv_label_create(drive_pill);
    lv_obj_set_style_text_font(drive_tag, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(drive_tag, col_drive_tag_fg(), 0);
    lv_label_set_text(drive_tag, "MV 0 lx");

    /* ---- marcador de luz (linha branca + pill à esquerda + grip à direita) */
    lv_obj_t *light_marker = bare(root);
    lv_obj_add_flag(light_marker, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(light_marker, LCD_WIDTH, CHIP_H);
    lv_obj_set_pos(light_marker, 0, LCD_HEIGHT);

    lv_obj_t *light_line = bare(light_marker);
    lv_obj_add_flag(light_line, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(light_line, LCD_WIDTH, MARKER_H);
    lv_obj_set_pos(light_line, 0, (CHIP_H - MARKER_H) / 2);
    lv_obj_set_style_bg_opa(light_line, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(light_line, col_marker_light(), 0);

    /* pill à esquerda */
    lv_obj_t *light_pill = bare(light_marker);
    lv_obj_add_flag(light_pill, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(light_pill, LV_SIZE_CONTENT, CHIP_H - 10);
    lv_obj_set_style_radius(light_pill, (CHIP_H - 10) / 2, 0);
    lv_obj_set_style_bg_opa(light_pill, LV_OPA_90, 0);
    lv_obj_set_style_bg_color(light_pill, lv_color_white(), 0);
    lv_obj_set_style_pad_hor(light_pill, 8, 0);
    lv_obj_set_style_pad_ver(light_pill, 3, 0);
    lv_obj_align(light_pill, LV_ALIGN_LEFT_MID, 14, 0);
    lv_obj_t *light_tag_label = lv_label_create(light_pill);
    lv_obj_set_style_text_font(light_tag_label, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(light_tag_label, lv_color_hex(0x92591A), 0);
    lv_label_set_text(light_tag_label, "Luz");

    /* grip à direita (barra arredondada) */
    lv_obj_t *grip = bare(light_marker);
    lv_obj_add_flag(grip, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(grip, 38, 5);
    lv_obj_set_style_radius(grip, 3, 0);
    lv_obj_set_style_bg_opa(grip, LV_OPA_90, 0);
    lv_obj_set_style_bg_color(grip, lv_color_white(), 0);
    lv_obj_align(grip, LV_ALIGN_RIGHT_MID, -14, 0);

    /* ---- readout central -------------------------------------------------- */
    lv_obj_t *readout = bare(root);
    lv_obj_add_flag(readout, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(readout, LCD_WIDTH, LV_SIZE_CONTENT);
    lv_obj_align(readout, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_flex_flow(readout, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(readout, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(readout, 6, 0);

    lv_obj_t *kicker = lv_label_create(readout);
    lv_obj_set_style_text_font(kicker, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(kicker, col_ink_light(), 0);
    lv_obj_set_style_text_letter_space(kicker, 2, 0);
    lv_label_set_text(kicker, "SETPOINT");

    lv_obj_t *val_row = bare(readout);
    lv_obj_set_size(val_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(val_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(val_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(val_row, 2, 0);

    lv_obj_t *big_val = lv_label_create(val_row);
    lv_obj_set_style_text_font(big_val, UI_FONT_HERO, 0);
    lv_obj_set_style_text_color(big_val, col_ink_light(), 0);
    lv_label_set_text(big_val, "0");

    lv_obj_t *big_pct = lv_label_create(val_row);
    lv_obj_set_style_text_font(big_pct, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(big_pct, col_ink_light(), 0);
    lv_label_set_text(big_pct, "lx");

    lv_obj_t *lux_val = lv_label_create(readout);
    lv_obj_set_style_text_font(lux_val, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(lux_val, col_ink_light(), 0);
    lv_label_set_text(lux_val, "MV: 0 lx");

    /* ---- dica de arraste no rodapé --------------------------------------- */
    lv_obj_t *hint = lv_label_create(root);
    lv_obj_add_flag(hint, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_style_text_font(hint, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(hint, col_ink_light(), 0);
    lv_obj_set_style_opa(hint, LV_OPA_50, 0);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
    // lv_label_set_text(hint, "arraste para ajustar a intensidade");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -18);

    /* ---- barra superior (criada por último → z-order mais alto) ----------- */
    lv_obj_t *topbar = bare(root);
    lv_obj_add_flag(topbar, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_set_size(topbar, LCD_WIDTH, TOPBAR_H);
    lv_obj_set_pos(topbar, 0, 0);
    lv_obj_set_flex_flow(topbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(topbar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(topbar, 14, 0);
    lv_obj_set_style_pad_ver(topbar, 10, 0);

    /* chip Wi-Fi (esquerda) */
    lv_obj_t *wifi_chip = chip_create(topbar, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(wifi_chip, 10, 0);

    lv_obj_t *wifi_dot = bare(wifi_chip);
    lv_obj_set_size(wifi_dot, 8, 8);
    lv_obj_set_style_radius(wifi_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(wifi_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(wifi_dot, col_green_dot(), 0);

    lv_obj_t *wifi_icon = lv_label_create(wifi_chip);
    lv_obj_set_style_text_color(wifi_icon, col_ink_light(), 0);
    lv_obj_set_style_text_font(wifi_icon, UI_FONT_BODY, 0);
    lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);

    /* chip relógio (centro) */
    lv_obj_t *clock_chip = chip_create(topbar, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(clock_chip, 12, 0);
    lv_obj_t *clock_label = lv_label_create(clock_chip);
    lv_obj_set_style_text_font(clock_label, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(clock_label, col_ink_light(), 0);
    lv_label_set_text(clock_label, "--:--");

    /* chip de configurações (direita) — tocável → Ajustes */
    lv_obj_t *cfg_btn = chip_create(topbar, CHIP_H);
    lv_obj_add_flag(cfg_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_pad_hor(cfg_btn, 0, 0);
    lv_obj_t *gear_icon = lv_label_create(cfg_btn);
    lv_obj_set_style_text_font(gear_icon, UI_FONT_BODY, 0);
    lv_obj_set_style_text_color(gear_icon, col_ink_light(), 0);
    lv_label_set_text(gear_icon, LV_SYMBOL_SETTINGS);

    /* ---- preenche a struct de saída --------------------------------------- */
    out->root         = root;
    out->drive_layer  = drive_layer;
    out->drive_paint  = drive_paint;
    out->light_layer  = light_layer;
    out->light_paint  = light_paint;
    out->glow         = glow;
    out->wifi_dot     = wifi_dot;
    out->clock_label  = clock_label;
    out->cfg_btn      = cfg_btn;
    out->drive_marker = drive_marker;
    out->drive_tag    = drive_tag;
    out->light_marker = light_marker;
    out->readout_kicker = kicker;
    out->big_val      = big_val;
    out->big_pct      = big_pct;
    out->lux_val      = lux_val;
}

/* ============================================================================
 * Atualização a partir do estado
 * ========================================================================== */

void screen_main_update(const screen_main_t *s, const ui_runtime_state_t *state) {
    /* SP = setpoint em lux (controlado pelo gesto vertical)
     * MV = medido pelo sensor LDR (natural_lux do event_bus) */
    const int sp_lux = (int)state->target_lux;
    const int mv_lux = (int)state->natural_lux;

    /* Normaliza para 0-100% da escala de lux */
    const int sp_pct = (int)(state->target_lux * 100.0f / UI_AMBIENT_LUX_SCALE);
    const int mv_pct = (int)(state->natural_lux * 100.0f / UI_AMBIENT_LUX_SCALE);

    /* Alturas das camadas em pixels:
     * âmbar (light_layer) = SP — "o que você quer"
     * azul  (drive_layer) = MV — "o que o sensor mede" */
    const int32_t light_h = (int32_t)sp_pct * LCD_HEIGHT / 100;
    const int32_t drive_h = (int32_t)mv_pct * LCD_HEIGHT / 100;

    /* Reposiciona e redimensiona as camadas (revelação de baixo para cima). */
    lv_obj_set_pos(s->light_layer, 0, LCD_HEIGHT - light_h);
    lv_obj_set_size(s->light_layer, LCD_WIDTH, light_h);
    lv_obj_set_pos(s->light_paint, 0, light_h - LCD_HEIGHT);

    lv_obj_set_pos(s->drive_layer, 0, LCD_HEIGHT - drive_h);
    lv_obj_set_size(s->drive_layer, LCD_WIDTH, drive_h);
    lv_obj_set_pos(s->drive_paint, 0, drive_h - LCD_HEIGHT);

    /* Glow: opacidade proporcional ao SP */
    lv_obj_set_style_opa(s->glow, (lv_opa_t)(sp_pct * LV_OPA_90 / 100), 0);

    /* Marcador SP (âmbar) e marcador MV (azul) */
    int32_t light_marker_y = LCD_HEIGHT - light_h - CHIP_H / 2;
    int32_t drive_marker_y = LCD_HEIGHT - drive_h - CHIP_H / 2;
    lv_obj_set_pos(s->light_marker, 0, light_marker_y);
    lv_obj_set_pos(s->drive_marker, 0, drive_marker_y);

    /* Marcador MV: visível quando MV difere do SP por mais de 5% */
    int diff = mv_pct - sp_pct;
    bool mv_marker_visible = (diff > 5 || diff < -5);
    if (mv_marker_visible) {
        lv_obj_remove_flag(s->drive_marker, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s->drive_marker, LV_OBJ_FLAG_HIDDEN);
    }
    lv_label_set_text_fmt(s->drive_tag, "MV %d lx", mv_lux);

    /* Readout central: texto escuro quando SP cobre a metade da tela (≥50%) */
    lv_color_t txt = (sp_pct >= 50) ? col_ink_dark() : col_ink_light();
    lv_obj_set_style_text_color(s->readout_kicker, txt, 0);
    lv_obj_set_style_text_color(s->big_val, txt, 0);
    lv_obj_set_style_text_color(s->big_pct, txt, 0);
    lv_obj_set_style_text_color(s->lux_val, txt, 0);

    lv_label_set_text_fmt(s->big_val, "%d", sp_lux);
    lv_label_set_text_fmt(s->lux_val, "MV: %d lx", mv_lux);

    /* Dot de Wi-Fi */
    if (state->wifi_connected) {
        lv_obj_remove_flag(s->wifi_dot, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s->wifi_dot, LV_OBJ_FLAG_HIDDEN);
    }
}
