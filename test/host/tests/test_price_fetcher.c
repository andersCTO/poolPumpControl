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
    const char *mock_json = "{"
                            "\"records\": ["
                            "{\"SpotPriceEUR\": 0.123},"
                            "{\"SpotPriceEUR\": 0.145},"
                            "{\"SpotPriceEUR\": 0.089}"
                            "]"
                            "}";

    mock_http_client_set_response_data(mock_json, strlen(mock_json));
    mock_http_client_set_status_code(200);
    mock_http_client_set_content_length(strlen(mock_json));

    price_data_t prices[24];
    esp_err_t result = price_fetcher_get_today_prices(prices);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    TEST_ASSERT_EQUAL_FLOAT(0.123f, prices[0].price_eur_kwh);
    TEST_ASSERT_EQUAL_FLOAT(0.145f, prices[1].price_eur_kwh);
    TEST_ASSERT_EQUAL_FLOAT(0.089f, prices[2].price_eur_kwh);
}

TEST(price_fetcher_tests, test_get_current_price_no_data) {
    price_fetcher_init();
    float price = price_fetcher_get_current_price();
    TEST_ASSERT_EQUAL_FLOAT(0.0f, price);
}

TEST(price_fetcher_tests, test_is_low_price_period_high_price) {
    const char *mock_json = "{"
                            "\"records\": ["
                            "{\"SpotPriceEUR\": 0.25}"
                            "]"
                            "}";

    mock_http_client_set_response_data(mock_json, strlen(mock_json));
    mock_http_client_set_status_code(200);
    mock_http_client_set_content_length(strlen(mock_json));

    price_data_t prices[24];
    price_fetcher_get_today_prices(prices);

    bool is_low = price_fetcher_is_low_price_period();
    TEST_ASSERT_FALSE(is_low);
}

TEST(price_fetcher_tests, test_is_low_price_period_zero_price) {
    const char *mock_json = "{"
                            "\"records\": ["
                            "{\"SpotPriceEUR\": 0.0}"
                            "]"
                            "}";

    mock_http_client_set_response_data(mock_json, strlen(mock_json));
    mock_http_client_set_status_code(200);
    mock_http_client_set_content_length(strlen(mock_json));

    price_data_t prices[24];
    price_fetcher_get_today_prices(prices);

    bool is_low = price_fetcher_is_low_price_period();
    TEST_ASSERT_FALSE(is_low);
}

TEST(price_fetcher_tests, test_multiple_price_records) {
    const char *mock_json = "{"
                            "\"records\": ["
                            "{\"SpotPriceEUR\": 0.10},"
                            "{\"SpotPriceEUR\": 0.12},"
                            "{\"SpotPriceEUR\": 0.08},"
                            "{\"SpotPriceEUR\": 0.15},"
                            "{\"SpotPriceEUR\": 0.09}"
                            "]"
                            "}";

    mock_http_client_set_response_data(mock_json, strlen(mock_json));
    mock_http_client_set_status_code(200);
    mock_http_client_set_content_length(strlen(mock_json));

    price_data_t prices[24];
    esp_err_t result = price_fetcher_get_today_prices(prices);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    TEST_ASSERT_EQUAL_FLOAT(0.10f, prices[0].price_eur_kwh);
    TEST_ASSERT_EQUAL_FLOAT(0.12f, prices[1].price_eur_kwh);
    TEST_ASSERT_EQUAL_FLOAT(0.08f, prices[2].price_eur_kwh);
    TEST_ASSERT_EQUAL_FLOAT(0.15f, prices[3].price_eur_kwh);
    TEST_ASSERT_EQUAL_FLOAT(0.09f, prices[4].price_eur_kwh);

    for (int i = 5; i < 24; i++) {
        TEST_ASSERT_EQUAL_FLOAT(0.0f, prices[i].price_eur_kwh);
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
