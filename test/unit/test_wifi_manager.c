/**
 * @file test_wifi_manager.c
 * @brief Unit tests for WiFi manager component
 */

#include "unity.h"
#include "wifi_manager.h"
#include "mock_esp_wifi.h"
#include <string.h>

// Test group
TEST_GROUP(wifi_manager_tests);

// Test setup and teardown
TEST_SETUP(wifi_manager_tests)
{
    mock_esp_wifi_reset();
}

TEST_TEAR_DOWN(wifi_manager_tests)
{
    // Clean up after each test
}

/**
 * @brief Test WiFi manager initialization
 */
TEST(wifi_manager_tests, test_init_success)
{
    esp_err_t result = wifi_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

/**
 * @brief Test WiFi connection with valid credentials
 */
TEST(wifi_manager_tests, test_connect_success)
{
    const char *ssid = "TestNetwork";
    const char *password = "testpassword";

    esp_err_t result = wifi_manager_connect(ssid, password);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    // Verify connection was attempted
    TEST_ASSERT_TRUE(mock_esp_wifi_is_connected());
}

/**
 * @brief Test WiFi connection with NULL SSID
 */
TEST(wifi_manager_tests, test_connect_null_ssid)
{
    esp_err_t result = wifi_manager_connect(NULL, "password");
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

/**
 * @brief Test WiFi connection with NULL password
 */
TEST(wifi_manager_tests, test_connect_null_password)
{
    esp_err_t result = wifi_manager_connect("TestNetwork", NULL);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

/**
 * @brief Test WiFi connection status when connected
 */
TEST(wifi_manager_tests, test_is_connected_true)
{
    mock_esp_wifi_set_connected(true);
    bool result = wifi_manager_is_connected();
    TEST_ASSERT_TRUE(result);
}

/**
 * @brief Test WiFi connection status when disconnected
 */
TEST(wifi_manager_tests, test_is_connected_false)
{
    mock_esp_wifi_set_connected(false);
    bool result = wifi_manager_is_connected();
    TEST_ASSERT_FALSE(result);
}

/**
 * @brief Test WiFi disconnection
 */
TEST(wifi_manager_tests, test_disconnect)
{
    // First connect
    mock_esp_wifi_set_connected(true);
    TEST_ASSERT_TRUE(wifi_manager_is_connected());

    // Then disconnect
    esp_err_t result = wifi_manager_disconnect();
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_FALSE(mock_esp_wifi_is_connected());
}

/**
 * @brief Test WiFi connection with empty strings
 */
TEST(wifi_manager_tests, test_connect_empty_credentials)
{
    esp_err_t result = wifi_manager_connect("", "");
    TEST_ASSERT_EQUAL(ESP_OK, result); // Empty strings are technically valid
}

/**
 * @brief Test WiFi connection with very long SSID
 */
TEST(wifi_manager_tests, test_connect_long_ssid)
{
    char long_ssid[64] = {0};
    memset(long_ssid, 'A', 63); // Fill with 'A's

    esp_err_t result = wifi_manager_connect(long_ssid, "password");
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

// Test group runner
TEST_GROUP_RUNNER(wifi_manager_tests)
{
    RUN_TEST_CASE(wifi_manager_tests, test_init_success);
    RUN_TEST_CASE(wifi_manager_tests, test_connect_success);
    RUN_TEST_CASE(wifi_manager_tests, test_connect_null_ssid);
    RUN_TEST_CASE(wifi_manager_tests, test_connect_null_password);
    RUN_TEST_CASE(wifi_manager_tests, test_is_connected_true);
    RUN_TEST_CASE(wifi_manager_tests, test_is_connected_false);
    RUN_TEST_CASE(wifi_manager_tests, test_disconnect);
    RUN_TEST_CASE(wifi_manager_tests, test_connect_empty_credentials);
    RUN_TEST_CASE(wifi_manager_tests, test_connect_long_ssid);
}