idf_component_register(
    SRCS "bluetooth_config.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash bt esp_wifi
)