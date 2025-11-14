# Integration Tests Configuration
idf_component_register(
    SRCS
        "test_pump_scheduling.c"
        "test_full_system.c"
    INCLUDE_DIRS "."
    REQUIRES
        unity
        main
        wifi_manager
        price_fetcher
        pump_controller
        relay_control
        nvs_storage
)

# Add test definitions
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    INTEGRATION_TESTING=1
    ESP_PLATFORM=1
)

# Add test-specific include paths
target_include_directories(${COMPONENT_LIB} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../mocks
)