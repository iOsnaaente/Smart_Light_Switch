#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"

/**
 * @brief   Ponte entre o driver de painel (ili9340/XPT2046) e o LVGL: registra
 *          display + touch, converte/encaixota pixels para o barramento SPI e
 *          mantém o "coração" do LVGL batendo (tick + lv_timer_handler) numa
 *          task dedicada — para que ui_manager/telas só falem a língua do LVGL.
 */

#ifdef __cplusplus
extern "C" {
#endif

// Inicializa painel + LVGL (display, buffer, indev de toque). Chame ANTES de
// construir qualquer tela — lv_obj_create já exige lv_init concluído.
esp_err_t lvgl_port_init(void);

// Sobe a task periódica (tick + lv_timer_handler/render/entrada). Chame DEPOIS
// que as telas iniciais já estiverem montadas, para a task não desenhar uma UI
// pela metade enquanto ui_manager ainda está construindo os widgets.
esp_err_t lvgl_port_start(void);

// API do LVGL não é thread-safe: qualquer chamada lv_* feita fora da task do
// LVGL (ex.: handlers do event_bus atualizando texto/estado de widgets) deve
// vir cercada por lock/unlock. timeout_ms == 0 espera indefinidamente.
bool lvgl_port_lock(uint32_t timeout_ms);
void lvgl_port_unlock(void);

#ifdef __cplusplus
}
#endif
