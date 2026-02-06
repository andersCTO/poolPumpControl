#include "driver/gpio.h"
#include "pump_controller.h"
#include "unity.h"
#include "unity_fixture.h"
#include <string.h>

TEST_GROUP(pump_controller_tests);

TEST_SETUP(pump_controller_tests) { mock_gpio_reset(); }

TEST_TEAR_DOWN(pump_controller_tests) {}

TEST(pump_controller_tests, test_init_success) {
    esp_err_t result = pump_controller_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

TEST(pump_controller_tests, test_set_mode_off) {
    esp_err_t result = pump_controller_set_mode(PUMP_MODE_OFF);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    pump_status_t status;
    result = pump_controller_get_status(&status);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(PUMP_MODE_OFF, status.mode);
    TEST_ASSERT_EQUAL(0, status.current_rpm);
    TEST_ASSERT_FALSE(status.is_running);
}

TEST(pump_controller_tests, test_set_mode_night) {
    esp_err_t result = pump_controller_set_mode(PUMP_MODE_NIGHT);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    pump_status_t status;
    result = pump_controller_get_status(&status);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(PUMP_MODE_NIGHT, status.mode);
    TEST_ASSERT_EQUAL(1200, status.current_rpm); // DI4 → 1200 rpm per manual
    TEST_ASSERT_FALSE(status.is_running);
}

TEST(pump_controller_tests, test_set_mode_day) {
    esp_err_t result = pump_controller_set_mode(PUMP_MODE_DAY);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    pump_status_t status;
    result = pump_controller_get_status(&status);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(PUMP_MODE_DAY, status.mode);
    TEST_ASSERT_EQUAL(2400, status.current_rpm); // DI3 → 2400 rpm per manual
    TEST_ASSERT_FALSE(status.is_running);
}

TEST(pump_controller_tests, test_set_mode_backwash) {
    esp_err_t result = pump_controller_set_mode(PUMP_MODE_BACKWASH);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    pump_status_t status;
    result = pump_controller_get_status(&status);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(PUMP_MODE_BACKWASH, status.mode);
    TEST_ASSERT_EQUAL(2900, status.current_rpm);
    TEST_ASSERT_FALSE(status.is_running);
}

TEST(pump_controller_tests, test_invalid_mode) {
    esp_err_t result = pump_controller_set_mode((pump_mode_t)99);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

TEST(pump_controller_tests, test_start_in_off_mode) {
    pump_controller_set_mode(PUMP_MODE_OFF);

    esp_err_t result = pump_controller_start();
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);

    pump_status_t status;
    pump_controller_get_status(&status);
    TEST_ASSERT_FALSE(status.is_running);
}

TEST(pump_controller_tests, test_start_in_valid_mode) {
    pump_controller_set_mode(PUMP_MODE_DAY);

    esp_err_t result = pump_controller_start();
    TEST_ASSERT_EQUAL(ESP_OK, result);

    pump_status_t status;
    pump_controller_get_status(&status);
    TEST_ASSERT_TRUE(status.is_running);
    TEST_ASSERT_EQUAL(PUMP_MODE_DAY, status.mode);
}

TEST(pump_controller_tests, test_stop_pump) {
    pump_controller_set_mode(PUMP_MODE_DAY);
    pump_controller_start();

    pump_status_t status;
    pump_controller_get_status(&status);
    TEST_ASSERT_TRUE(status.is_running);

    esp_err_t result = pump_controller_stop();
    TEST_ASSERT_EQUAL(ESP_OK, result);

    pump_controller_get_status(&status);
    TEST_ASSERT_FALSE(status.is_running);
    TEST_ASSERT_EQUAL(PUMP_MODE_OFF, status.mode);
    TEST_ASSERT_EQUAL(0, status.current_rpm);
}

TEST(pump_controller_tests, test_get_status_null_pointer) {
    esp_err_t result = pump_controller_get_status(NULL);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

TEST(pump_controller_tests, test_run_backwash) {
    esp_err_t result = pump_controller_run_backwash(10);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    pump_status_t status;
    pump_controller_get_status(&status);
    TEST_ASSERT_EQUAL(PUMP_MODE_BACKWASH, status.mode);
    TEST_ASSERT_EQUAL(2900, status.current_rpm);
    TEST_ASSERT_TRUE(status.is_running);
}

TEST_GROUP_RUNNER(pump_controller_tests) {
    RUN_TEST_CASE(pump_controller_tests, test_init_success);
    RUN_TEST_CASE(pump_controller_tests, test_set_mode_off);
    RUN_TEST_CASE(pump_controller_tests, test_set_mode_night);
    RUN_TEST_CASE(pump_controller_tests, test_set_mode_day);
    RUN_TEST_CASE(pump_controller_tests, test_set_mode_backwash);
    RUN_TEST_CASE(pump_controller_tests, test_invalid_mode);
    RUN_TEST_CASE(pump_controller_tests, test_start_in_off_mode);
    RUN_TEST_CASE(pump_controller_tests, test_start_in_valid_mode);
    RUN_TEST_CASE(pump_controller_tests, test_stop_pump);
    RUN_TEST_CASE(pump_controller_tests, test_get_status_null_pointer);
    RUN_TEST_CASE(pump_controller_tests, test_run_backwash);
}
