idf_component_register(
    SRCS
        "main.cpp"

    INCLUDE_DIRS
        "."

    REQUIRES
        controllers
        sensors
        ui
        esp_driver_gpio
        esp_driver_spi
        esp_timer
        esp_lcd
        ili9340
)

