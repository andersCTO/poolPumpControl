#include "nvs.h"
#include "nvs_storage.h"
#include "unity.h"
#include "unity_fixture.h"
#include <string.h>

TEST_GROUP(nvs_storage_tests);

TEST_SETUP(nvs_storage_tests) { mock_nvs_reset(); }

TEST_TEAR_DOWN(nvs_storage_tests) {}

TEST(nvs_storage_tests, test_init_success) {
    esp_err_t result = nvs_storage_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

TEST(nvs_storage_tests, test_save_and_load_string) {
    nvs_storage_init();

    esp_err_t result = nvs_storage_save_string("test_key", "test_value");
    TEST_ASSERT_EQUAL(ESP_OK, result);

    char buffer[64] = {0};
    result = nvs_storage_load_string("test_key", buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL_STRING("test_value", buffer);
}

TEST(nvs_storage_tests, test_save_and_load_int) {
    nvs_storage_init();

    esp_err_t result = nvs_storage_save_int("int_key", 42);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    int32_t value = 0;
    result = nvs_storage_load_int("int_key", &value);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(42, value);
}

TEST(nvs_storage_tests, test_wifi_credentials) {
    nvs_storage_init();

    esp_err_t result = nvs_storage_set_wifi_credentials("MySSID", "MyPassword");
    TEST_ASSERT_EQUAL(ESP_OK, result);

    char ssid[32] = {0};
    char password[64] = {0};
    result = nvs_storage_get_wifi_credentials(ssid, password);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL_STRING("MySSID", ssid);
    TEST_ASSERT_EQUAL_STRING("MyPassword", password);
}

TEST(nvs_storage_tests, test_pump_config) {
    nvs_storage_init();

    esp_err_t result = nvs_storage_set_pump_config(2, 240);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    uint8_t mode = 0;
    uint16_t daily_runtime = 0;
    result = nvs_storage_get_pump_config(&mode, &daily_runtime);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(2, mode);
    TEST_ASSERT_EQUAL(240, daily_runtime);
}

TEST(nvs_storage_tests, test_load_missing_key) {
    nvs_storage_init();

    int32_t value;
    esp_err_t result = nvs_storage_load_int("nonexistent", &value);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

TEST(nvs_storage_tests, test_erase_key) {
    nvs_storage_init();

    nvs_storage_save_int("to_erase", 99);
    esp_err_t result = nvs_storage_erase("to_erase");
    TEST_ASSERT_EQUAL(ESP_OK, result);

    int32_t value;
    result = nvs_storage_load_int("to_erase", &value);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

TEST(nvs_storage_tests, test_overwrite_value) {
    nvs_storage_init();

    nvs_storage_save_int("key", 10);
    nvs_storage_save_int("key", 20);

    int32_t value;
    esp_err_t result = nvs_storage_load_int("key", &value);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(20, value);
}

TEST_GROUP_RUNNER(nvs_storage_tests) {
    RUN_TEST_CASE(nvs_storage_tests, test_init_success);
    RUN_TEST_CASE(nvs_storage_tests, test_save_and_load_string);
    RUN_TEST_CASE(nvs_storage_tests, test_save_and_load_int);
    RUN_TEST_CASE(nvs_storage_tests, test_wifi_credentials);
    RUN_TEST_CASE(nvs_storage_tests, test_pump_config);
    RUN_TEST_CASE(nvs_storage_tests, test_load_missing_key);
    RUN_TEST_CASE(nvs_storage_tests, test_erase_key);
    RUN_TEST_CASE(nvs_storage_tests, test_overwrite_value);
}
