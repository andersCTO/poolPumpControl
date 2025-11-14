#ifndef PRICE_FETCHER_H
#define PRICE_FETCHER_H

#include "esp_err.h"
#include <stdbool.h>

typedef struct {
    int hour;            // Hour of day (0-23)
    float price_eur_kwh; // Price in EUR/kWh
} price_data_t;

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

#endif // PRICE_FETCHER_H
