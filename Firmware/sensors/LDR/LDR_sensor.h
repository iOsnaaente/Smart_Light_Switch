/**
 * @file    LDR_sensor.h
 * @brief   Driver para sensor LDR utilizando ADC do ESP32 com leitura 
 *          periódica em task dedicada e filtro de média móvel 
 *          exponencial (EMA).
 * @author  Bruno Gabriel Flores Sampaio
 * @date    Criado em 13 de Abril de 2026
 */

#pragma once

#include "stdbool.h"
#include "stdlib.h"
#include "stdint.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#include "esp_err.h"
#include "esp_log.h"


#ifndef CLAMP_01
#define CLAMP_01(value) \
    ((value) < 0.0f ? 0.0f : ((value) > 1.0f ? 1.0f : (value)))
#endif

static constexpr float ADC_REF_VOLTAGE = 3.3f;

class LDRSensor {
    protected:
        static constexpr const char *LOG_TAG        = "LDR SENSOR";
        static constexpr const char *TASK_NAME      = "LDRSensor";
        static constexpr uint32_t   TASK_STACK_SIZE = 4096;
        static constexpr uint32_t   TASK_PRIORITY   = 8;

    private:

        /* Variáveis de controle da task */
        uint32_t        read_period_ms;
        TaskHandle_t    task_handle;
        portMUX_TYPE    lock;

        /* GPIO do pino de medição - Salvo para debug */
        gpio_num_t input_gpio;

        /* Variáveis de configuração do ADC */
        adc_oneshot_unit_handle_t adc_handle;
        adc_channel_t   adc_channel;
        adc_unit_t      unit;
        adc_atten_t     atten;
        adc_bitwidth_t  bitwidth;
        uint32_t        bitwidth_len;

        /* Variáveis de filtragem */
        float           filtered;
        float           alpha;

        /* Variável de controle do estado da task */
        bool running;

        /**
         * @brief   Entry point da Task de leitura periodica 
         * @note    Necessário para não ser Singleton 
         */
        static void sampling_task_entry(void *pvarg);

        /**
         * @brief   Task de leitura periodica do sensor, que atualiza 
         *          os valores lidos e salva a média filtrada (filtro EMA).
         */
        void sampling_task();

    public:

        /** 
         * @brief   Construtor da classe LDRSensor
         */
        LDRSensor() :   
            read_period_ms(100),
            task_handle(nullptr),
            lock(portMUX_INITIALIZER_UNLOCKED),
            input_gpio(GPIO_NUM_0),
            adc_handle(nullptr),
            adc_channel(ADC_CHANNEL_0),
            unit(ADC_UNIT_1),
            atten(ADC_ATTEN_DB_12),
            bitwidth(ADC_BITWIDTH_12),
            bitwidth_len(4096), 
            filtered(0.0f),
            alpha(0.2f),
            running(false) 
        { }


        /**
         * @brief   Inicializa o sensor LDR, configurando o ADC e preparando 
         *          a task de leitura.
         * @param   unit Unidade ADC a ser utilizada 
         *              (ADC_UNIT_1 ou ADC_UNIT_2)
         * @param   channel Canal ADC a ser utilizado 
         *              (ADC_CHANNEL_0 a ADC_CHANNEL_9)
         * @param   read_period_ms Periodo de leitura em milissegundos 
         *              (default: 100ms)
         * @param   atten Atenuação do ADC 
         *              (default: ADC_ATTEN_DB_12)
         * @param   bitwidth Resolução do ADC 
         *              (default: ADC_BITWIDTH_12)
         * @param   alpha Fator de suavização para o filtro EMA 
         *              (default: 0.2f / valor típico entre 0.1 e 0.3)
         * @return  ESP_OK se a inicialização foi bem-sucedida
         * @return  ESP_ERR_INVALID_ARG se os argumentos são inválidos
         * @return  ESP_ERR_INVALID_STATE se o sensor já foi inicializado ou se 
         *          houve falha na configuração do ADC   
         * @note    O método `start()` deve ser chamado após a inicialização 
         *          para iniciar a leitura periódica do sensor.         
         **/
        esp_err_t init(
            adc_unit_t      unit = ADC_UNIT_1,
            adc_channel_t   channel = ADC_CHANNEL_0,
            uint32_t        read_period_ms = 100,
            adc_atten_t     atten = ADC_ATTEN_DB_12,
            adc_bitwidth_t  bitwidth = ADC_BITWIDTH_12,
            float           alpha = 0.2f
        );

        /**
         * @brief   Inicia a leitura periódica do sensor, criando a task 
         *          responsável pela leitura e atualização dos valores.
         * @return  ESP_OK se a task foi criada com sucesso ou já está rodando
         * @return  ESP_ERR_INVALID_STATE se o sensor não foi inicializado ou se
         *          houve falha ao criar a task.
         */
        esp_err_t start();

        /**
         * @brief   Para a leitura periódica do sensor, deletando a task responsável.
         * @return  ESP_OK se a task foi parada com sucesso ou já está parada
         * @return  ESP_ERR_INVALID_STATE se o sensor não foi inicializado ou se
         *          houve falha ao parar a task.
         */
        esp_err_t stop();

        /**
         * @brief   Desinicializa o sensor, parando a leitura periódica e liberando
         *          os recursos alocados para o ADC.
         * @return  ESP_OK se a desinicialização foi bem-sucedida
         * @return  ESP_ERR_INVALID_STATE se o sensor não foi inicializado ou se
         *          houve falha ao parar a task ou liberar os recursos do ADC.
         */
        esp_err_t deinit();

        /**
         * @brief   Realiza uma leitura única do sensor, retornando os valores lidos
         *          e atualizando a média filtrada.
         * @param   out_normalized Ponteiro para variável onde será armazenado o valor
         *          normalizado (entre 0.0 e 1.0)
         * @return  ESP_OK se a leitura foi bem-sucedida
         * @return  ESP_ERR_INVALID_STATE se o sensor não foi inicializado ou se
         *          houve falha ao ler o ADC.
         */
        esp_err_t read_normalized( float *out_normalized );

        /**
         * @brief   Realiza uma leitura única do sensor, retornando o valor em volts e
         *          o valor bruto do ADC.
         * @param   out_voltage Ponteiro para variável onde será armazenado o valor em
         *          volts.
         * @return  ESP_OK se a leitura foi bem-sucedida
         * @return  ESP_ERR_INVALID_STATE se o sensor não foi inicializado ou se
         *          houve falha ao ler o ADC.
         */
        esp_err_t read_voltage( float *out_voltage );

        /**
         * @brief   Realiza uma leitura única do sensor, retornando o valor bruto do ADC.
         * @param   out_raw Ponteiro para variável onde será armazenado o valor bruto do 
         *          ADC.
         * @return  ESP_OK se a leitura foi bem-sucedida
         * @return  ESP_ERR_INVALID_STATE se o sensor não foi inicializado ou se houve 
         *          falha ao ler o ADC.
         */
        esp_err_t read_raw( uint32_t *out_raw );

        
        /** Destrutor */
        ~LDRSensor(){
            this->deinit();
        }
};
