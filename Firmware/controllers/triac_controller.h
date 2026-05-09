/**
 * @file    triac_controller.h
 * @brief   Classe de controle do acionamento do triac.
 * @author  Bruno Gabriel Flores Sampaio
 * @date    Criado em 13 de Abril de 2026
 */

#pragma once

#include "stdbool.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"


#define CLAMP_64(x, min, max) \
    ( ((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x)))

#define CLAMP_01(x) \
    ( ((x) < 0.0f) ? 0.0f : (((x) > 1.0f) ? 1.0f : (x)))


typedef enum {
    LAMP_STATUS_OK = 0,
    LAMP_FULLY_ON,
    LAMP_FULLY_OFF,
    LAMP_STATUS_ERROR,
    LAMP_STATUS_OFFLINE,
} LampStatus_t;

static inline const char* lamp_status_to_string(LampStatus_t status) {
    switch (status) {
        case LAMP_STATUS_OK:        return "OK";
        case LAMP_FULLY_ON:         return "FULLY_ON";
        case LAMP_FULLY_OFF:        return "FULLY_OFF";
        case LAMP_STATUS_ERROR:     return "ERROR";
        case LAMP_STATUS_OFFLINE:   return "OFFLINE";
        default:                    return "UNKNOWN";
    }
}


class TriacController {
    protected:

        static constexpr const char *LOG_TAG = "TRIAC_CNTRL";
        static constexpr const char *TASK_NAME = "TriacTask";
        static constexpr uint32_t TASK_STACK_SIZE = 4096;
        static constexpr uint32_t TASK_PRIORITY = 10;

        /**
         * @brief   Entry point da ISR de interrupção da detecção 
         *          de zero-cross.
         */
        static void IRAM_ATTR zero_cross_ISR_Entry( void *pvarg );

        /**
         * @brief   Entry point da tarefa de controle do triac
         */
        static void triac_task_Entry( void *pvarg );

    private:
    
        /**
         * @brief   ISR de detecção do ZeroCross 
         * @note    Esta função é chamada na interrupção de borda 
         *          configurada para o pino de detecção de zero-cross.
         *          As detecções podem ser HALF ou FULL cycle.
         */
        void zero_cross_ISR();

        /**
         * @brief   Tarefa principal de controle do triac
         * @note    Esta função é executada em uma tarefa FreeRTOS e 
         *          é responsável por aguardar os eventos de zero-cross
         *          e acionar o triac no tempo definido pelo setpoint.
         */
        void triac_task();

        /**
         * @brief   Aciona o pulso de Gate do triac
         */
        void triac_fire_pulse();

        /**
         * @brief   Aguarda até um tempo específico em microsegundos
         * @param   target_us: tempo alvo em microsegundos
         */
        static void wait_until_us(int64_t target_us);

        int64_t open_loop_anchor_us;
        TaskHandle_t task_handle;
        portMUX_TYPE lock;
        bool initialized;
        bool running;

        
    public:
        gpio_num_t  zero_cross_gpio;
        gpio_num_t  triac_gate_gpio;

        gpio_int_type_t zero_cross_edge;
        uint32_t    debounce_us;
        
        uint32_t    triac_gate_pulse_us;
        int64_t     nominal_half_cycle_us;
        int64_t     min_half_cycle_us;
        int64_t     max_half_cycle_us;
        int64_t     zc_timeout_us;
        
        uint32_t    task_stack_size;
        uint32_t    task_priority;
        
        float       setpoint;
        
        int64_t     last_zero_cross_us;
        int64_t     last_half_cycle_us;
        
        uint32_t    isr_count;
        
        uint32_t    debounce_drop_count;
        uint32_t    triac_pulse_count;
        bool        zc_online;

        LampStatus_t status;

