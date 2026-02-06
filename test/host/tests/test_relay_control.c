#include "driver/gpio.h"
#include "relay_control.h"
#include "unity.h"
#include "unity_fixture.h"
#include <string.h>

TEST_GROUP(relay_control_tests);

TEST_SETUP(relay_control_tests) { mock_gpio_reset(); }

TEST_TEAR_DOWN(relay_control_tests) {}

TEST(relay_control_tests, test_init_success) {
    esp_err_t result = relay_control_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

TEST(relay_control_tests, test_set_relay_1_on) {
    esp_err_t result = relay_control_set(RELAY_1, true);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    bool state;
    result = relay_control_get(RELAY_1, &state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_TRUE(state);
}

TEST(relay_control_tests, test_set_relay_1_off) {
    esp_err_t result = relay_control_set(RELAY_1, false);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    bool state;
    result = relay_control_get(RELAY_1, &state);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_FALSE(state);
}

TEST(relay_control_tests, test_all_off) {
    relay_control_set(RELAY_1, true);
    relay_control_set(RELAY_2, true);
    relay_control_set(RELAY_3, true);

    bool state;
    relay_control_get(RELAY_1, &state);
    TEST_ASSERT_TRUE(state);
    relay_control_get(RELAY_2, &state);
    TEST_ASSERT_TRUE(state);
    relay_control_get(RELAY_3, &state);
    TEST_ASSERT_TRUE(state);

    esp_err_t result = relay_control_all_off();
    TEST_ASSERT_EQUAL(ESP_OK, result);

    relay_control_get(RELAY_1, &state);
    TEST_ASSERT_FALSE(state);
    relay_control_get(RELAY_2, &state);
    TEST_ASSERT_FALSE(state);
    relay_control_get(RELAY_3, &state);
    TEST_ASSERT_FALSE(state);
    relay_control_get(RELAY_4, &state);
    TEST_ASSERT_FALSE(state);
}

TEST(relay_control_tests, test_invalid_relay_number) {
    esp_err_t result = relay_control_set((relay_num_t)RELAY_MAX, true);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

TEST(relay_control_tests, test_get_null_pointer) {
    esp_err_t result = relay_control_get(RELAY_1, NULL);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

TEST(relay_control_tests, test_pump_mode_off) {
    esp_err_t result = relay_control_set_pump_mode(0);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    bool state;
    relay_control_get(RELAY_1, &state);
    TEST_ASSERT_FALSE(state);
    relay_control_get(RELAY_2, &state);
    TEST_ASSERT_FALSE(state);
    relay_control_get(RELAY_3, &state);
    TEST_ASSERT_FALSE(state);
}

TEST(relay_control_tests, test_pump_mode_night) {
    esp_err_t result = relay_control_set_pump_mode(1);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    // Night mode uses Relay 3 → DI4 → 1200 rpm (per manual)
    bool state;
    relay_control_get(RELAY_1, &state);
    TEST_ASSERT_FALSE(state);
    relay_control_get(RELAY_2, &state);
    TEST_ASSERT_FALSE(state);
    relay_control_get(RELAY_3, &state);
    TEST_ASSERT_TRUE(state);
}

TEST(relay_control_tests, test_pump_mode_day) {
    esp_err_t result = relay_control_set_pump_mode(2);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    // Day mode uses Relay 2 → DI3 → 2400 rpm (per manual)
    bool state;
    relay_control_get(RELAY_1, &state);
    TEST_ASSERT_FALSE(state);
    relay_control_get(RELAY_2, &state);
    TEST_ASSERT_TRUE(state);
    relay_control_get(RELAY_3, &state);
    TEST_ASSERT_FALSE(state);
}

TEST(relay_control_tests, test_pump_mode_backwash) {
    esp_err_t result = relay_control_set_pump_mode(3);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    // Backwash mode uses Relay 1 → DI2 → 2900 rpm (per manual)
    bool state;
    relay_control_get(RELAY_1, &state);
    TEST_ASSERT_TRUE(state);
    relay_control_get(RELAY_2, &state);
    TEST_ASSERT_FALSE(state);
    relay_control_get(RELAY_3, &state);
    TEST_ASSERT_FALSE(state);
}

TEST(relay_control_tests, test_invalid_pump_mode) {
    esp_err_t result = relay_control_set_pump_mode(99);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

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
