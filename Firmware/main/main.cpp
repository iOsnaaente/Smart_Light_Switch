#include "../board_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "triac_controller.h"
#include "LDR/LDR_sensor.h"

#include "comm_manager.h"
#include "lvgl_port.h"
#include "ui_manager.h"

#include "app_events.h"
#include "app_modes.h"
#include "event_bus.h"

#include "credentials.h"


static const char *TAG = "LAMP_APP";

/**
 * @brief   Chaves para isolar a comunicação durante o debug do reset.
 * @details Wi-Fi sempre sobe junto quando o comm_manager inicia (a task de
 *          orquestração chama wifi_manager_start incondicionalmente) — então
 *          "Wi-Fi isolado" é só ble=0/mqtt=0. Vá ligando um de cada vez,
 *          recompilando/flasheando entre os testes:
 *              1) ble=0 mqtt=0  -> só Wi-Fi
 *              2) ble=1 mqtt=0  -> Wi-Fi + provisionamento BLE
 *              3) ble=0 mqtt=1  -> Wi-Fi + MQTT
 *              4) ble=1 mqtt=1  -> tudo (config original)
 */
#define COMM_TEST_ENABLE_BLE   1
#define COMM_TEST_ENABLE_MQTT  1

static TriacController *triac_cntrl;
static LDRSensor *ldr_sensor;


static void on_dimmer_command(
    void *handler_arg, 
    esp_event_base_t base, 
    int32_t id, 
    void *event_data
) {
    event_dimmer_command_t *evt = (event_dimmer_command_t *)event_data;
    if (triac_cntrl != nullptr) {
        triac_cntrl->set_setpoint(evt->value / 100.0f);
    }
}

static void on_relay_command(
    void *handler_arg, 
    esp_event_base_t base, 
    int32_t id, 
    void *event_data
) {
    event_relay_command_t *evt = (event_relay_command_t *)event_data;
    if (triac_cntrl != nullptr) {
        triac_cntrl->set_setpoint(evt->relay_on ? 1.0f : 0.0f);
    }
}

