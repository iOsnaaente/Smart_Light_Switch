/**
 * @file    lamp_controller.cpp
 * @brief   Implementação da classe de controle do acionamento da lâmpada via 
 *          detecção de zero-cross e controle de dimmer via triac.
 * @author  Bruno Gabriel Flores Sampaio
 * @date    Criado em 13 de Abril de 2026
 */

#include "lamp_controller.h"


void IRAM_ATTR LampController::zero_cross_ISR_Entry( void *pvarg ){
	LampController *self = static_cast<LampController *>( pvarg );
	if (self != nullptr) {
		self->zero_cross_ISR();
	}
}

void IRAM_ATTR LampController::zero_cross_ISR() {
	BaseType_t hp_task_woken = pdFALSE;
	int64_t now_us = esp_timer_get_time();
	this->last_zero_cross_us = now_us;
	this->isr_count++;
	if ( this->task_handle != nullptr ) {
		vTaskNotifyGiveFromISR( this->task_handle, &hp_task_woken );
		if ( hp_task_woken == pdTRUE ) {
			portYIELD_FROM_ISR();
		}
	}
}

void LampController::lamp_task_Entry( void *pvarg ){
	LampController *self = static_cast<LampController *>( pvarg );
	if (self != nullptr) {
		self->lamp_task();
	}
	vTaskDelete(nullptr);
}

void LampController::lamp_task(){
	const int64_t safety_margin_us = 20;
	int64_t prev_zc_us = 0;

	while (this->running) {
		int64_t half_cycle_us;
        int64_t cycle_start_us;
        bool is_online;

		int64_t now_us = esp_timer_get_time();
		portENTER_CRITICAL(&this->lock);
		int64_t zc_us = this->last_zero_cross_us;
		float sp = CLAMP_01(this->setpoint);
		portEXIT_CRITICAL(&this->lock);

		/* Calcula o Half Cycle Period */
		if ( zc_us > 0 && prev_zc_us > 0 ){
			int64_t dt = zc_us - prev_zc_us;
			if ( dt >= (int64_t)this->debounce_us ) {
				this->last_half_cycle_us = CLAMP_64(
					dt, 
					this->min_half_cycle_us, 
					this->max_half_cycle_us
				);
			} else {
				this->debounce_drop_count++;
			}
		}
		prev_zc_us = zc_us;
		half_cycle_us = this->last_half_cycle_us > 0 
			? this->last_half_cycle_us 
			: this->nominal_half_cycle_us;

		TickType_t wait_ticks = pdMS_TO_TICKS((uint32_t)(half_cycle_us / 1000));
		if (wait_ticks == 0) {
			wait_ticks = 1;
		}
		ulTaskNotifyTake(pdTRUE, wait_ticks);
		now_us = esp_timer_get_time();
		
		/* Verifica se o ZC está online ou se deve operar offline */
		is_online = (zc_us > 0) && (now_us - zc_us < this->zc_timeout_us);
		if (is_online) {
			cycle_start_us = zc_us;
			this->open_loop_anchor_us = zc_us;
		} else {
			if (this->open_loop_anchor_us <= 0) {
				this->open_loop_anchor_us = now_us;
			} else {
				this->open_loop_anchor_us += half_cycle_us;
			}
			if (this->open_loop_anchor_us < (now_us - half_cycle_us)) {
				this->open_loop_anchor_us = now_us;
			}
			cycle_start_us = this->open_loop_anchor_us;
		}

		/* Calculo do delay do TRIAC */
		int64_t max_delay_us = 
			half_cycle_us - (int64_t)this->triac_gate_pulse_us - safety_margin_us;
		if (max_delay_us < 0) {
			max_delay_us = 0;
		}
		int64_t trigger_delay_us = 
			(int64_t)((1.0f - sp) * (float)max_delay_us);
		int64_t trigger_at_us = cycle_start_us + trigger_delay_us;
		now_us = esp_timer_get_time();
		if (trigger_at_us < now_us) {
			trigger_at_us = now_us;
		}

		/* Aguar o tempo para disparo */
		this->wait_until_us( trigger_at_us );
		this->triac_fire_pulse();

		/* Atualiza o status da lâmpada */
		portENTER_CRITICAL(&this->lock);
		this->triac_pulse_count++;
		this->zc_online = is_online;
		if (sp <= 0.0f) {
			this->status = LAMP_FULLY_OFF;
		} else if (sp >= 1.0f) {
			this->status = LAMP_FULLY_ON;
		} else if ( !is_online ) {
			this->status = LAMP_STATUS_OFFLINE;
		} else{
			this->status = LAMP_STATUS_OK;
		}
		portEXIT_CRITICAL(&this->lock);
	}
	this->task_handle = nullptr;
}


void LampController::triac_fire_pulse() {
	gpio_set_level( this->triac_gate_gpio, true );
	esp_rom_delay_us( this->triac_gate_pulse_us );
	gpio_set_level( this->triac_gate_gpio, false );
}


