#ifndef PRICE_FETCHER_H
#define PRICE_FETCHER_H

#include "esp_err.h"
#include <stdbool.h>
#include <time.h>

typedef struct {
    int hour;            // Hour of day (0-23)
    float price_eur_kwh; // Price in EUR/kWh
} price_data_t;

typedef enum {
    PRICE_FETCH_STATUS_IDLE,
    PRICE_FETCH_STATUS_FETCHING,
    PRICE_FETCH_STATUS_SUCCESS,
    PRICE_FETCH_STATUS_FAILED,
    PRICE_FETCH_STATUS_NO_WIFI
} price_fetch_status_t;

typedef struct {
    price_fetch_status_t status;
    time_t last_fetch_time;
    time_t last_attempt_time;
    uint8_t consecutive_failures;
    uint8_t cached_day;      // Day of month for staleness detection
    bool tomorrow_available; // Whether tomorrow's prices are cached
} price_refresh_state_t;

/**
 * @brief Initialize price fetcher component
 * @return ESP_OK on success
 */
esp_err_t price_fetcher_init(void);

/**
 * @brief Fetch current day electricity prices
 * @param prices Array to store 24-hour price data
 * @return ESP_OK on success
 */
esp_err_t price_fetcher_get_today_prices(price_data_t prices[24]);

/**
 * @brief Get current hour price
 * @return Current electricity price in EUR/kWh
 */
float price_fetcher_get_current_price(void);

/**
 * @brief Check if current price is below threshold for pump operation
 * @return true if price is low enough for operation
 */
bool price_fetcher_is_low_price_period(void);

/**
 * @brief Check if cached price data is valid and not stale
 * @return true if data is valid for scheduling decisions
 */
bool price_fetcher_is_data_valid(void);

/**
 * @brief Get timestamp of last successful price fetch
 * @return time_t of last fetch, or 0 if never fetched
 */
time_t price_fetcher_get_last_fetch_time(void);

/**
 * @brief Get current refresh state for status reporting
 * @param state Output structure to fill with current state
 */
void price_fetcher_get_refresh_state(price_refresh_state_t *state);

/**
 * @brief Trigger an immediate price refresh
 * @return ESP_OK if fetch started, error otherwise
 */
esp_err_t price_fetcher_trigger_refresh(void);

/**
 * @brief Price refresh task - fetches prices periodically
 * @param pvParameters FreeRTOS task parameter (unused)
 */
void price_refresh_task(void *pvParameters);

#endif // PRICE_FETCHER_H
