/**
 * @file test_nvs_storage.c
 * @brief Unit tests for NVS storage component
 */

#include "nvs_storage.h"
#include "unity.h"
#include <string.h>

// Test group
TEST_GROUP(nvs_storage_tests);

// Test setup and teardown
TEST_SETUP(nvs_storage_tests) {
    // Reset NVS state before each test
    nvs_storage_deinit();
}

TEST_TEAR_DOWN(nvs_storage_tests) {
    // Clean up after each test
}

/**
 * @brief Test NVS initialization
 */
TEST(nvs_storage_tests, test_init_success) {
    esp_err_t result = nvs_storage_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

/**
 * @brief Test NVS deinitialization
 */
TEST(nvs_storage_tests, test_deinit_success) {
    nvs_storage_init();
    esp_err_t result = nvs_storage_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

/**
 * @brief Test storing and retrieving pump schedule
 */
TEST(nvs_storage_tests, test_store_and_get_pump_schedule) {
    nvs_storage_init();

    pump_schedule_t schedule = {
        .start_hour = 8, .start_minute = 30, .duration_hours = 4, .duration_minutes = 15, .enabled = true};

    // Store schedule
    esp_err_t store_result = nvs_storage_store_pump_schedule(&schedule);
    TEST_ASSERT_EQUAL(ESP_OK, store_result);

    // Retrieve schedule
    pump_schedule_t retrieved_schedule;
    esp_err_t get_result = nvs_storage_get_pump_schedule(&retrieved_schedule);
    TEST_ASSERT_EQUAL(ESP_OK, get_result);

    // Verify data
    TEST_ASSERT_EQUAL(schedule.start_hour, retrieved_schedule.start_hour);
    TEST_ASSERT_EQUAL(schedule.start_minute, retrieved_schedule.start_minute);
    TEST_ASSERT_EQUAL(schedule.duration_hours, retrieved_schedule.duration_hours);
    TEST_ASSERT_EQUAL(schedule.duration_minutes, retrieved_schedule.duration_minutes);
    TEST_ASSERT_EQUAL(schedule.enabled, retrieved_schedule.enabled);
}

/**
 * @brief Test storing and retrieving WiFi configuration
 */
TEST(nvs_storage_tests, test_store_and_get_wifi_config) {
    nvs_storage_init();

    wifi_config_t config = {.ssid = "TestNetwork", .password = "TestPassword123"};

    // Store WiFi config
    esp_err_t store_result = nvs_storage_store_wifi_config(&config);
    TEST_ASSERT_EQUAL(ESP_OK, store_result);

    // Retrieve WiFi config
    wifi_config_t retrieved_config;
    esp_err_t get_result = nvs_storage_get_wifi_config(&retrieved_config);
    TEST_ASSERT_EQUAL(ESP_OK, get_result);

    // Verify data
    TEST_ASSERT_EQUAL_STRING(config.ssid, retrieved_config.ssid);
    TEST_ASSERT_EQUAL_STRING(config.password, retrieved_config.password);
}

/**
 * @brief Test storing and retrieving system settings
 */
TEST(nvs_storage_tests, test_store_and_get_system_settings) {
    nvs_storage_init();

    system_settings_t settings = {.backwash_interval_days = 7,
                                  .backwash_duration_minutes = 5,
                                  .low_price_threshold = 0.12f,
                                  .timezone_offset = 2};

    // Store settings
    esp_err_t store_result = nvs_storage_store_system_settings(&settings);
    TEST_ASSERT_EQUAL(ESP_OK, store_result);

    // Retrieve settings
    system_settings_t retrieved_settings;
    esp_err_t get_result = nvs_storage_get_system_settings(&retrieved_settings);
    TEST_ASSERT_EQUAL(ESP_OK, get_result);

    // Verify data
    TEST_ASSERT_EQUAL(settings.backwash_interval_days, retrieved_settings.backwash_interval_days);
    TEST_ASSERT_EQUAL(settings.backwash_duration_minutes, retrieved_settings.backwash_duration_minutes);
    TEST_ASSERT_EQUAL_FLOAT(settings.low_price_threshold, retrieved_settings.low_price_threshold);
    TEST_ASSERT_EQUAL(settings.timezone_offset, retrieved_settings.timezone_offset);
}

/**
 * @brief Test retrieving data when NVS is not initialized
 */
TEST(nvs_storage_tests, test_get_without_init) {
    pump_schedule_t schedule;
    esp_err_t result = nvs_storage_get_pump_schedule(&schedule);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

/**
 * @brief Test storing data when NVS is not initialized
 */
TEST(nvs_storage_tests, test_store_without_init) {
    pump_schedule_t schedule = {0};
    esp_err_t result = nvs_storage_store_pump_schedule(&schedule);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

/**
 * @brief Test default pump schedule values
 */
TEST(nvs_storage_tests, test_default_pump_schedule) {
    nvs_storage_init();

    pump_schedule_t schedule;
    esp_err_t result = nvs_storage_get_pump_schedule(&schedule);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    // Check default values
    TEST_ASSERT_EQUAL(8, schedule.start_hour);     // Default start hour
    TEST_ASSERT_EQUAL(0, schedule.start_minute);   // Default start minute
    TEST_ASSERT_EQUAL(4, schedule.duration_hours); // Default duration
    TEST_ASSERT_EQUAL(0, schedule.duration_minutes);
    TEST_ASSERT_TRUE(schedule.enabled); // Default enabled
}

/**
 * @brief Test default system settings values
 */
TEST(nvs_storage_tests, test_default_system_settings) {
    nvs_storage_init();

    system_settings_t settings;
    esp_err_t result = nvs_storage_get_system_settings(&settings);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    // Check default values
    TEST_ASSERT_EQUAL(7, settings.backwash_interval_days);
    TEST_ASSERT_EQUAL(5, settings.backwash_duration_minutes);
    TEST_ASSERT_EQUAL_FLOAT(0.15f, settings.low_price_threshold);
    TEST_ASSERT_EQUAL(0, settings.timezone_offset);
}

/**
 * @brief Test WiFi config with empty strings
 */
TEST(nvs_storage_tests, test_wifi_config_empty_strings) {
    nvs_storage_init();

    wifi_config_t config = {.ssid = "", .password = ""};

    esp_err_t store_result = nvs_storage_store_wifi_config(&config);
    TEST_ASSERT_EQUAL(ESP_OK, store_result);

    wifi_config_t retrieved_config;
    esp_err_t get_result = nvs_storage_get_wifi_config(&retrieved_config);
    TEST_ASSERT_EQUAL(ESP_OK, get_result);

    TEST_ASSERT_EQUAL_STRING("", retrieved_config.ssid);
    TEST_ASSERT_EQUAL_STRING("", retrieved_config.password);
}

/**
 * @brief Test pump schedule with boundary values
 */
TEST(nvs_storage_tests, test_pump_schedule_boundary_values) {
    nvs_storage_init();

    pump_schedule_t schedule = {.start_hour = 23,     // Max hour
                                .start_minute = 59,   // Max minute
                                .duration_hours = 12, // Long duration
                                .duration_minutes = 30,
                                .enabled = false};

    esp_err_t store_result = nvs_storage_store_pump_schedule(&schedule);
    TEST_ASSERT_EQUAL(ESP_OK, store_result);

    pump_schedule_t retrieved_schedule;
    esp_err_t get_result = nvs_storage_get_pump_schedule(&retrieved_schedule);
    TEST_ASSERT_EQUAL(ESP_OK, get_result);

    TEST_ASSERT_EQUAL(23, retrieved_schedule.start_hour);
    TEST_ASSERT_EQUAL(59, retrieved_schedule.start_minute);
    TEST_ASSERT_EQUAL(12, retrieved_schedule.duration_hours);
    TEST_ASSERT_EQUAL(30, retrieved_schedule.duration_minutes);
    TEST_ASSERT_FALSE(retrieved_schedule.enabled);
}

/**
 * @brief Test system settings with boundary values
 */
TEST(nvs_storage_tests, test_system_settings_boundary_values) {
    nvs_storage_init();

    system_settings_t settings = {
        .backwash_interval_days = 30,    // Max interval
        .backwash_duration_minutes = 15, // Max duration
        .low_price_threshold = 0.50f,    // High threshold
        .timezone_offset = 12            // Max offset
    };

    esp_err_t store_result = nvs_storage_store_system_settings(&settings);
    TEST_ASSERT_EQUAL(ESP_OK, store_result);

    system_settings_t retrieved_settings;
    esp_err_t get_result = nvs_storage_get_system_settings(&retrieved_settings);
    TEST_ASSERT_EQUAL(ESP_OK, get_result);

    TEST_ASSERT_EQUAL(30, retrieved_settings.backwash_interval_days);
    TEST_ASSERT_EQUAL(15, retrieved_settings.backwash_duration_minutes);
    TEST_ASSERT_EQUAL_FLOAT(0.50f, retrieved_settings.low_price_threshold);
    TEST_ASSERT_EQUAL(12, retrieved_settings.timezone_offset);
}

// Test group runner
TEST_GROUP_RUNNER(nvs_storage_tests) {
    RUN_TEST_CASE(nvs_storage_tests, test_init_success);
    RUN_TEST_CASE(nvs_storage_tests, test_deinit_success);
    RUN_TEST_CASE(nvs_storage_tests, test_store_and_get_pump_schedule);
    RUN_TEST_CASE(nvs_storage_tests, test_store_and_get_wifi_config);
    RUN_TEST_CASE(nvs_storage_tests, test_store_and_get_system_settings);
    RUN_TEST_CASE(nvs_storage_tests, test_get_without_init);
    RUN_TEST_CASE(nvs_storage_tests, test_store_without_init);
    RUN_TEST_CASE(nvs_storage_tests, test_default_pump_schedule);
    RUN_TEST_CASE(nvs_storage_tests, test_default_system_settings);
    RUN_TEST_CASE(nvs_storage_tests, test_wifi_config_empty_strings);
    RUN_TEST_CASE(nvs_storage_tests, test_pump_schedule_boundary_values);
    RUN_TEST_CASE(nvs_storage_tests, test_system_settings_boundary_values);
}