# Unit Tests Configuration
idf_component_register(
    SRCS
        "test_wifi_manager.c"
        "test_price_fetcher.c"
        "test_pump_controller.c"
        "test_relay_control.c"
        "test_nvs_storage.c"
    INCLUDE_DIRS "."
    REQUIRES
        unity
        wifi_manager
        price_fetcher
        pump_controller
        relay_control
        nvs_storage
        main
)

# Add test definitions
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    UNIT_TESTING=1
    ESP_PLATFORM=1
)

# Add test-specific include paths
target_include_directories(${COMPONENT_LIB} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../mocks
)