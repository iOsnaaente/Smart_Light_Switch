#pragma once

#include "esp_err.h"

/**
 * @brief   Orquestra a interface local: monta as 3 telas (Principal, Modo
 *          Automático, Ajustes), liga cada toque ao comando correspondente
 *          do sistema (event_bus/app_modes) e mantém o que é exibido em
 *          sincronia com o estado real via assinaturas no event_bus
 *          (LDR/DIMMER/MODE/SETPOINT/NET_STATUS) — telas só desenham
 *          (ui_state.h), este módulo decide o que cada mudança significa.
 *
 * Pré-requisito: lvgl_port_init() concluído — a criação das telas já
 * depende do LVGL (lv_init/display/indev) estar pronto.
 */

#ifdef __cplusplus
extern "C" {
#endif

// Constrói as telas, registra os toques e assina o event_bus. Chame depois de
// lvgl_port_init()/app_modes_init()/event_bus_init() e ANTES de lvgl_port_start()
// (mesma ordem documentada em lvgl_port.h: telas prontas antes da task começar a desenhar).
esp_err_t ui_manager_init(void);

// Carrega a tela principal e sobe a task do LVGL (lvgl_port_start).
esp_err_t ui_manager_start(void);

#ifdef __cplusplus
}
#endif
