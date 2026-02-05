cmake_minimum_required(VERSION 3.14)
project(poolpump_host_tests C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare")

# Root of the project
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# ---- Mock include paths (must come FIRST to shadow ESP-IDF headers) ----
set(MOCK_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

# ---- Project include paths ----
set(PROJECT_INCLUDE_DIRS
    ${PROJECT_ROOT}/include
    ${PROJECT_ROOT}/components/relay_control/include
    ${PROJECT_ROOT}/components/pump_controller/include
    ${PROJECT_ROOT}/components/wifi_manager/include
    ${PROJECT_ROOT}/components/nvs_storage/include
    ${PROJECT_ROOT}/components/price_fetcher/include
    ${PROJECT_ROOT}/components/web_server/include
)

# ---- Vendored libraries ----
set(UNITY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/unity)
set(CJSON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/cjson)

# ---- Unity library ----
add_library(unity STATIC
    ${UNITY_DIR}/unity.c
    ${UNITY_DIR}/unity_fixture.c
)
target_include_directories(unity PUBLIC ${UNITY_DIR})
target_compile_definitions(unity PUBLIC UNITY_FIXTURE_NO_EXTRAS)

# ---- cJSON library ----
add_library(cjson STATIC
    ${CJSON_DIR}/cJSON.c
)
target_include_directories(cjson PUBLIC ${CJSON_DIR})

# ---- Mock implementations ----
add_library(mocks STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/mock_esp_idf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/mock_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/mock_wifi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/mock_http_client.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/mock_http_server.c
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks/mock_nvs.c
)
target_include_directories(mocks PUBLIC ${MOCK_INCLUDE_DIRS})

# ---- Real component source files (compiled against mocks) ----
add_library(components STATIC
    ${PROJECT_ROOT}/components/relay_control/relay_control.c
    ${PROJECT_ROOT}/components/pump_controller/pump_controller.c
    ${PROJECT_ROOT}/components/wifi_manager/wifi_manager.c
    ${PROJECT_ROOT}/components/nvs_storage/nvs_storage.c
    ${PROJECT_ROOT}/components/price_fetcher/price_fetcher.c
    ${PROJECT_ROOT}/components/web_server/web_server.c
    ${PROJECT_ROOT}/main/pump_scheduler.c
    ${PROJECT_ROOT}/main/config.c
)
# Mock headers MUST come first to intercept ESP-IDF includes
target_include_directories(components PUBLIC
    ${MOCK_INCLUDE_DIRS}
    ${PROJECT_INCLUDE_DIRS}
    ${CJSON_DIR}
)
target_link_libraries(components mocks cjson)

# ---- Test executable ----
add_executable(host_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_relay_control.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_pump_controller.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_wifi_manager.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_nvs_storage.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_price_fetcher.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_web_server.c
)
target_include_directories(host_tests PRIVATE
    ${MOCK_INCLUDE_DIRS}
    ${PROJECT_INCLUDE_DIRS}
    ${CJSON_DIR}
)
target_link_libraries(host_tests unity components mocks cjson)

# ---- CTest ----
enable_testing()
add_test(NAME host_tests COMMAND host_tests)
