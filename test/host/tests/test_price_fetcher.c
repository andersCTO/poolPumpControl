#include "esp_http_client.h"
#include "price_fetcher.h"
#include "unity.h"
#include "unity_fixture.h"
#include <string.h>

TEST_GROUP(price_fetcher_tests);

TEST_SETUP(price_fetcher_tests) { mock_http_client_reset(); }

TEST_TEAR_DOWN(price_fetcher_tests) {}

TEST(price_fetcher_tests, test_init_success) {
    esp_err_t result = price_fetcher_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

TEST(price_fetcher_tests, test_get_today_prices_success) {
    // elprisetjustnu.se API returns 96 entries (15-min intervals) per day
    const char *mock_json = "["
                            "{\"SEK_per_kWh\": 0.75, \"time_start\": \"2024-01-15T00:00:00+01:00\"},"
                            "{\"SEK_per_kWh\": 0.80, \"time_start\": \"2024-01-15T01:00:00+01:00\"},"
                            "{\"SEK_per_kWh\": 0.65, \"time_start\": \"2024-01-15T02:00:00+01:00\"}"
                            "]";

    mock_http_client_set_response_data(mock_json, strlen(mock_json));
    mock_http_client_set_status_code(200);
    mock_http_client_set_content_length(strlen(mock_json));

    price_data_t prices[24];
    esp_err_t result = price_fetcher_get_today_prices(prices);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    TEST_ASSERT_EQUAL_FLOAT(0.75f, prices[0].price_sek_kwh);
    TEST_ASSERT_EQUAL_FLOAT(0.80f, prices[1].price_sek_kwh);
    TEST_ASSERT_EQUAL_FLOAT(0.65f, prices[2].price_sek_kwh);
}

TEST(price_fetcher_tests, test_get_current_price_no_data) {
    price_fetcher_init();
    float price = price_fetcher_get_current_price();
    TEST_ASSERT_EQUAL_FLOAT(0.0f, price);
}

TEST(price_fetcher_tests, test_is_low_price_period_high_price) {
    // Price above PRICE_THRESHOLD_LOW (0.50 SEK) should not be low
    const char *mock_json = "["
                            "{\"SEK_per_kWh\": 2.50, \"time_start\": \"2024-01-15T00:00:00+01:00\"}"
                            "]";

    mock_http_client_set_response_data(mock_json, strlen(mock_json));
    mock_http_client_set_status_code(200);
    mock_http_client_set_content_length(strlen(mock_json));

    price_data_t prices[24];
    price_fetcher_get_today_prices(prices);

    bool is_low = price_fetcher_is_low_price_period();
    TEST_ASSERT_FALSE(is_low);
}

TEST(price_fetcher_tests, test_is_low_price_period_zero_price) {
    // Zero price should not trigger low price (need positive price)
    const char *mock_json = "["
                            "{\"SEK_per_kWh\": 0.0, \"time_start\": \"2024-01-15T00:00:00+01:00\"}"
                            "]";

    mock_http_client_set_response_data(mock_json, strlen(mock_json));
    mock_http_client_set_status_code(200);
    mock_http_client_set_content_length(strlen(mock_json));

    price_data_t prices[24];
    price_fetcher_get_today_prices(prices);

    bool is_low = price_fetcher_is_low_price_period();
    TEST_ASSERT_FALSE(is_low);
}

TEST(price_fetcher_tests, test_multiple_price_records) {
    const char *mock_json = "["
                            "{\"SEK_per_kWh\": 1.00, \"time_start\": \"2024-01-15T00:00:00+01:00\"},"
                            "{\"SEK_per_kWh\": 1.20, \"time_start\": \"2024-01-15T01:00:00+01:00\"},"
                            "{\"SEK_per_kWh\": 0.80, \"time_start\": \"2024-01-15T02:00:00+01:00\"},"
                            "{\"SEK_per_kWh\": 1.50, \"time_start\": \"2024-01-15T03:00:00+01:00\"},"
                            "{\"SEK_per_kWh\": 0.90, \"time_start\": \"2024-01-15T04:00:00+01:00\"}"
                            "]";

    mock_http_client_set_response_data(mock_json, strlen(mock_json));
    mock_http_client_set_status_code(200);
    mock_http_client_set_content_length(strlen(mock_json));

    price_data_t prices[24];
    esp_err_t result = price_fetcher_get_today_prices(prices);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    TEST_ASSERT_EQUAL_FLOAT(1.00f, prices[0].price_sek_kwh);
    TEST_ASSERT_EQUAL_FLOAT(1.20f, prices[1].price_sek_kwh);
    TEST_ASSERT_EQUAL_FLOAT(0.80f, prices[2].price_sek_kwh);
    TEST_ASSERT_EQUAL_FLOAT(1.50f, prices[3].price_sek_kwh);
    TEST_ASSERT_EQUAL_FLOAT(0.90f, prices[4].price_sek_kwh);

    // Remaining prices should be initialized to -1 (invalid)
    for (int i = 5; i < 24; i++) {
        TEST_ASSERT_EQUAL_FLOAT(-1.0f, prices[i].price_sek_kwh);
    }
}

TEST_GROUP_RUNNER(price_fetcher_tests) {
    RUN_TEST_CASE(price_fetcher_tests, test_init_success);
    RUN_TEST_CASE(price_fetcher_tests, test_get_today_prices_success);
    RUN_TEST_CASE(price_fetcher_tests, test_get_current_price_no_data);
    RUN_TEST_CASE(price_fetcher_tests, test_is_low_price_period_high_price);
    RUN_TEST_CASE(price_fetcher_tests, test_is_low_price_period_zero_price);
    RUN_TEST_CASE(price_fetcher_tests, test_multiple_price_records);
}
