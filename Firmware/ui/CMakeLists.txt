file(GLOB IMAGE_SOURCES "${CMAKE_CURRENT_LIST_DIR}/images/image_*.c")

idf_component_register(
    SRCS
        "ui_stub.cpp"
        "display_manager.cpp"
        ${IMAGE_SOURCES}
    INCLUDE_DIRS
        "."
    REQUIRES
        system
        lvgl
        esp_timer
        esp_driver_spi
        esp_lcd
        ili9340
)