static void lamp_control_task( void *arg ) {
    uint32_t counter = 0;
    float normalized = 0.0f;
    float voltage = 0.0f;
    while (true) {
        if ( triac_cntrl != nullptr ) {

            /* Lê o sensor LDR */
            esp_err_t err = ldr_sensor->read_normalized( &normalized );
            if (err != ESP_OK) {
                ESP_LOGE( 
                    TAG, 
                    "Erro ao ler valor normalizado do LDR: %s", 
                    esp_err_to_name(err) 
                );
                triac_cntrl->status = LAMP_STATUS_ERROR;
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }
            ldr_sensor->read_voltage( &voltage );

            /** 
             * @brief   Publica a leitura crua do LDR.
             * @details Quem decide o que ela significa (telas, MQTT) assina 
             *          LDR_UPDATE; o guardrail abaixo só "achata" os 
             *          extremos para fins de controle. 
             */
            event_ldr_update_t ldr_evt = {}; 
            ldr_evt.normalized = normalized, 
            ldr_evt.voltage = voltage;
            event_bus_post(
                SMART_SWITCH_EVENT_LDR_UPDATE,
                &ldr_evt, sizeof(ldr_evt),
                pdMS_TO_TICKS(100)
            );

            /**
             * @details O ajuste automático por LDR só faz sentido no modo 
             *          Automático - no Manual, quem decide o setpoint são 
             *          os comandos vindos da UI/MQTT/BLE 
             *              - DIMMER_COMMAND / RELAY_COMMAND 
             *          tratados em on_dimmer_command / on_relay_command, 
             *          logo acima. 
             */
            if ( app_modes_get() == APP_MODE_AUTOMATIC ) {
                /* Faz o guardrail do valor normalizado */
                if ( normalized < 0.15f ) {
                    // Força o setpoint para 0.0f quando a luz ambiente estiver muito baixa
                    triac_cntrl->status = LAMP_FULLY_ON;
                    normalized = 0.0f;
                } else if (normalized > 0.85f) {
                    // Força o setpoint para 1.0f quando a luz ambiente estiver muito alta
                    triac_cntrl->status = LAMP_FULLY_OFF;
                    normalized = 1.0f;
                } else {
                    triac_cntrl->status = LAMP_STATUS_OK;
                }

                /* Aplica o setpoint do triac */
                triac_cntrl->set_setpoint( normalized );
            }

            /** 
             * @brief   Publica o estado atual do dimmer — espelha o setpoint realmente
             * @details armado no triac agora (decidido pelo automático ou por um
             *          comando manual), para telas/MQTT ecoarem o valor de verdade.
             * @note    delay_us não tem um equivalente público exato no controlador
             *          (o atraso de disparo é interno); usamos last_half_cycle_us —
             *          a telemetria de tempo mais próxima e honesta disponível. 
             */
            event_dimmer_update_t dimmer_evt = {};
            dimmer_evt.duty = triac_cntrl->setpoint;
            dimmer_evt.delay_us = triac_cntrl->last_half_cycle_us;
            event_bus_post(
                SMART_SWITCH_EVENT_DIMMER_UPDATE, 
                &dimmer_evt, 
                sizeof(dimmer_evt), 
                pdMS_TO_TICKS(100)
            );

            /* [DEBUG] Rebaixado de LOGI -> LOGD: enquanto o foco é depurar a UI
             * (touch/menus), esse bloco a cada ~2.5s só polui o monitor serial
             * e atrapalha a leitura dos logs [DEBUG] de touch/navegação. Sobe
             * de novo pra LOGI (ou `idf.py menuconfig` -> Log level = Debug)
             * quando precisar depurar LDR/triac de novo.              
             * 1000.0f precisa bater com MQTT_LUX_SCALE em mqtt_client.cpp,
             * mesma conversão usada na telemetria MQTT. */
            if ( (counter++) % 25 == 0 ) {
                ESP_LOGD( TAG, "LDR Normalized: %.2f", normalized );
                ESP_LOGI( TAG, "Lux medido: %.1f", normalized * 1000.0f);
                if ( triac_cntrl->zc_online ) {
                    ESP_LOGD( TAG, "Setpoint: %.2f%%", triac_cntrl->setpoint * 100.0f );
                } else {
                    ESP_LOGD( TAG, "Setpoint: N/A (ZC Offline)" );
                }
                ESP_LOGD( TAG, "Triac Status: %s", lamp_status_to_string(triac_cntrl->status)  );
                ESP_LOGD( TAG, "Triac Pulse Count: %u", triac_cntrl->triac_pulse_count );
                ESP_LOGD( TAG, "ISR Count: %u", triac_cntrl->isr_count );
                ESP_LOGD( TAG, "Last Half Cycle (ms): %.2f", triac_cntrl->last_half_cycle_us / 1000.0f );
                ESP_LOGD( TAG, "Debounce Drop Count: %u\n", triac_cntrl->debounce_drop_count );
                
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


extern "C" void app_main(void) {
    esp_err_t err;

    /* event_bus_init() precisa rodar ANTES de ui_manager_init(): este último
     * chama event_bus_register() para todos os handlers da UI (on_ldr_update,
     * on_dimmer_update, on_net_status, etc.). Sem o loop já criado,
     * event_bus_register() retorna ESP_ERR_INVALID_STATE silenciosamente (o
     * retorno não é checado em ui_manager_init) — a UI nunca recebia nenhum
     * evento do event_bus (PV/MV/Wi-Fi/modo ficavam travados em 0/estado
     * inicial); só o gesto de toque "funcionava" por atualizar a tela direto,
     * sem passar pelo event_bus. event_bus_init() é idempotente — chamar de
     * novo dentro de comm_manager_init() mais abaixo é seguro (no-op). */
    err = event_bus_init();
    if (err != ESP_OK) {
        ESP_LOGE( TAG, "Falha ao inicializar o event_bus: %s", esp_err_to_name(err));
    }

    /* [DEBUG] Movido para ANTES do comm_manager: o driver SPI do TFT precisa
     * de memória capaz de DMA para o barramento (spi_bus_initialize). Se o
     * BLE/Wi-Fi/MQTT subirem primeiro, consomem essa memória e o ili9340
     * falha com ESP_ERR_NO_MEM (257) -> assert -> reset (visto no monitor
     * com comm_manager habilitado). Inicializando o display primeiro, ele
     * garante seus buffers DMA enquanto a memória ainda está livre. */
    ESP_LOGI( TAG, "Inicializando interface local (LVGL)");
    err = lvgl_port_init();
    if (err != ESP_OK) {
        ESP_LOGE( TAG, "Falha ao inicializar o LVGL: %s", esp_err_to_name(err));
    } else {
        err = ui_manager_init();
        if (err != ESP_OK) {
            ESP_LOGE( TAG, "Falha ao montar a interface: %s", esp_err_to_name(err));
        } else {
            ui_manager_start();
        }
    }

    ESP_LOGI( TAG, "Inicializando ciclo de conectividade (BLE -> Wi-Fi -> MQTT)");
    comm_manager_config_t comm_cfg = {};
    comm_cfg.enable_ble = COMM_TEST_ENABLE_BLE;
    comm_cfg.enable_mqtt = COMM_TEST_ENABLE_MQTT;
    comm_cfg.enable_http = false;
    comm_cfg.mqtt_broker_uri = MQTT_BROKER_URI;
    comm_cfg.mqtt_username = MQTT_USERNAME;
    comm_cfg.mqtt_password = MQTT_PASSWORD;
    err = comm_manager_init(&comm_cfg);
    if (err != ESP_OK) {
        ESP_LOGE( TAG, "Falha ao inicializar o comm_manager: %s", esp_err_to_name(err));
    } else {
        comm_manager_start();
    }

    ESP_LOGD( TAG, "Inicializando sensor LDR");
    ldr_sensor = new LDRSensor();
    err = ldr_sensor->init(
        ADC_UNIT_1, ADC_CHANNEL_0,
        100, 
        ADC_ATTEN_DB_12,
        ADC_BITWIDTH_DEFAULT,
        0.2f
    );
    if (err != ESP_OK) {
        ESP_LOGE( TAG, "Falha ao inicializar o LDR: %s", esp_err_to_name(err));
    } else {
        ESP_LOGD( TAG, "LDR configurado no GPIO %d", (int)PIN_NUM_SENSOR_LDR);
        ldr_sensor->start();
    }

    ESP_LOGD( TAG, "Inicializando controlador de triac");
    triac_cntrl = new TriacController();
    err = triac_cntrl->init(
        PIN_NUM_ZEROCROSS, 
        PIN_NUM_GATE_TRIAC
    );   
    if (err != ESP_OK) {
        ESP_LOGE( 
            TAG, 
            "Falha ao inicializar o controlador de triac: %s", 
            esp_err_to_name(err)
        );
    } else {
        triac_cntrl->start();

        // Comandos vindos de qualquer canal (MQTT/BLE/UI) chegam aqui como
        // eventos — aplicam direto no controlador, ponto final do "comando".
        event_bus_register(SMART_SWITCH_EVENT_DIMMER_COMMAND, &on_dimmer_command, nullptr);
        event_bus_register(SMART_SWITCH_EVENT_RELAY_COMMAND,  &on_relay_command,  nullptr);
    }

    ESP_LOGI( TAG, "Iniciando tarefa de controle da lâmpada");
    xTaskCreate(
        lamp_control_task, 
        "lamp_ctrl", 
        1024*4, 
        &triac_cntrl, 
        5, 
        nullptr
    );

    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI( TAG, "Setup completo. Tasks inicializadas.");
    vTaskDelete( NULL );
}
