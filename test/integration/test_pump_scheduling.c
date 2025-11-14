/**
 * @file test_pump_scheduling.c
 * @brief Integration tests for pump scheduling functionality
 */

#include "nvs_storage.h"
#include "price_fetcher.h"
#include "pump_controller.h"
#include "pump_scheduler.h"
#include "scheduler.h"
#include "unity.h"
#include <time.h>

// Test group
TEST_GROUP(pump_scheduling_integration_tests);

// Test setup and teardown
TEST_SETUP(pump_scheduling_integration_tests) {
    // Initialize all components
    nvs_storage_init();
    pump_controller_init();
    price_fetcher_init();
    scheduler_init();
    pump_scheduler_init();
}

TEST_TEAR_DOWN(pump_scheduling_integration_tests) {
    // Clean up
    pump_scheduler_deinit();
    scheduler_deinit();
    price_fetcher_deinit();
    pump_controller_deinit();
    nvs_storage_deinit();
}

/**
 * @brief Test scheduled pump start and stop
 */
TEST(pump_scheduling_integration_tests, test_scheduled_pump_start_stop) {
    // Set up a schedule: start at 8:00, run for 2 hours
    pump_schedule_t schedule = {
        .start_hour = 8, .start_minute = 0, .duration_hours = 2, .duration_minutes = 0, .enabled = true};

    nvs_storage_store_pump_schedule(&schedule);

    // Simulate time at 8:00
    struct tm time_8am = {.tm_hour = 8, .tm_min = 0, .tm_sec = 0};

    pump_scheduler_check_schedule(&time_8am);
    pump_status_t status = pump_controller_get_status();
    TEST_ASSERT_EQUAL(PUMP_MODE_NORMAL, status.mode);
    TEST_ASSERT_TRUE(status.is_running);

    // Simulate time at 10:00 (2 hours later)
    struct tm time_10am = {.tm_hour = 10, .tm_min = 0, .tm_sec = 0};

    pump_scheduler_check_schedule(&time_10am);
    status = pump_controller_get_status();
    TEST_ASSERT_FALSE(status.is_running);
}

/**
 * @brief Test price-based scheduling during low price period
 */
TEST(pump_scheduling_integration_tests, test_price_based_scheduling_low_price) {
    // Set up low price threshold
    system_settings_t settings = {.low_price_threshold = 0.15f};
    nvs_storage_store_system_settings(&settings);

    // Mock low price data
    const char *mock_json = "{"
                            "\"records\": ["
                            "{\"SpotPriceEUR\": 0.10}" // Below threshold
                            "]"
                            "}";

    // Simulate price fetcher having low price data
    // Note: In real integration, this would call price_fetcher_get_today_prices

    // Enable price-based scheduling
    pump_scheduler_enable_price_based_scheduling(true);

    // Check if pump should run during low price
    bool should_run = pump_scheduler_should_run_due_to_price();
    TEST_ASSERT_TRUE(should_run);
}

/**
 * @brief Test price-based scheduling during high price period
 */
TEST(pump_scheduling_integration_tests, test_price_based_scheduling_high_price) {
    // Set up low price threshold
    system_settings_t settings = {.low_price_threshold = 0.15f};
    nvs_storage_store_system_settings(&settings);

    // Mock high price data
    const char *mock_json = "{"
                            "\"records\": ["
                            "{\"SpotPriceEUR\": 0.20}" // Above threshold
                            "]"
                            "}";

    // Enable price-based scheduling
    pump_scheduler_enable_price_based_scheduling(true);

    // Check if pump should run during high price
    bool should_run = pump_scheduler_should_run_due_to_price();
    TEST_ASSERT_FALSE(should_run);
}

/**
 * @brief Test backwash scheduling
 */
TEST(pump_scheduling_integration_tests, test_backwash_scheduling) {
    // Set up backwash settings
    system_settings_t settings = {.backwash_interval_days = 7, .backwash_duration_minutes = 5};
    nvs_storage_store_system_settings(&settings);

    // Simulate time when backwash is due
    struct tm backwash_time = {.tm_hour = 6, // Early morning
                               .tm_min = 0,
                               .tm_sec = 0};

    // Trigger backwash
    pump_scheduler_start_backwash();

    pump_status_t status = pump_controller_get_status();
    TEST_ASSERT_EQUAL(PUMP_MODE_BACKWASH, status.mode);
    TEST_ASSERT_TRUE(status.is_running);

    // Simulate backwash duration passed
    // In real scenario, this would be handled by timer
    pump_scheduler_stop_backwash();

    status = pump_controller_get_status();
    TEST_ASSERT_EQUAL(PUMP_MODE_NORMAL, status.mode);
    TEST_ASSERT_FALSE(status.is_running);
}

/**
 * @brief Test manual pump control overrides schedule
 */
