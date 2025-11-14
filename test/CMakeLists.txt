# Test Configuration for ESP32 Pool Pump Controller
cmake_minimum_required(VERSION 3.16)

# Set the test component requirements
set(TEST_COMPONENTS
    main
    wifi_manager
    price_fetcher
    pump_controller
    relay_control
    nvs_storage
    unity
    cmock
)

# Include ESP-IDF test framework
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Set test-specific configurations
set(TEST_EXCLUDE_COMPONENTS "" CACHE STRING "Components to exclude from test build")

# Create test executable
idf_component_register(
    SRCS "test_main.c"
    INCLUDE_DIRS "."
    REQUIRES ${TEST_COMPONENTS}
)

# Add test components
add_subdirectory(unit)
add_subdirectory(integration)

# Enable testing
enable_testing()

# Add test targets
add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "test_.*_unit"
    DEPENDS pool_pump_controller.elf
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running unit tests"
)

add_custom_target(run_integration_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "test_.*_integration"
    DEPENDS pool_pump_controller.elf
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running integration tests"
)

add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS pool_pump_controller.elf
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests"
)