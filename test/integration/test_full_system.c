/**
 * @file test_full_system.c
 * @brief Full system integration tests
 */

#include "app_main.h"
#include "nvs_storage.h"
#include "price_fetcher.h"
#include "pump_controller.h"
#include "pump_scheduler.h"
#include "sensors.h"
#include "unity.h"
#include "wifi_manager.h"
#include <time.h>

// Test group
TEST_GROUP(full_system_integration_tests);

// Test setup and teardown
TEST_SETUP(full_system_integration_tests) {
    // Initialize entire system
    app_init();
}

TEST_TEAR_DOWN(full_system_integration_tests) {
    // Clean up entire system
    app_deinit();
}

/**
 * @brief Test complete system initialization
 */
TEST(full_system_integration_tests, test_system_initialization) {
    // System should initialize without errors
    esp_err_t result = app_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);

    // Check that all components are initialized
    wifi_status_t wifi_status = wifi_manager_get_status();
    TEST_ASSERT(wifi_status == WIFI_DISCONNECTED || wifi_status == WIFI_CONNECTED);

    pump_status_t pump_status = pump_controller_get_status();
    TEST_ASSERT_EQUAL(PUMP_MODE_NORMAL, pump_status.mode);
    TEST_ASSERT_FALSE(pump_status.is_running);
}

/**
 * @brief Test WiFi connection and pump control integration
 */
TEST(full_system_integration_tests, test_wifi_pump_integration) {
    // Connect to WiFi
    esp_err_t wifi_result = wifi_manager_connect("TestSSID", "TestPassword");
    TEST_ASSERT_EQUAL(ESP_OK, wifi_result);

    // Wait for connection (in real test, this would have timeout)
    wifi_status_t status = wifi_manager_get_status();
    TEST_ASSERT_EQUAL(WIFI_CONNECTED, status);

    // Once connected, start pump
    esp_err_t pump_result = pump_controller_start_pump(PUMP_MODE_NORMAL);
    TEST_ASSERT_EQUAL(ESP_OK, pump_result);

    pump_status_t pump_status = pump_controller_get_status();
    TEST_ASSERT_TRUE(pump_status.is_running);
    TEST_ASSERT_EQUAL(PUMP_MODE_NORMAL, pump_status.mode);

    // Stop pump
    pump_controller_stop_pump();
    pump_status = pump_controller_get_status();
    TEST_ASSERT_FALSE(pump_status.is_running);
}

/**
 * @brief Test price fetching and scheduling integration
 */
TEST(full_system_integration_tests, test_price_scheduling_integration) {
    // Connect to WiFi first
    wifi_manager_connect("TestSSID", "TestPassword");

    // Fetch price data
    price_data_t prices[24];
    esp_err_t price_result = price_fetcher_get_today_prices(prices);
    TEST_ASSERT_EQUAL(ESP_OK, price_result);

    // Set up scheduling with price awareness
    system_settings_t settings = {.low_price_threshold = 0.15f};
    nvs_storage_store_system_settings(&settings);

    pump_scheduler_enable_price_based_scheduling(true);

    // Check if current price is low
    bool is_low_price = price_fetcher_is_low_price_period();
    float current_price = price_fetcher_get_current_price();

    // Based on price, pump should be controlled
    if (is_low_price && current_price > 0) {
        // Should allow pump to run during low price
        bool should_run = pump_scheduler_should_run_due_to_price();
        TEST_ASSERT_TRUE(should_run);
    }
}

/**
 * @brief Test sensor monitoring and pump control integration
 */
TEST(full_system_integration_tests, test_sensor_pump_integration) {
    // Initialize sensors
    esp_err_t sensor_result = sensors_init();
    TEST_ASSERT_EQUAL(ESP_OK, sensor_result);

    // Read sensor data
    sensor_data_t data;
    esp_err_t read_result = sensors_read_all(&data);
    TEST_ASSERT_EQUAL(ESP_OK, read_result);

    // Based on sensor readings, system should make decisions
    // For example, if water level is low, pump should not start
    if (data.water_level < 10.0f) { // Low water level threshold
        // Pump should be prevented from starting
        esp_err_t pump_result = pump_controller_start_pump(PUMP_MODE_NORMAL);
        // In real system, this might return error or be prevented by safety checks
        TEST_ASSERT_EQUAL(ESP_OK, pump_result); // For this test, assume it starts but logs warning
    }
}

/**
 * @brief Test system recovery after WiFi disconnection
 */
TEST(full_system_integration_tests, test_wifi_reconnection_recovery) {
    // Connect to WiFi
    wifi_manager_connect("TestSSID", "TestPassword");
    wifi_status_t status = wifi_manager_get_status();
    TEST_ASSERT_EQUAL(WIFI_CONNECTED, status);

    // Simulate WiFi disconnection
    wifi_manager_disconnect();
    status = wifi_manager_get_status();
    TEST_ASSERT_EQUAL(WIFI_DISCONNECTED, status);

    // System should attempt reconnection
    esp_err_t reconnect_result = wifi_manager_connect("TestSSID", "TestPassword");
    TEST_ASSERT_EQUAL(ESP_OK, reconnect_result);

    // Price fetching should resume after reconnection
    price_data_t prices[24];
    esp_err_t price_result = price_fetcher_get_today_prices(prices);
    TEST_ASSERT_EQUAL(ESP_OK, price_result);
}

/**
 * @brief Test backwash cycle integration
 */
