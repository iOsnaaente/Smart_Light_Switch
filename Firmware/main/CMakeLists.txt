file(GLOB IMAGE_SOURCES "${CMAKE_CURRENT_LIST_DIR}/images/image_*.c")

idf_component_register(
    SRCS
        "main.cpp"
        ${IMAGE_SOURCES}

    INCLUDE_DIRS
        "."

    REQUIRES
        esp_driver_gpio
        esp_driver_spi
        esp_timer
        esp_lcd
        ili9340
)

