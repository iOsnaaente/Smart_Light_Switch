/**
 * @file    LDR_sensor.cpp
 * @brief   Implementação para sensor LDR utilizando ADC do ESP32 com 
 *          leitura periódica em task dedicada e filtro de média móvel 
 *          exponencial (EMA).
 * @author  Bruno Gabriel Flores Sampaio
 * @date    Criado em 13 de Abriel de 2026
 */

#include "LDR_sensor.h"

void LDRSensor::sampling_task_entry(void *pvarg) {
	LDRSensor *sensor = static_cast<LDRSensor *>( pvarg );
	if (sensor != nullptr) {
		sensor->sampling_task();
	}
	vTaskDelete(nullptr);
}

void LDRSensor::sampling_task() {
    while ( this->running) {
        int raw = 0;
        adc_oneshot_read( this->adc_handle, this->adc_channel, &raw);
        portENTER_CRITICAL(&this->lock);
        this->filtered = 
            this->alpha * raw + (1.0f - this->alpha) * this->filtered;
        portEXIT_CRITICAL(&this->lock);
        vTaskDelay(pdMS_TO_TICKS(this->read_period_ms));
    }
    vTaskDelete(nullptr);
}


esp_err_t LDRSensor::init(
	adc_unit_t      unit,
	adc_channel_t   channel,
	uint32_t        read_period_ms,
	adc_atten_t     atten,
	adc_bitwidth_t  bitwidth,
	float           alpha
) {
	if (this->adc_handle != nullptr) {
		return ESP_ERR_INVALID_STATE;
	}
	if (read_period_ms == 0) {
		return ESP_ERR_INVALID_ARG;
	}

	this->unit = unit;
	this->adc_channel = channel;
	this->read_period_ms = read_period_ms;
	this->atten = atten;
	this->bitwidth = bitwidth;
	this->alpha = CLAMP_01(alpha);
	this->filtered = -1.0f;
	this->bitwidth_len = 
        this->bitwidth == ADC_BITWIDTH_9  ? 512 :
        this->bitwidth == ADC_BITWIDTH_10 ? 1024 :
        this->bitwidth == ADC_BITWIDTH_11 ? 2048 :
        this->bitwidth == ADC_BITWIDTH_13 ? 8192 :
        4096;
    /* Configura a unidade ADC */
	const adc_oneshot_unit_init_cfg_t unit_cfg = {
		.unit_id = this->unit,
		.clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
		.ulp_mode = ADC_ULP_MODE_DISABLE,
	};
    /* Configura o canal ADC */
	esp_err_t err = adc_oneshot_new_unit(&unit_cfg, &this->adc_handle);
	if (err != ESP_OK) {
		ESP_LOGE(
            this->LOG_TAG, 
            "Falha ao criar unidade ADC: %s", 
            esp_err_to_name(err)
        );
		this->adc_handle = nullptr;
		return err;
	}
	const adc_oneshot_chan_cfg_t chan_cfg = {
		.atten = this->atten,
		.bitwidth = this->bitwidth,
	};
	err = adc_oneshot_config_channel(
        this->adc_handle, 
        this->adc_channel, 
        &chan_cfg
    );
	if (err != ESP_OK) {
		ESP_LOGE(
            this->LOG_TAG, 
            "Falha ao configurar canal ADC: %s", 
            esp_err_to_name(err)
        );
		adc_oneshot_del_unit(this->adc_handle);
		this->adc_handle = nullptr;
		return err;
	}
	ESP_LOGD(
		this->LOG_TAG,
		"LDR inicializado (unit=%d channel=%d periodo=%lums)",
		(int)this->unit,
		(int)this->adc_channel,
		(unsigned long)this->read_period_ms
	);
	return ESP_OK;
}


esp_err_t LDRSensor::start() {
	if (this->adc_handle == nullptr) {
		return ESP_ERR_INVALID_STATE;
	}
	if (this->running || this->task_handle != nullptr) {
		return ESP_OK;
	}
	this->running = true;
	BaseType_t ok = xTaskCreate(
		LDRSensor::sampling_task_entry,
		this->TASK_NAME,
		this->TASK_STACK_SIZE,
		this,
        this->TASK_PRIORITY,
		&this->task_handle
	);
	if (ok != pdPASS) {
		this->running = false;
		this->task_handle = nullptr;
		ESP_LOGE(
            this->LOG_TAG, 
            "Falha ao criar task de leitura periodica"
        );
		return ESP_ERR_INVALID_STATE;
	}
	return ESP_OK;
}


esp_err_t LDRSensor::stop() {
	if (this->adc_handle == nullptr) {
		return ESP_ERR_INVALID_STATE;
	}
	if (!this->running && this->task_handle == nullptr) {
		return ESP_OK;
	}
	this->running = false;
	if (this->task_handle != nullptr) {
		TaskHandle_t handle = this->task_handle;
		this->task_handle = nullptr;
		vTaskDelete(handle);
	}
	return ESP_OK;
}


esp_err_t LDRSensor::deinit() {
	if (this->adc_handle == nullptr) {
		return ESP_OK;
	}
	esp_err_t err = this->stop();
	esp_err_t del_err = adc_oneshot_del_unit(this->adc_handle);
	this->adc_handle = nullptr;
	this->filtered = 0.0f;
	if (err != ESP_OK) {
		return err;
	}
	return del_err;
}


esp_err_t LDRSensor::read_raw( uint32_t *out_raw ) {
	if (out_raw == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}
	if (this->adc_handle == nullptr) {
		return ESP_ERR_INVALID_STATE;
	}
    portENTER_CRITICAL(&this->lock);
	*out_raw = this->filtered;
	portEXIT_CRITICAL(&this->lock);
    return ESP_OK;
}


esp_err_t LDRSensor::read_normalized( float *out_normalized ) {
	if (out_normalized == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}
	if (this->adc_handle == nullptr) {
		return ESP_ERR_INVALID_STATE;
	}
	portENTER_CRITICAL(&this->lock);
	*out_normalized = CLAMP_01( filtered / this->bitwidth_len );
	portEXIT_CRITICAL(&this->lock);
	return ESP_OK;
}


esp_err_t LDRSensor::read_voltage( float *out_voltage ) {
	if ( out_voltage == nullptr ) {
		return ESP_ERR_INVALID_ARG;
	}
	if (this->adc_handle == nullptr) {
		return ESP_ERR_INVALID_STATE;
	}
    portENTER_CRITICAL(&this->lock);
    *out_voltage = 
        CLAMP_01( filtered / this->bitwidth_len ) * ADC_REF_VOLTAGE;
    portEXIT_CRITICAL(&this->lock);
	return ESP_OK;
}