        /**
         * @brief   Construtor da classe TriacController
         * @note    Inicializa as variáveis com valores padrão e configura 
         *          o controlador para um estado inicial seguro: 
         *              -> lâmpada desligada e offline
         */
        TriacController() :   
            open_loop_anchor_us(0),
            task_handle(nullptr),
            lock(portMUX_INITIALIZER_UNLOCKED),
            initialized(false),
            running(false),
            zero_cross_gpio(GPIO_NUM_NC), 
            triac_gate_gpio(GPIO_NUM_NC),
            zero_cross_edge(GPIO_INTR_ANYEDGE),
            debounce_us(400), 
            triac_gate_pulse_us(120),
            nominal_half_cycle_us(8333),
            min_half_cycle_us(7000), 
            max_half_cycle_us(10000),
            zc_timeout_us(16666), 
            task_stack_size(TASK_STACK_SIZE),
            task_priority(TASK_PRIORITY),
            setpoint(0.0f),
            last_zero_cross_us(0), 
            last_half_cycle_us(8333),
            isr_count(0), 
            debounce_drop_count(0), 
            triac_pulse_count(0),
            zc_online(false),
            status(LAMP_STATUS_OFFLINE)
        { }


        /**
         * @brief   Inicializa o controlador de lâmpada, configurando os pinos
         *          de zero-cross e triac, e preparando a estrutura para operação.
         * @param   zero_cross_gpio: GPIO utilizado para detecção de zero-cross
         * @param   triac_gate_gpio: GPIO utilizado para acionar o gate do triac
         * @return  ESP_OK se a inicialização foi bem-sucedida
         * @return  ESP_ERR_INVALID_ARG se os pinos fornecidos são inválidos
         * @return  ESP_ERR_INVALID_STATE se o controlador já estiver em execução
         * @note    Esta função deve ser chamada antes de iniciar o controlador.
         */
        esp_err_t   init( 
            gpio_num_t zero_cross_gpio, gpio_num_t triac_gate_gpio 
        );

        /**
         * @brief   Inicia a tarefa de controle da lâmpada, habilita a detecção 
         *          de zero-cross e o acionamento do triac conforme o setpoint.
         * @return  ESP_OK se a tarefa foi iniciada com sucesso
         * @return  ESP_ERR_INVALID_STATE se o controlador não foi inicializado ou já 
         *          estiver em execução
         * @note    Esta função deve ser chamada após a inicialização do controlador.
         */
        esp_err_t   start();

        /**
         * @brief   Para a tarefa de controle da lâmpada e desabilita a detecção de 
         *          zero-cross.
         * @return  ESP_OK se a tarefa foi parada com sucesso
         * @return  ESP_ERR_INVALID_STATE se o controlador não estiver em execução
         */
        esp_err_t   stop();


        /**
         * @brief   Define o setpoint de controle da lâmpada, onde: 
         *              - 0.0f = totalmente desligada
         *             - 1.0f = totalmente ligada
         * @param   setpoint: valor de controle entre 0.0f e 1.0f
         * @return  ESP_OK se o setpoint foi definido com sucesso
         * @return  ESP_ERR_INVALID_STATE se o controlador não foi inicializado
         */
        esp_err_t set_setpoint( float setpoint ){
            if ( !initialized ) {
                this->status = LAMP_STATUS_ERROR;
                ESP_LOGE( 
                    LOG_TAG, 
                    "SET_SETPOINT: Controller not initialized, cannot set setpoint"
                );
                return ESP_ERR_INVALID_STATE;
            }
            portENTER_CRITICAL( &this->lock );
            this->setpoint = CLAMP_01( setpoint );
            portEXIT_CRITICAL( &this->lock );
            this->status = LAMP_STATUS_OK;
            ESP_LOGD( 
                LOG_TAG, 
                "SET_SETPOINT: Setpoint set to %.3f", 
                this->get_setpoint()
            );
            return ESP_OK;
        }

        /**
         * @brief   Retorna o valor atual do setpoint de controle da lâmpada
         * @return  Valor do setpoint entre 0.0f e 1.0f
         * @return  0.0f se o controlador estiver em estado de erro
         * @return  ESP_ERR_INVALID_STATE se o controlador não foi inicializado
         */
        float get_setpoint(){
            if ( this->status == LAMP_STATUS_ERROR ) {
                ESP_LOGE( 
                    LOG_TAG, 
                    "GET_SETPOINT: Controller in error state, returning 0.0f"
                );
                return 0.0f;
            }
            float value;
            portENTER_CRITICAL( &this->lock );
            value = setpoint;
            portEXIT_CRITICAL( &this->lock );
            ESP_LOGD( 
                LOG_TAG, 
                "GET_SETPOINT: Returning setpoint value %.3f", 
                value
            );
            return value;
        }
        
        LampStatus_t get_status(){
            return this->status;
        }
};
