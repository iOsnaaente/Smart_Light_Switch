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
// uso). [DEBUG] Recalibrado a partir de um teste de 4 quinas DELIBERADO
// (muito mais confiável que o gesto circular usado antes): o usuário tocou
// cada canto da tela e leu o "bruto" nos logs de touch_read_cb:
//     sup-esq=(1640,305)  sup-dir=(304,118)
//     inf-esq=(1187,1723) inf-dir=(449,1834)
// Cruzando com a posição lógica de cada canto (sup-esq=(0,0), sup-dir=(239,0),
// inf-esq=(0,319), inf-dir=(239,319)): bruto X CAI conforme a tela X sobe
// (1640/1187 nos cantos X=0, 304/449 nos X=239 -> precisa INVERT_X) e bruto Y
// SOBE conforme a tela Y sobe (305/118 nos Y=0, 1723/1834 nos Y=319 -> não
// precisa inverter Y nem trocar eixos). MIN/MAX abaixo são as médias dos
// extremos observados — ainda fica uma folga de até ~50px nas quinas (a
// película resistiva tem acoplamento entre eixos que uma calibração linear
// de 1 eixo não corrige 100%), mas é suficiente pra acertar botões de 40px+.
static constexpr int TOUCH_RAW_X_MIN = 370;   // ~ bruto X nos cantos onde tela X = 239
static constexpr int TOUCH_RAW_X_MAX = 1415;  // ~ bruto X nos cantos onde tela X = 0 (eixo invertido)
static constexpr int TOUCH_RAW_Y_MIN = 210;   // ~ bruto Y nos cantos onde tela Y = 0
static constexpr int TOUCH_RAW_Y_MAX = 1780;  // ~ bruto Y nos cantos onde tela Y = 319

// [DEBUG] Filtro de "toque fantasma": com o painel em repouso (sem encostar),
// o XPT2046 ficou relatando IRQ ativo e leituras BRUTAS estáveis perto de
// (94,100) continuamente — sintoma de pressão mecânica da película resistiva
// contra o vidro (moldura/bezel apertando), não ruído elétrico aleatório.
// Esses falsos toques mapeavam para tela=(0,0) e o LVGL os via como
// "pressionado" sem parar — provável causa dos botões não responderem (o
// indev nunca via uma transição solto->pressionado limpa num botão real).
// Toques de verdade sempre vieram com bruto X >= ~150 (gap de ~50 unidades
// sem sobreposição com o fantasma em ~94) — qualquer leitura com X abaixo
// deste limiar é descartada como "sem toque real". Se o fantasma persistir ou
// toques reais no canto esquerdo passarem a ser ignorados, ajuste aqui — mas
// o problema de fundo é mecânico: vale checar se a película de toque não está
// sendo pressionada pelo encaixe da moldura do display.
static constexpr int TOUCH_RAW_NOISE_MAX = 130;

// [DEBUG] Eixo do XPT2046 é fixo À PELÍCULA FÍSICA — não acompanha o MADCTL
// do ILI9341 (ili9340.c). Quando giramos o conteúdo da tela em hardware
// (MADCTL 0x08 -> 0xE8, para ficar em pé) o mapeamento touch->tela ficou
// desalinhado: o dedo aparecia em (x,y) errado pro LVGL, fora da área
// clicável do botão tocado — essa era a causa real dos toques nos menus
// "não fazerem nada" (os callbacks já estavam certos, ver ui_manager.cpp).
//
// CONFIRMADO com o teste de 4 quinas (valores brutos no comentário de
// TOUCH_RAW_*_MIN/MAX acima): comparando bruto x tela em cada canto,
// bruto X cai conforme a tela X sobe (precisa inverter) e bruto Y sobe
// junto com a tela Y (não inverte, não troca eixos) — ou seja, os eixos
// X/Y do XPT2046 já correspondem 1:1 aos da tela, só o X está espelhado:
static constexpr bool TOUCH_SWAP_XY  = false;
static constexpr bool TOUCH_INVERT_X = true;
static constexpr bool TOUCH_INVERT_Y = false;

