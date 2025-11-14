/**
 * @file test_price_fetcher.c
 * @brief Unit tests for price fetcher component
 */

#include "mock_esp_http_client.h"
#include "price_fetcher.h"
#include "unity.h"
#include <string.h>

// Test group
TEST_GROUP(price_fetcher_tests);

// Test setup and teardown
TEST_SETUP(price_fetcher_tests) { mock_http_client_reset(); }

TEST_TEAR_DOWN(price_fetcher_tests) {
    // Clean up after each test
}

/**
 * @brief Test price fetcher initialization
 */
TEST(price_fetcher_tests, test_init_success) {
    esp_err_t result = price_fetcher_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

/**
 * @brief Test fetching today's prices with valid JSON response
 */
TEST(price_fetcher_tests, test_get_today_prices_success) {
    // Mock JSON response with price data
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

    // Check first few prices
    TEST_ASSERT_EQUAL_FLOAT(0.123f, prices[0].price_eur_kwh);
    TEST_ASSERT_EQUAL_FLOAT(0.145f, prices[1].price_eur_kwh);
    TEST_ASSERT_EQUAL_FLOAT(0.089f, prices[2].price_eur_kwh);
}

/**
 * @brief Test fetching prices with HTTP error
 */
TEST(price_fetcher_tests, test_get_today_prices_http_error) {
    mock_http_client_set_status_code(404);

    price_data_t prices[24];
    esp_err_t result = price_fetcher_get_today_prices(prices);
    TEST_ASSERT_NOT_EQUAL(ESP_OK, result);
}

/**
 * @brief Test fetching prices with invalid JSON
 */
TEST(price_fetcher_tests, test_get_today_prices_invalid_json) {
    const char *invalid_json = "{ invalid json }";
    mock_http_client_set_response_data(invalid_json, strlen(invalid_json));
    mock_http_client_set_status_code(200);

    price_data_t prices[24];
    esp_err_t result = price_fetcher_get_today_prices(prices);
    TEST_ASSERT_EQUAL(ESP_OK, result); // HTTP request succeeds, but parsing might fail
}

/**
 * @brief Test getting current price when no data is available
 */
TEST(price_fetcher_tests, test_get_current_price_no_data) {
    float price = price_fetcher_get_current_price();
    TEST_ASSERT_EQUAL_FLOAT(0.0f, price);
}

/**
 * @brief Test low price period detection with high price
 */
TEST(price_fetcher_tests, test_is_low_price_period_high_price) {
    // Mock high price data
    const char *mock_json = "{"
                            "\"records\": ["
                            "{\"SpotPriceEUR\": 0.25}" // Above threshold
                            "]"
                            "}";

    mock_http_client_set_response_data(mock_json, strlen(mock_json));
    mock_http_client_set_status_code(200);

    price_data_t prices[24];
    price_fetcher_get_today_prices(prices);

    bool is_low = price_fetcher_is_low_price_period();
    TEST_ASSERT_FALSE(is_low);
}

/**
 * @brief Test low price period detection with low price
 */
TEST(price_fetcher_tests, test_is_low_price_period_low_price) {
    // Mock low price data
    const char *mock_json = "{"
                            "\"records\": ["
                            "{\"SpotPriceEUR\": 0.08}" // Below threshold
                            "]"
                            "}";

    mock_http_client_set_response_data(mock_json, strlen(mock_json));
    mock_http_client_set_status_code(200);

    price_data_t prices[24];
    price_fetcher_get_today_prices(prices);

    bool is_low = price_fetcher_is_low_price_period();
    TEST_ASSERT_TRUE(is_low);
}

/**
 * @brief Test low price period detection with zero price
 */
TEST(price_fetcher_tests, test_is_low_price_period_zero_price) {
    // Mock zero price data
    const char *mock_json = "{"
                            "\"records\": ["
                            "{\"SpotPriceEUR\": 0.0}"
                            "]"
                            "}";

    mock_http_client_set_response_data(mock_json, strlen(mock_json));
    mock_http_client_set_status_code(200);

    price_data_t prices[24];
    price_fetcher_get_today_prices(prices);

    bool is_low = price_fetcher_is_low_price_period();
    TEST_ASSERT_FALSE(is_low); // Zero price is not considered low
}

/**
 * @brief Test price data structure initialization
 */
TEST(price_fetcher_tests, test_price_data_initialization) {
    price_data_t prices[24];

    // Initially all prices should be 0
    for (int i = 0; i < 24; i++) {
        TEST_ASSERT_EQUAL_FLOAT(0.0f, prices[i].price_eur_kwh);
        TEST_ASSERT_EQUAL(i, prices[i].hour);
    }
}

/**
 * @brief Test multiple price records parsing
 */
TEST(price_fetcher_tests, test_multiple_price_records) {
    // Mock JSON with 5 price records
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

    price_data_t prices[24];
    esp_err_t result = price_fetcher_get_today_prices(prices);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    // Check all parsed prices
    TEST_ASSERT_EQUAL_FLOAT(0.10f, prices[0].price_eur_kwh);
    TEST_ASSERT_EQUAL_FLOAT(0.12f, prices[1].price_eur_kwh);
    TEST_ASSERT_EQUAL_FLOAT(0.08f, prices[2].price_eur_kwh);
    TEST_ASSERT_EQUAL_FLOAT(0.15f, prices[3].price_eur_kwh);
    TEST_ASSERT_EQUAL_FLOAT(0.09f, prices[4].price_eur_kwh);

    // Remaining hours should be 0
    for (int i = 5; i < 24; i++) {
        TEST_ASSERT_EQUAL_FLOAT(0.0f, prices[i].price_eur_kwh);
    }
}

// Test group runner
TEST_GROUP_RUNNER(price_fetcher_tests) {
    RUN_TEST_CASE(price_fetcher_tests, test_init_success);
    RUN_TEST_CASE(price_fetcher_tests, test_get_today_prices_success);
    RUN_TEST_CASE(price_fetcher_tests, test_get_today_prices_http_error);
    RUN_TEST_CASE(price_fetcher_tests, test_get_today_prices_invalid_json);
    RUN_TEST_CASE(price_fetcher_tests, test_get_current_price_no_data);
    RUN_TEST_CASE(price_fetcher_tests, test_is_low_price_period_high_price);
    RUN_TEST_CASE(price_fetcher_tests, test_is_low_price_period_low_price);
    RUN_TEST_CASE(price_fetcher_tests, test_is_low_price_period_zero_price);
    RUN_TEST_CASE(price_fetcher_tests, test_price_data_initialization);
    RUN_TEST_CASE(price_fetcher_tests, test_multiple_price_records);
}