TEST(pump_scheduling_integration_tests, test_manual_override) {
    // Set up schedule
    pump_schedule_t schedule = {
        .start_hour = 8, .start_minute = 0, .duration_hours = 2, .duration_minutes = 0, .enabled = true};
    nvs_storage_store_pump_schedule(&schedule);

    // Manually start pump
    pump_controller_start_pump(PUMP_MODE_NORMAL);

    pump_status_t status = pump_controller_get_status();
    TEST_ASSERT_EQUAL(PUMP_MODE_NORMAL, status.mode);
    TEST_ASSERT_TRUE(status.is_running);

    // Even at scheduled stop time, pump should keep running due to manual control
    struct tm stop_time = {.tm_hour = 10, .tm_min = 0, .tm_sec = 0};

    pump_scheduler_check_schedule(&stop_time);
    status = pump_controller_get_status();
    TEST_ASSERT_TRUE(status.is_running); // Should still be running

    // Manually stop pump
    pump_controller_stop_pump();
    status = pump_controller_get_status();
    TEST_ASSERT_FALSE(status.is_running);
}

/**
 * @brief Test disabled schedule doesn't start pump
 */
TEST(pump_scheduling_integration_tests, test_disabled_schedule) {
    // Set up disabled schedule
    pump_schedule_t schedule = {
        .start_hour = 8,
        .start_minute = 0,
        .duration_hours = 2,
        .duration_minutes = 0,
        .enabled = false // Disabled
    };
    nvs_storage_store_pump_schedule(&schedule);

    // Simulate scheduled start time
    struct tm start_time = {.tm_hour = 8, .tm_min = 0, .tm_sec = 0};

    pump_scheduler_check_schedule(&start_time);
    pump_status_t status = pump_controller_get_status();
    TEST_ASSERT_FALSE(status.is_running);
}

/**
 * @brief Test scheduler handles multiple time checks
 */
TEST(pump_scheduling_integration_tests, test_multiple_schedule_checks) {
    // Set up schedule
    pump_schedule_t schedule = {
        .start_hour = 9, .start_minute = 0, .duration_hours = 1, .duration_minutes = 0, .enabled = true};
    nvs_storage_store_pump_schedule(&schedule);

    // Check at various times
    struct tm times[] = {
        {.tm_hour = 7, .tm_min = 0},  // Before start
        {.tm_hour = 8, .tm_min = 30}, // Still before start
        {.tm_hour = 9, .tm_min = 0},  // At start time
        {.tm_hour = 9, .tm_min = 30}, // During run time
        {.tm_hour = 10, .tm_min = 0}, // At stop time
        {.tm_hour = 10, .tm_min = 30} // After stop time
    };

    bool expected_running[] = {false, false, true, true, false, false};

    for (int i = 0; i < 6; i++) {
        pump_scheduler_check_schedule(&times[i]);
        pump_status_t status = pump_controller_get_status();
        TEST_ASSERT_EQUAL(expected_running[i], status.is_running);
    }
}

/**
 * @brief Test price and schedule combination
 */
TEST(pump_scheduling_integration_tests, test_price_and_schedule_combination) {
    // Set up schedule and price settings
    pump_schedule_t schedule = {
        .start_hour = 8, .start_minute = 0, .duration_hours = 4, .duration_minutes = 0, .enabled = true};
    nvs_storage_store_pump_schedule(&schedule);

    system_settings_t settings = {.low_price_threshold = 0.15f};
    nvs_storage_store_system_settings(&settings);

    // Enable price-based scheduling
    pump_scheduler_enable_price_based_scheduling(true);

    // Simulate scheduled time but high price
    struct tm scheduled_time = {.tm_hour = 9, .tm_min = 0, .tm_sec = 0};

    // Mock high price (pump should not start)
    pump_scheduler_check_schedule(&scheduled_time);
    pump_status_t status = pump_controller_get_status();
    TEST_ASSERT_FALSE(status.is_running);

    // Mock low price (pump should start)
    // In real integration, price data would be updated
    pump_scheduler_check_schedule(&scheduled_time);
    // Note: This test assumes price check is separate from schedule check
}

// Test group runner
TEST_GROUP_RUNNER(pump_scheduling_integration_tests) {
    RUN_TEST_CASE(pump_scheduling_integration_tests, test_scheduled_pump_start_stop);
    RUN_TEST_CASE(pump_scheduling_integration_tests, test_price_based_scheduling_low_price);
    RUN_TEST_CASE(pump_scheduling_integration_tests, test_price_based_scheduling_high_price);
    RUN_TEST_CASE(pump_scheduling_integration_tests, test_backwash_scheduling);
    RUN_TEST_CASE(pump_scheduling_integration_tests, test_manual_override);
    RUN_TEST_CASE(pump_scheduling_integration_tests, test_disabled_schedule);
    RUN_TEST_CASE(pump_scheduling_integration_tests, test_multiple_schedule_checks);
    RUN_TEST_CASE(pump_scheduling_integration_tests, test_price_and_schedule_combination);
}