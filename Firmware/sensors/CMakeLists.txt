idf_component_register(
    SRCS
        "LDR/LDR_sensor.cpp"
        "PowerMeter/PowerMeter_Sensor.cpp"

    INCLUDE_DIRS
        "."
    
    REQUIRES
        esp_driver_gpio
        esp_adc
)
