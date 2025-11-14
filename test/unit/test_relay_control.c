/**
 * @file test_relay_control.c
 * @brief Unit tests for relay control component
 */

#include "mock_driver_gpio.h"
#include "relay_control.h"
#include "unity.h"
#include <string.h>

// Test group
TEST_GROUP(relay_control_tests);

// Test setup and teardown
TEST_SETUP(relay_control_tests) { mock_gpio_reset(); }

TEST_TEAR_DOWN(relay_control_tests) {
    // Clean up after each test
}

/**
 * @brief Test relay control initialization
 */
TEST(relay_control_tests, test_init_success) {
    esp_err_t result = relay_control_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

/**
 * @brief Test setting relay 1 to ON
 */
TEST(relay_control_tests, test_set_relay_1_on) {
    esp_err_t result = relay_control_set(RELAY_1, true);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    bool state;
    result = relay_control_get(RELAY_1, &state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_TRUE(state);
}

/**
 * @brief Test setting relay 1 to OFF
 */
TEST(relay_control_tests, test_set_relay_1_off) {
    esp_err_t result = relay_control_set(RELAY_1, false);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    bool state;
    result = relay_control_get(RELAY_1, &state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_FALSE(state);
}

/**
 * @brief Test setting all relays to OFF
 */
TEST(relay_control_tests, test_all_off) {
    // First set some relays ON
    relay_control_set(RELAY_1, true);
    relay_control_set(RELAY_2, true);
    relay_control_set(RELAY_3, true);

    // Verify they are ON
    bool state;
    relay_control_get(RELAY_1, &state);
    TEST_ASSERT_TRUE(state);
    relay_control_get(RELAY_2, &state);
    TEST_ASSERT_TRUE(state);
    relay_control_get(RELAY_3, &state);
    TEST_ASSERT_TRUE(state);

    // Turn all OFF
    esp_err_t result = relay_control_all_off();
    TEST_ASSERT_EQUAL(ESP_OK, result);

    // Verify all are OFF
    relay_control_get(RELAY_1, &state);
    TEST_ASSERT_FALSE(state);
    relay_control_get(RELAY_2, &state);
    TEST_ASSERT_FALSE(state);
    relay_control_get(RELAY_3, &state);
    TEST_ASSERT_FALSE(state);
    relay_control_get(RELAY_4, &state);
    TEST_ASSERT_FALSE(state);
}

/**
 * @brief Test invalid relay number
 */
TEST(relay_control_tests, test_invalid_relay_number) {
    esp_err_t result = relay_control_set((relay_num_t)RELAY_MAX, true);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

/**
 * @brief Test relay get with NULL pointer
 */
TEST(relay_control_tests, test_get_null_pointer) {
    esp_err_t result = relay_control_get(RELAY_1, NULL);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

/**
 * @brief Test pump mode setting to OFF
 */
TEST(relay_control_tests, test_pump_mode_off) {
    esp_err_t result = relay_control_set_pump_mode(0); // OFF mode
    TEST_ASSERT_EQUAL(ESP_OK, result);

    // All relays should be OFF
    bool state;
    relay_control_get(RELAY_1, &state);
    TEST_ASSERT_FALSE(state);
    relay_control_get(RELAY_2, &state);
    TEST_ASSERT_FALSE(state);
    relay_control_get(RELAY_3, &state);
    TEST_ASSERT_FALSE(state);
}

/**
 * @brief Test pump mode setting to NIGHT (1400 RPM)
 */
TEST(relay_control_tests, test_pump_mode_night) {
    esp_err_t result = relay_control_set_pump_mode(1); // NIGHT mode
    TEST_ASSERT_EQUAL(ESP_OK, result);

    // RELAY_1 should be ON, others OFF
    bool state;
    relay_control_get(RELAY_1, &state);
    TEST_ASSERT_TRUE(state);
    relay_control_get(RELAY_2, &state);
    TEST_ASSERT_FALSE(state);
    relay_control_get(RELAY_3, &state);
    TEST_ASSERT_FALSE(state);
}

/**
 * @brief Test pump mode setting to DAY (2000 RPM)
 */
TEST(relay_control_tests, test_pump_mode_day) {
    esp_err_t result = relay_control_set_pump_mode(2); // DAY mode
    TEST_ASSERT_EQUAL(ESP_OK, result);

    // RELAY_2 should be ON, others OFF
    bool state;
    relay_control_get(RELAY_1, &state);
    TEST_ASSERT_FALSE(state);
    relay_control_get(RELAY_2, &state);
    TEST_ASSERT_TRUE(state);
    relay_control_get(RELAY_3, &state);
    TEST_ASSERT_FALSE(state);
}

/**
 * @brief Test pump mode setting to BACKWASH (2900 RPM)
 */
TEST(relay_control_tests, test_pump_mode_backwash) {
    esp_err_t result = relay_control_set_pump_mode(3); // BACKWASH mode
    TEST_ASSERT_EQUAL(ESP_OK, result);

    // RELAY_3 should be ON, others OFF
    bool state;
    relay_control_get(RELAY_1, &state);
    TEST_ASSERT_FALSE(state);
    relay_control_get(RELAY_2, &state);
    TEST_ASSERT_FALSE(state);
    relay_control_get(RELAY_3, &state);
    TEST_ASSERT_TRUE(state);
}

/**
 * @brief Test invalid pump mode
 */
TEST(relay_control_tests, test_invalid_pump_mode) {
    esp_err_t result = relay_control_set_pump_mode(99); // Invalid mode
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

// Test group runner
TEST_GROUP_RUNNER(relay_control_tests) {
    RUN_TEST_CASE(relay_control_tests, test_init_success);
    RUN_TEST_CASE(relay_control_tests, test_set_relay_1_on);
    RUN_TEST_CASE(relay_control_tests, test_set_relay_1_off);
    RUN_TEST_CASE(relay_control_tests, test_all_off);
    RUN_TEST_CASE(relay_control_tests, test_invalid_relay_number);
    RUN_TEST_CASE(relay_control_tests, test_get_null_pointer);
    RUN_TEST_CASE(relay_control_tests, test_pump_mode_off);
    RUN_TEST_CASE(relay_control_tests, test_pump_mode_night);
    RUN_TEST_CASE(relay_control_tests, test_pump_mode_day);
    RUN_TEST_CASE(relay_control_tests, test_pump_mode_backwash);
    RUN_TEST_CASE(relay_control_tests, test_invalid_pump_mode);
}