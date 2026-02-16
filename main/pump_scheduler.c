#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "config.h"
#include "optimizer.h"
#include "price_fetcher.h"
#include "pump_controller.h"
#include "wifi_manager.h"

static const char *TAG = "PUMP_SCHEDULER";

// Mutex for thread-safe access to scheduler state (read by web_server task)
static SemaphoreHandle_t s_scheduler_mutex = NULL;

static bool s_pump_running = false;
static int s_daily_runtime_minutes = 0;
static int s_current_hour = -1;

// Optimizer state
static optimizer_schedule_t s_today_schedule;
static bool s_schedule_computed = false;
static uint8_t s_last_computed_day = 0;
static time_t s_last_price_fetch_time = 0;

// Get current 15-minute slot index (0-95)
static int get_current_slot(void) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    return timeinfo.tm_hour * 4 + (timeinfo.tm_min / 15);
}

// Check if schedule needs recomputation
static bool schedule_needs_update(void) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    // Recompute on day change
    if (s_last_computed_day != timeinfo.tm_mday) {
        ESP_LOGI(TAG, "Day changed, schedule recomputation needed");
        return true;
    }

    // Recompute if prices were refreshed
    time_t last_fetch = price_fetcher_get_last_fetch_time();
    if (last_fetch > s_last_price_fetch_time) {
        ESP_LOGI(TAG, "Prices updated, schedule recomputation needed");
        return true;
    }

    // Recompute if we haven't computed yet
    if (!s_schedule_computed) {
        return true;
    }

    return false;
}

// Compute and cache the daily schedule
static void update_schedule(void) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    // Get current interval prices
    price_interval_t prices[PRICE_INTERVALS_PER_DAY];
    int valid_count = price_fetcher_get_interval_prices(prices);

    if (valid_count == 0) {
        ESP_LOGW(TAG, "No valid prices available, cannot compute schedule");
        s_schedule_computed = false;
        return;
    }

    // Compute optimal schedule
    esp_err_t ret = optimizer_compute_daily(prices, &s_today_schedule);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to compute schedule: %s", esp_err_to_name(ret));
        s_schedule_computed = false;
        return;
    }

    s_schedule_computed = true;
    s_last_computed_day = timeinfo.tm_mday;
    s_last_price_fetch_time = price_fetcher_get_last_fetch_time();

    ESP_LOGI(TAG,
             "Schedule computed: %ld L volume, %.2f SEK estimated cost",
             (long)s_today_schedule.total_volume_liters,
             s_today_schedule.total_cost_cents / 100.0f);
}

// Apply the scheduled pump mode for current slot
static void apply_scheduled_mode(int current_slot) {
    pump_mode_t target_mode = PUMP_MODE_OFF;

    if (s_schedule_computed && s_today_schedule.valid) {
        target_mode = optimizer_get_slot_mode(&s_today_schedule, current_slot);
    } else {
        // Fallback: use price thresholds if no schedule available
        if (!wifi_manager_is_connected() || !price_fetcher_is_data_valid()) {
            target_mode = PUMP_MODE_OFF;
        } else {
            float current_price = price_fetcher_get_current_price();
            if (current_price < PRICE_THRESHOLD_LOW) {
                target_mode = PUMP_MODE_DAY;
            } else if (current_price > PRICE_THRESHOLD_HIGH) {
                target_mode = PUMP_MODE_OFF;
            } else {
                target_mode = PUMP_MODE_NIGHT;
            }
        }
    }

    // Get current pump state
    pump_status_t status;
    pump_controller_get_status(&status);

    // Apply mode change if needed
    if (target_mode == PUMP_MODE_OFF) {
        if (s_pump_running) {
            ESP_LOGI(TAG, "Slot %d: Stopping pump (scheduled OFF)", current_slot);
            pump_controller_stop();
            s_pump_running = false;
        }
    } else {
        if (!s_pump_running) {
            ESP_LOGI(TAG,
                     "Slot %d: Starting pump in %s mode",
                     current_slot,
                     target_mode == PUMP_MODE_NIGHT      ? "LOW"
                     : target_mode == PUMP_MODE_DAY      ? "MEDIUM"
                     : target_mode == PUMP_MODE_BACKWASH ? "HIGH"
                                                         : "UNKNOWN");
            pump_controller_set_mode(target_mode);
            pump_controller_start();
            s_pump_running = true;
        } else if (status.mode != target_mode) {
            ESP_LOGI(TAG,
                     "Slot %d: Changing mode to %s",
                     current_slot,
                     target_mode == PUMP_MODE_NIGHT      ? "LOW"
                     : target_mode == PUMP_MODE_DAY      ? "MEDIUM"
                     : target_mode == PUMP_MODE_BACKWASH ? "HIGH"
                                                         : "UNKNOWN");
            pump_controller_set_mode(target_mode);
        }
    }
}