esp_err_t LampController::init(
	gpio_num_t zc_gpio, gpio_num_t triac_gpio
){
	/* Verifica se Pinos de ZC e Triac são válidos */
	if ( zc_gpio < 0 || triac_gpio < 0 ) {
		ESP_LOGE(
			LOG_TAG, 
			"INIT: Invalid GPIO numbers: zc_gpio=%d triac_gpio=%d", 
			(int)zc_gpio, (int)triac_gpio
		);	
		this->status = LAMP_STATUS_ERROR;
		return ESP_ERR_INVALID_ARG;
	}
	this->zero_cross_gpio = zc_gpio;
	this->triac_gate_gpio = triac_gpio;

	/* Verifica se o controlador já está em execução */
	if (this->running) {
		ESP_LOGW(
			LOG_TAG, 
			"INIT: Controller is already running, cannot re-initialize"
		);
		return ESP_ERR_INVALID_STATE;
	}

	/* Zera as variáveis para inicialização correta */
	this->setpoint = 0.0f;
	this->last_zero_cross_us = 0;
	this->last_half_cycle_us = this->nominal_half_cycle_us;
	this->isr_count = 0;
	this->debounce_drop_count = 0;
	this->triac_pulse_count = 0;
	this->zc_online = false;
	this->status = LAMP_STATUS_OFFLINE;
	this->open_loop_anchor_us = 0;

	/* Configura o pino do Triac para Saída do disparo de Gate */
	const gpio_config_t triac_cfg = {
		.pin_bit_mask = (1ULL << this->triac_gate_gpio),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE,
	};
	ESP_ERROR_CHECK( gpio_config(&triac_cfg) );
	ESP_ERROR_CHECK( gpio_set_level(this->triac_gate_gpio, 0) );

	/* Configura o pino de Zero-Cross para Entrada com interrupção */
	const gpio_config_t zc_cfg = {
		.pin_bit_mask = (1ULL << this->zero_cross_gpio),
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = this->zero_cross_edge,
	};
	ESP_ERROR_CHECK( gpio_config(&zc_cfg) );

	/* Define a interrupção */
	esp_err_t isr_ret = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
	if ( isr_ret != ESP_OK && isr_ret != ESP_ERR_INVALID_STATE) {
		ESP_LOGE(
			LOG_TAG, 
			"Failed to install GPIO ISR service: %s", 
			esp_err_to_name(isr_ret)
		);
		this->status = LAMP_STATUS_ERROR;
		return isr_ret;
	}

	/* Inicialização concluida - Controlador iniciado */
	this->initialized = true;
	ESP_LOGI(
		LOG_TAG, 
		"Controller initialized: zc_gpio=%d triac_gpio=%d", 
		this->zero_cross_gpio, 
		this->triac_gate_gpio
	);
	this->status = LAMP_STATUS_OK;
	return ESP_OK;
}


esp_err_t LampController::start(){
	if (!this->initialized) {
		return ESP_ERR_INVALID_STATE;
	}
	if (this->running) {
		return ESP_OK;
	}
	esp_err_t handler_ret = gpio_isr_handler_add(
		this->zero_cross_gpio, zero_cross_ISR_Entry, this
	);
	if (handler_ret != ESP_OK) {
		this->status = LAMP_STATUS_ERROR;
		ESP_LOGE(LOG_TAG, "START: Failed to add ISR handler: %s", esp_err_to_name(handler_ret));
		return handler_ret;
	}
	this->running = true;
	BaseType_t created = xTaskCreatePinnedToCore(
		lamp_task_Entry,
		"LampTask",
		task_stack_size / sizeof(StackType_t),
		this,
		task_priority,
		&task_handle,
		tskNO_AFFINITY
	);
	if ( created != pdPASS || this->task_handle == nullptr) {
		this->running = false;
		gpio_isr_handler_remove(this->zero_cross_gpio);
		this->status = LAMP_STATUS_ERROR;
		return ESP_ERR_NO_MEM;
	}
	ESP_LOGI( LOG_TAG, "Controller started" );
	this->status = LAMP_STATUS_OK;
	return ESP_OK;
}


esp_err_t LampController::stop(){
	if (!this->initialized) {
		ESP_LOGE( LOG_TAG, "STOP: Controller not initialized");
		return ESP_ERR_INVALID_STATE;
	}
	if (!this->running) {
		ESP_LOGW(LOG_TAG, "STOP: Controller not running");
		return ESP_OK;
	}
	this->running = false;
	if (this->task_handle != nullptr) {
		xTaskNotifyGive(this->task_handle);
		vTaskDelay(pdMS_TO_TICKS(5));
		this->task_handle = nullptr;
	}
	gpio_set_level(this->triac_gate_gpio, 0);
	gpio_isr_handler_remove(this->zero_cross_gpio);
	this->zc_online = false;
	this->status = LAMP_STATUS_OFFLINE;
	ESP_LOGI(LOG_TAG, "Controller stopped");
	return ESP_OK;
}


void LampController::wait_until_us( int64_t target_us ){
	int64_t now_us;
	while ((now_us = esp_timer_get_time()) < target_us) {
		int64_t remaining = target_us - now_us;
		if ( remaining > 1000 ) {
			vTaskDelay( pdMS_TO_TICKS( remaining / 1000 ) );
		} else {
			esp_rom_delay_us( (uint32_t)remaining );
		}
	}
}