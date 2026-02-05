#include "esp_wifi.h"
#include "unity.h"
#include "unity_fixture.h"
#include "wifi_manager.h"
#include <string.h>

TEST_GROUP(wifi_manager_tests);

TEST_SETUP(wifi_manager_tests) { mock_esp_wifi_reset(); }

TEST_TEAR_DOWN(wifi_manager_tests) {}

TEST(wifi_manager_tests, test_init_success) {
    esp_err_t result = wifi_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

TEST(wifi_manager_tests, test_connect_success) {
    const char *ssid = "TestNetwork";
    const char *password = "testpassword";

    esp_err_t result = wifi_manager_connect(ssid, password);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    TEST_ASSERT_TRUE(mock_esp_wifi_is_connected());
}

TEST(wifi_manager_tests, test_is_connected_default_false) {
    // wifi_connected is internal and only set via event handler;
    // default state should be false
    bool result = wifi_manager_is_connected();
    TEST_ASSERT_FALSE(result);
}

TEST(wifi_manager_tests, test_disconnect) {
    esp_err_t result = wifi_manager_disconnect();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_FALSE(wifi_manager_is_connected());
}

TEST(wifi_manager_tests, test_connect_empty_credentials) {
    esp_err_t result = wifi_manager_connect("", "");
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

TEST(wifi_manager_tests, test_connect_long_ssid) {
    char long_ssid[64] = {0};
    memset(long_ssid, 'A', 63);

    esp_err_t result = wifi_manager_connect(long_ssid, "password");
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

TEST_GROUP_RUNNER(wifi_manager_tests) {
    RUN_TEST_CASE(wifi_manager_tests, test_init_success);
    RUN_TEST_CASE(wifi_manager_tests, test_connect_success);
    RUN_TEST_CASE(wifi_manager_tests, test_is_connected_default_false);
    RUN_TEST_CASE(wifi_manager_tests, test_disconnect);
    RUN_TEST_CASE(wifi_manager_tests, test_connect_empty_credentials);
    RUN_TEST_CASE(wifi_manager_tests, test_connect_long_ssid);
}
