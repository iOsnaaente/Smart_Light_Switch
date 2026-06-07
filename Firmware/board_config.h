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

// SPI - Dimensões Display TFT (montagem física do painel é em retrato)
static constexpr int LCD_WIDTH  = 240;
static constexpr int LCD_HEIGHT = 320;
static constexpr int CAL_MARGIN = 0;
static constexpr int CAL_CROSS = 0;

// Touch XPT2046 - faixa bruta de leitura do ADC (12 bits). Não existe rotina
// de calibração implementada (campos _min_xp/_max_xp etc. de TFT_t ficam sem
// uso); estes são valores de partida típicos — ajuste-os conforme o
// comportamento observado no painel físico (ex.: trocar MIN<->MAX inverte o eixo).
static constexpr int TOUCH_RAW_X_MIN = 200;
static constexpr int TOUCH_RAW_X_MAX = 3900;
static constexpr int TOUCH_RAW_Y_MIN = 200;
static constexpr int TOUCH_RAW_Y_MAX = 3900;

