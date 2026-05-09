idf_component_register(
    SRCS
        "triac_controller.cpp"

    INCLUDE_DIRS
        "."

    REQUIRES
        esp_driver_gpio
        esp_timer
)