void pump_scheduler_task(void *pvParameters) {
    ESP_LOGI(TAG, "Pump scheduler task started");

    // Create mutex for thread-safe state access
    s_scheduler_mutex = xSemaphoreCreateMutex();
    if (s_scheduler_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create scheduler mutex");
        vTaskDelete(NULL);
        return;
    }

    // Initialize optimizer with default config
    optimizer_config_t opt_config = {
        .pool_volume_liters = POOL_VOLUME_LITERS,
        .circulation_factor = POOL_CIRCULATION_FACTOR,
        .op_start_hour = PUMP_OP_START_HOUR,
        .op_end_hour = PUMP_OP_END_HOUR,
    };
    optimizer_init(&opt_config);

    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(60000); // Run every minute

    int last_hour = -1;

    while (1) {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);

        // Hold mutex while modifying shared state (read by web_server task)
        xSemaphoreTake(s_scheduler_mutex, portMAX_DELAY);

        // Reset daily counter at midnight
        if (timeinfo.tm_hour == 0 && last_hour == 23) {
            s_daily_runtime_minutes = 0;
            s_schedule_computed = false; // Force recomputation for new day
            ESP_LOGI(TAG, "New day started, resetting runtime counter");
        }
        last_hour = timeinfo.tm_hour;
        s_current_hour = timeinfo.tm_hour;

        // Check if schedule needs update
        if (schedule_needs_update()) {
            update_schedule();
        }

        // Get current slot and apply scheduled mode
        int current_slot = get_current_slot();
        apply_scheduled_mode(current_slot);

        // Update runtime counter
        if (s_pump_running) {
            s_daily_runtime_minutes++;
        }

        xSemaphoreGive(s_scheduler_mutex);

        // Log status every 15 minutes (outside mutex to avoid nested locks)
        static int log_counter = 0;
        if (++log_counter >= 15) {
            log_counter = 0;
            pump_status_t status;
            pump_controller_get_status(&status);
            ESP_LOGI(TAG,
                     "Status: %s, Mode: %s, Slot: %d, Runtime: %d min, Cost: %.2f SEK",
                     s_pump_running ? "RUNNING" : "STOPPED",
                     status.mode == PUMP_MODE_NIGHT      ? "LOW"
                     : status.mode == PUMP_MODE_DAY      ? "MEDIUM"
                     : status.mode == PUMP_MODE_BACKWASH ? "HIGH"
                                                         : "OFF",
                     current_slot,
                     s_daily_runtime_minutes,
                     s_schedule_computed ? s_today_schedule.total_cost_cents / 100.0f : 0.0f);
        }

        vTaskDelayUntil(&last_wake_time, frequency);
    }
}

void pump_scheduler_get_status(scheduler_status_t *status) {
    if (status == NULL) {
        return;
    }
    if (s_scheduler_mutex != NULL && xSemaphoreTake(s_scheduler_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        status->pump_running = s_pump_running;
        status->daily_runtime_minutes = s_daily_runtime_minutes;
        status->current_hour = s_current_hour;
        xSemaphoreGive(s_scheduler_mutex);
    } else {
        ESP_LOGW(TAG, "Scheduler mutex timeout in get_status");
        memset(status, 0, sizeof(scheduler_status_t));
    }
}

bool pump_scheduler_get_schedule(optimizer_schedule_t *schedule) {
    if (schedule == NULL) {
        return false;
    }
    if (s_scheduler_mutex != NULL && xSemaphoreTake(s_scheduler_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        bool result = false;
        if (s_schedule_computed) {
            *schedule = s_today_schedule;
            result = true;
        }
        xSemaphoreGive(s_scheduler_mutex);
        return result;
    }
    ESP_LOGW(TAG, "Scheduler mutex timeout in get_schedule");
    return false;
}
