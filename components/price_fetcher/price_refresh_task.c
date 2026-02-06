#include "config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "price_fetcher.h"
#include "wifi_manager.h"
#include <time.h>

static const char *TAG = "PRICE_REFRESH";

// Forward declarations for internal functions
extern void price_fetcher_update_state(bool success);
extern void price_fetcher_handle_midnight_rollover(void);

// Calculate backoff delay based on consecutive failures
static uint32_t calculate_backoff_delay_ms(uint8_t failures) {
    if (failures == 0) {
        return 0;
    }

    // Exponential backoff: base * 2^(failures-1), capped at max
    uint32_t delay_sec = PRICE_FETCH_RETRY_BASE_SEC;
    for (uint8_t i = 1; i < failures && delay_sec < (PRICE_FETCH_RETRY_MAX_MIN * 60); i++) {
        delay_sec *= 2;
    }

    // Cap at maximum
    if (delay_sec > (PRICE_FETCH_RETRY_MAX_MIN * 60)) {
        delay_sec = PRICE_FETCH_RETRY_MAX_MIN * 60;
    }

    return delay_sec * 1000;
}

// Check if we need to fetch today's prices
static bool should_fetch_today(void) {
    price_refresh_state_t state;
    price_fetcher_get_refresh_state(&state);

    // Always fetch if we've never fetched
    if (state.last_fetch_time == 0) {
        ESP_LOGI(TAG, "No cached prices, fetch needed");
        return true;
    }

    // Fetch if data is stale
    time_t now;
    time(&now);
    time_t age_hours = (now - state.last_fetch_time) / 3600;
    if (age_hours >= PRICE_STALE_THRESHOLD_HOURS) {
        ESP_LOGI(TAG, "Price data stale (%lld hours old), fetch needed", (long long)age_hours);
        return true;
    }

    // Fetch if day has changed
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    if (state.cached_day != (uint8_t)timeinfo.tm_mday) {
        ESP_LOGI(TAG, "Day changed (cached=%d, current=%d), fetch needed", state.cached_day, timeinfo.tm_mday);
        return true;
    }

    return false;
}

// Check if we should fetch tomorrow's prices (available after 13:00 CET)
static bool should_fetch_tomorrow(void) {
    price_refresh_state_t state;
    price_fetcher_get_refresh_state(&state);

    // Skip if we already have tomorrow's prices
    if (state.tomorrow_available) {
        return false;
    }

    // Check current hour (assuming local time is CET)
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_hour >= PRICE_TOMORROW_AVAILABLE_HOUR) {
        ESP_LOGI(TAG, "After %02d:00, tomorrow's prices should be available", PRICE_TOMORROW_AVAILABLE_HOUR);
        return true;
    }

    return false;
}

// Perform price fetch
static bool do_price_fetch(void) {
    ESP_LOGI(TAG, "Fetching electricity prices...");

    price_data_t prices[24];
    esp_err_t err = price_fetcher_get_today_prices(prices);

    bool success = (err == ESP_OK);
    price_fetcher_update_state(success);

    return success;
}

void price_refresh_task(void *pvParameters) {
    ESP_LOGI(TAG, "Price refresh task started");

    // Initial delay to let other services initialize
    vTaskDelay(pdMS_TO_TICKS(5000));

    TickType_t last_check_time = xTaskGetTickCount();
    const TickType_t check_interval = pdMS_TO_TICKS(60000); // Check every minute
    bool first_connect = true;

    while (1) {
        // Wait for WiFi connection
        if (g_wifi_event_group != NULL) {
            EventBits_t bits = xEventGroupWaitBits(g_wifi_event_group,
                                                   WIFI_CONNECTED_BIT,
                                                   pdFALSE, // Don't clear on exit
                                                   pdFALSE, // Don't wait for all bits
                                                   pdMS_TO_TICKS(10000));

            if (!(bits & WIFI_CONNECTED_BIT)) {
                // Not connected, wait and retry
                ESP_LOGD(TAG, "Waiting for WiFi connection...");
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
        } else if (!wifi_manager_is_connected()) {
            ESP_LOGD(TAG, "Waiting for WiFi connection...");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        // Handle midnight rollover
        price_fetcher_handle_midnight_rollover();

        // Get current state
        price_refresh_state_t state;
        price_fetcher_get_refresh_state(&state);

        // Calculate next fetch timing
        time_t now;
        time(&now);
        bool should_fetch = false;

        // Fetch immediately on first WiFi connect
        if (first_connect) {
            ESP_LOGI(TAG, "First WiFi connection, fetching prices immediately");
            should_fetch = true;
            first_connect = false;
        }
        // Check if we're in backoff period after failures
        else if (state.consecutive_failures > 0) {
            uint32_t backoff_ms = calculate_backoff_delay_ms(state.consecutive_failures);
            time_t backoff_sec = backoff_ms / 1000;
            time_t elapsed = now - state.last_attempt_time;

            if (elapsed >= backoff_sec) {
                ESP_LOGI(TAG, "Backoff period elapsed, retrying fetch");
                should_fetch = true;
            } else {
                ESP_LOGD(TAG, "In backoff, %lld sec remaining", (long long)(backoff_sec - elapsed));
            }
        }
        // Regular hourly check
        else {
            // Check if an hour has passed since last successful fetch
            if (state.last_fetch_time > 0) {
                time_t elapsed_hours = (now - state.last_fetch_time) / 3600;
                if (elapsed_hours >= PRICE_FETCH_INTERVAL_HOURS) {
                    should_fetch = true;
                }
            }

            // Also check if we need today's or tomorrow's prices
            if (!should_fetch && should_fetch_today()) {
                should_fetch = true;
            }
            if (!should_fetch && should_fetch_tomorrow()) {
                should_fetch = true;
            }
        }

        // Perform fetch if needed
        if (should_fetch) {
            bool success = do_price_fetch();
            if (success) {
                ESP_LOGI(TAG, "Price fetch successful");
            } else {
                price_refresh_state_t new_state;
                price_fetcher_get_refresh_state(&new_state);
                uint32_t backoff_ms = calculate_backoff_delay_ms(new_state.consecutive_failures);
                ESP_LOGW(TAG, "Price fetch failed, next retry in %lu seconds", (unsigned long)(backoff_ms / 1000));
            }
        }

        // Sleep until next check
        vTaskDelayUntil(&last_check_time, check_interval);
    }
}
