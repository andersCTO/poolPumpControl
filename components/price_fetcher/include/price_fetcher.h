#ifndef PRICE_FETCHER_H
#define PRICE_FETCHER_H

#include "esp_err.h"
#include <stdbool.h>
#include <time.h>

#define PRICE_INTERVALS_PER_DAY 96 // 15-minute intervals

typedef struct {
    int hour;            // Hour of day (0-23)
    float price_sek_kwh; // Price in SEK/kWh
} price_data_t;

typedef struct {
    uint8_t interval;    // Interval index (0-95)
    float price_sek_kwh; // Price in SEK/kWh
} price_interval_t;

typedef enum {
    SCHEDULE_OFF = 0,  // Pump off (outside operating hours)
    SCHEDULE_REQUIRED, // Must run to meet minimum runtime
    SCHEDULE_OPTIONAL, // Running due to low price
    SCHEDULE_STOPPED   // Not running (price too high or max reached)
} schedule_slot_status_t;

typedef struct {
    schedule_slot_status_t status;
    uint8_t mode; // PUMP_MODE_* if running
    float price_sek_kwh;
} schedule_slot_t;

typedef struct {
    float min_price;
    float max_price;
    float avg_price;
    int min_hour;
    int max_hour;
    int valid_hours; // Number of hours with valid price data
} price_stats_t;

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
 * @return Current electricity price in SEK/kWh
 */
float price_fetcher_get_current_price(void);

/**
 * @brief Get all cached prices for today
 * @param prices Array to fill with 24-hour price data
 * @return Number of valid hours (0-24)
 */
int price_fetcher_get_cached_prices(price_data_t prices[24]);

/**
 * @brief Get all 15-minute interval prices for today
 * @param intervals Array to fill with 96 interval prices
 * @return Number of valid intervals (0-96)
 */
int price_fetcher_get_interval_prices(price_interval_t intervals[PRICE_INTERVALS_PER_DAY]);

/**
 * @brief Compute expected pump schedule for the day based on prices
 * @param schedule Array to fill with 96 schedule slots
 * @param current_runtime Current runtime in minutes today
 */
void price_fetcher_compute_schedule(schedule_slot_t schedule[PRICE_INTERVALS_PER_DAY], int current_runtime);

/**
 * @brief Get price statistics for today
 * @param stats Output structure for statistics
 */
void price_fetcher_get_stats(price_stats_t *stats);

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
