#include "config.h"
#include "driver/gpio.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "price_fetcher.h"
#include "pump_controller.h"
#include "unity.h"
#include "unity_fixture.h"
#include "web_server.h"
#include "wifi_manager.h"
#include <string.h>

TEST_GROUP(web_server_tests);

TEST_SETUP(web_server_tests) {
    mock_httpd_reset();
    mock_gpio_reset();
    mock_http_client_reset();
}

TEST_TEAR_DOWN(web_server_tests) { web_server_stop(); }

TEST(web_server_tests, test_init_registers_handlers) {
    esp_err_t result = web_server_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);

    TEST_ASSERT_EQUAL(2, mock_httpd_get_registered_uri_count());

    const httpd_uri_t *dashboard = mock_httpd_get_registered_uri(0);
    TEST_ASSERT_NOT_NULL(dashboard);
    TEST_ASSERT_EQUAL_STRING("/", dashboard->uri);
    TEST_ASSERT_EQUAL(HTTP_GET, dashboard->method);

    const httpd_uri_t *api = mock_httpd_get_registered_uri(1);
    TEST_ASSERT_NOT_NULL(api);
    TEST_ASSERT_EQUAL_STRING("/api/status", api->uri);
    TEST_ASSERT_EQUAL(HTTP_GET, api->method);
}

TEST(web_server_tests, test_dashboard_handler_returns_html) {
    web_server_init();

    const httpd_uri_t *dashboard = mock_httpd_get_registered_uri(0);
    TEST_ASSERT_NOT_NULL(dashboard);

    httpd_req_t req = {0};
    esp_err_t result = dashboard->handler(&req);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    const char *response = mock_httpd_get_last_response();
    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(strlen(response) > 0);

    // Verify it contains HTML markers
    TEST_ASSERT_NOT_NULL(strstr(response, "<!DOCTYPE html>"));
    TEST_ASSERT_NOT_NULL(strstr(response, "Pool Pump Controller"));

    // Verify content type
    TEST_ASSERT_EQUAL_STRING("text/html", mock_httpd_get_last_content_type());
}

TEST(web_server_tests, test_api_status_handler_returns_json) {
    web_server_init();

    const httpd_uri_t *api = mock_httpd_get_registered_uri(1);
    TEST_ASSERT_NOT_NULL(api);

    httpd_req_t req = {0};
    esp_err_t result = api->handler(&req);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    const char *response = mock_httpd_get_last_response();
    TEST_ASSERT_NOT_NULL(response);

    // Verify JSON structure
    TEST_ASSERT_NOT_NULL(strstr(response, "\"pump_running\""));
    TEST_ASSERT_NOT_NULL(strstr(response, "\"mode\""));
    TEST_ASSERT_NOT_NULL(strstr(response, "\"rpm\""));
    TEST_ASSERT_NOT_NULL(strstr(response, "\"daily_runtime_minutes\""));
    TEST_ASSERT_NOT_NULL(strstr(response, "\"price_eur_kwh\""));
    TEST_ASSERT_NOT_NULL(strstr(response, "\"wifi_connected\""));

    // Verify content type
    TEST_ASSERT_EQUAL_STRING("application/json", mock_httpd_get_last_content_type());
}

TEST(web_server_tests, test_stop_server) {
    web_server_init();
    esp_err_t result = web_server_stop();
    TEST_ASSERT_EQUAL(ESP_OK, result);

    // Stopping again should be safe
    result = web_server_stop();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

TEST(web_server_tests, test_double_init) {
    esp_err_t result = web_server_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);

    // Second init should succeed gracefully
    result = web_server_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

TEST_GROUP_RUNNER(web_server_tests) {
    RUN_TEST_CASE(web_server_tests, test_init_registers_handlers);
    RUN_TEST_CASE(web_server_tests, test_dashboard_handler_returns_html);
    RUN_TEST_CASE(web_server_tests, test_api_status_handler_returns_json);
    RUN_TEST_CASE(web_server_tests, test_stop_server);
    RUN_TEST_CASE(web_server_tests, test_double_init);
}
