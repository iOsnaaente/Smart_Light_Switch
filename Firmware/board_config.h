#pragma once 

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "esp_lcd_panel_io.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/task.h"


// Ajuste os pinos de acordo com sua placa/display.
static constexpr gpio_num_t PIN_NUM_MOSI            = GPIO_NUM_6;
static constexpr gpio_num_t PIN_NUM_MISO            = GPIO_NUM_5;
static constexpr gpio_num_t PIN_NUM_CLK             = GPIO_NUM_4;
static constexpr gpio_num_t PIN_NUM_CS              = GPIO_NUM_7;
static constexpr gpio_num_t PIN_NUM_DC              = GPIO_NUM_8;
static constexpr gpio_num_t PIN_NUM_RST             = GPIO_NUM_9;
static constexpr gpio_num_t PIN_NUM_T_CS 		  	= GPIO_NUM_2;
static constexpr gpio_num_t PIN_NUM_T_IRQ           = GPIO_NUM_3;

static constexpr gpio_num_t PIN_NUM_ZEROCROSS       = GPIO_NUM_1;
static constexpr gpio_num_t PIN_NUM_SENSOR_LDR      = GPIO_NUM_0;
static constexpr gpio_num_t PIN_NUM_GATE_TRIAC      = GPIO_NUM_10;

static constexpr gpio_num_t PIN_NUM_SENSOR_RXD      = GPIO_NUM_21;
static constexpr gpio_num_t PIN_NUM_SENSOR_TXD      = GPIO_NUM_20;

// Botão de reset de provisionamento. Mantido pressionado (nível baixo, com
// pull-up interno) durante o boot -> apaga credenciais Wi-Fi da NVS e reabre
// o pareamento BLE. GPIO9 é o "BOOT button" na maioria das devkits ESP32-C3,
// mas aqui já está ocupado por PIN_NUM_RST (display); usamos GPIO18 (livre
// no restante do mapeamento) — ajuste conforme a fiação da sua placa.
static constexpr gpio_num_t PIN_NUM_PROVISION_RESET = GPIO_NUM_18;


// SPI - Bus Display TFT 
static constexpr spi_host_device_t LCD_SPI_HOST     = SPI2_HOST;

// SPI - Dimensões Display TFT 
static constexpr uint32_t LCD_PIXEL_CLOCK_HZ = 40 * 1000 * 1000;
static constexpr uint32_t TOUCH_SPI_CLOCK_HZ = 2 * 1000 * 1000;

// SPI - Modelo Display TFT 
static constexpr uint16_t TFT_MODEL = 0x9341;

/* [DEBUG] Voltou para 240x320 (retrato) — testar 320x240 confirmou que o
 * painel físico é nativo PAISAGEM sob MADCTL=0x08 (preenche certinho, sem
 * corte). Como o usuário monta/usa o painel em pé (retrato), a correção
 * certa não é mexer nessas dimensões — é girar o ENDEREÇAMENTO em hardware
 * via MADCTL (bit MV, registrador 0x36 em ili9340.c), mantendo aqui as
 * dimensões LÓGICAS que a UI espera (240 col x 320 lin). */
static constexpr int LCD_WIDTH  = 240;
static constexpr int LCD_HEIGHT = 320;
static constexpr int CAL_MARGIN = 0;
static constexpr int CAL_CROSS  = 0;

// Touch XPT2046 - faixa bruta de leitura do ADC (12 bits). Não existe rotina
// de calibração implementada (campos _min_xp/_max_xp etc. de TFT_t ficam sem
// uso). [DEBUG] Recalibrado a partir de um teste de 4 quinas:
//     sup-esq=(150,100)  sup-dir=(950,100)
//     inf-esq=(150,1150) inf-dir=(1140,1150)
// Cruzando com a posição lógica de cada canto (sup-esq=(0,0), sup-dir=(239,0),
// inf-esq=(0,319), inf-dir=(239,319)): bruto X SOBE conforme a tela X sobe
// (150 nos cantos X=0, 950-1140 nos X=239 -> NÃO precisa INVERT_X) e bruto Y
// SOBE conforme a tela Y sobe (100 no topo, 1150 no fundo -> sem invert Y).
// X_MAX usa o mínimo observado nos cantos direitos (950 vs 1140) para garantir
// que QUALQUER toque na borda direita alcance x=239 via clamping — a
// não-linearidade da película faz o fundo-dir chegar mais longe (1140), e o
// clamping absorve esse excesso sem criar zona morta no topo-dir.
static constexpr int TOUCH_RAW_X_MIN = 0; // 150;   // bruto X nos cantos onde tela X = 0 (borda esquerda)
static constexpr int TOUCH_RAW_X_MAX = 1000; // 950;   // mínimo bruto X observado na borda direita (tela X = 239)
static constexpr int TOUCH_RAW_Y_MIN = 0; // 100;   // bruto Y nos cantos onde tela Y = 0 (borda superior)
static constexpr int TOUCH_RAW_Y_MAX = 1250; // 1150;  // bruto Y nos cantos onde tela Y = 319 (borda inferior)

// [DEBUG] Filtro de "toque fantasma": com o painel em repouso (sem encostar),
// o XPT2046 ficou relatando IRQ ativo e leituras BRUTAS estáveis perto de
// (94,100) continuamente — sintoma de pressão mecânica da película resistiva
// contra o vidro (moldura/bezel apertando), não ruído elétrico aleatório.
// Esses falsos toques mapeavam para tela=(0,0) e o LVGL os via como
// "pressionado" sem parar — provável causa dos botões não responderem.
// Toques de verdade sempre vieram com bruto X >= ~150. Limiar em 120 (era 130)
// abre mais margem para toques reais na borda esquerda (xp=121..149 mapeia
// para x<0 → clampado a 0) sem risco de aceitar o fantasma (~94, gap=26).
static constexpr int TOUCH_RAW_NOISE_MAX = 120;

// [DEBUG] Eixo do XPT2046 é fixo À PELÍCULA FÍSICA — não acompanha o MADCTL
// do ILI9341 (ili9340.c). A orientação atual do painel (MADCTL=0x08, retrato)
// faz com que bruto X SUBA para a direita e bruto Y SUBA para baixo — ambos
// acompanham a tela sem espelhamento. INVERT_X era 'true' numa calibração
// anterior (MADCTL diferente ou painel fisicamente rotacionado), mas com os
// dados atuais de 4 quinas (sup-esq=150, sup-dir=950, inf-dir=1140 → X cresce
// para a direita) a inversão está ERRADA e mapeia esq↔dir — removida.
static constexpr bool TOUCH_SWAP_XY  = false;
static constexpr bool TOUCH_INVERT_X = false;  // bruto X já cresce no mesmo sentido que tela X
static constexpr bool TOUCH_INVERT_Y = false;

