#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "app_modes.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Idioma atual da interface (alternável na tela de Ajustes)
 */
typedef enum {
    UI_LANG_PT = 0,
    UI_LANG_EN,
} ui_lang_t;

/**
 * @brief   Modelo de dados exibido pelas telas — atualizado pelo ui_manager
 *          a partir dos eventos do event_bus e repassado a cada tela via
 *          *_update(). Mantém as telas "burras": elas só desenham o estado,
 *          quem decide o que ele significa é o ui_manager.
 */
typedef struct {
    app_mode_t mode;                 // MANUAL / AUTOMATIC / ECO (event_bus: MODE_CHANGED)
    bool       relay_on;             // lâmpada ligada/desligada (event_bus: DIMMER_UPDATE/RELAY_COMMAND)
    uint8_t    brightness_pct;       // intensidade atual 0-100% (event_bus: DIMMER_UPDATE)
    float      natural_lux;          // estimativa de luz ambiente (event_bus: LDR_UPDATE)
    float      target_lux;           // alvo configurado no modo automático (local + SETPOINT_CHANGED)
    bool       wifi_connected;       // status de rede (event_bus: NET_STATUS)
    char       ip_addr[16];          // IP atual, exibido em Ajustes > Wi-Fi
    uint8_t    sensitivity_pct;      // sensibilidade do modo automático (estado local da UI)
    uint8_t    screen_brightness_pct;// indicador de brilho da tela (estado local da UI)
    ui_lang_t  lang;                 // idioma selecionado
} ui_runtime_state_t;

#ifdef __cplusplus
}
#endif