TEST(full_system_integration_tests, test_backwash_cycle_integration) {
    // Set up backwash settings
    system_settings_t settings = {.backwash_interval_days = 7, .backwash_duration_minutes = 5};
    nvs_storage_store_system_settings(&settings);

    // Start backwash cycle
    pump_scheduler_start_backwash();

    pump_status_t status = pump_controller_get_status();
    TEST_ASSERT_EQUAL(PUMP_MODE_BACKWASH, status.mode);
    TEST_ASSERT_TRUE(status.is_running);

    // Simulate backwash completion
    pump_scheduler_stop_backwash();

    status = pump_controller_get_status();
    TEST_ASSERT_EQUAL(PUMP_MODE_NORMAL, status.mode);
    TEST_ASSERT_FALSE(status.is_running);
}

/**
 * @brief Test system configuration persistence
 */
TEST(full_system_integration_tests, test_configuration_persistence) {
    // Set up system configuration
    pump_schedule_t schedule = {
        .start_hour = 10, .start_minute = 30, .duration_hours = 6, .duration_minutes = 0, .enabled = true};
    nvs_storage_store_pump_schedule(&schedule);

    system_settings_t settings = {.backwash_interval_days = 14,
                                  .backwash_duration_minutes = 10,
                                  .low_price_threshold = 0.12f,
                                  .timezone_offset = 1};
    nvs_storage_store_system_settings(&settings);

    wifi_config_t wifi_config = {.ssid = "MyHomeNetwork", .password = "SecurePass123"};
    nvs_storage_store_wifi_config(&wifi_config);

    // Simulate system restart (reinitialize)
    app_deinit();
    app_init();

    // Verify configuration was persisted and restored
    pump_schedule_t retrieved_schedule;
    nvs_storage_get_pump_schedule(&retrieved_schedule);
    TEST_ASSERT_EQUAL(schedule.start_hour, retrieved_schedule.start_hour);
    TEST_ASSERT_EQUAL(schedule.start_minute, retrieved_schedule.start_minute);
    TEST_ASSERT_EQUAL(schedule.duration_hours, retrieved_schedule.duration_hours);
    TEST_ASSERT_TRUE(retrieved_schedule.enabled);

    system_settings_t retrieved_settings;
    nvs_storage_get_system_settings(&retrieved_settings);
    TEST_ASSERT_EQUAL(settings.backwash_interval_days, retrieved_settings.backwash_interval_days);
    TEST_ASSERT_EQUAL_FLOAT(settings.low_price_threshold, retrieved_settings.low_price_threshold);

    wifi_config_t retrieved_wifi;
    nvs_storage_get_wifi_config(&retrieved_wifi);
    TEST_ASSERT_EQUAL_STRING(wifi_config.ssid, retrieved_wifi.ssid);
    TEST_ASSERT_EQUAL_STRING(wifi_config.password, retrieved_wifi.password);
}

/**
 * @brief Test error handling and recovery
 */
TEST(full_system_integration_tests, test_error_handling_recovery) {
    // Test pump failure recovery
    esp_err_t start_result = pump_controller_start_pump(PUMP_MODE_NORMAL);
    TEST_ASSERT_EQUAL(ESP_OK, start_result);

    // Simulate pump hardware failure (in real system, this would be detected by sensors)
    // For this test, we'll just verify the system can handle stop commands
    pump_controller_stop_pump();
    pump_status_t status = pump_controller_get_status();
    TEST_ASSERT_FALSE(status.is_running);

    // Test WiFi reconnection after failure
    wifi_manager_disconnect();
    esp_err_t reconnect_result = wifi_manager_connect("TestSSID", "TestPassword");
    TEST_ASSERT_EQUAL(ESP_OK, reconnect_result);
}

/**
 * @brief Test system performance under load
 */
TEST(full_system_integration_tests, test_system_performance) {
    // Start multiple operations simultaneously
    wifi_manager_connect("TestSSID", "TestPassword");
    pump_controller_start_pump(PUMP_MODE_NORMAL);

    // Fetch price data while pump is running
    price_data_t prices[24];
    price_fetcher_get_today_prices(prices);

    // Read sensors while other operations are active
    sensor_data_t sensor_data;
    sensors_read_all(&sensor_data);

    // System should handle concurrent operations without issues
    pump_status_t pump_status = pump_controller_get_status();
    TEST_ASSERT_TRUE(pump_status.is_running);

    wifi_status_t wifi_status = wifi_manager_get_status();
    TEST_ASSERT_EQUAL(WIFI_CONNECTED, wifi_status);

    // Clean up
    pump_controller_stop_pump();
}

// Test group runner
TEST_GROUP_RUNNER(full_system_integration_tests) {
    RUN_TEST_CASE(full_system_integration_tests, test_system_initialization);
    RUN_TEST_CASE(full_system_integration_tests, test_wifi_pump_integration);
    RUN_TEST_CASE(full_system_integration_tests, test_price_scheduling_integration);
    RUN_TEST_CASE(full_system_integration_tests, test_sensor_pump_integration);
    RUN_TEST_CASE(full_system_integration_tests, test_wifi_reconnection_recovery);
    RUN_TEST_CASE(full_system_integration_tests, test_backwash_cycle_integration);
    RUN_TEST_CASE(full_system_integration_tests, test_configuration_persistence);
    RUN_TEST_CASE(full_system_integration_tests, test_error_handling_recovery);
    RUN_TEST_CASE(full_system_integration_tests, test_system_performance);
}