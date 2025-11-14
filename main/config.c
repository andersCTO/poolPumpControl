#include "config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

static const char *TAG = "CONFIG";

void config_init(void) {
    ESP_LOGI(TAG, "Initializing configuration...");
    ESP_LOGI(TAG,
             "Pump speeds: Night=%d RPM, Day=%d RPM, Backwash=%d RPM",
             PUMP_SPEED_NIGHT,
             PUMP_SPEED_DAY,
             PUMP_SPEED_BACKWASH);
    ESP_LOGI(TAG, "Price thresholds: Low=%.2f EUR/kWh, High=%.2f EUR/kWh", PRICE_THRESHOLD_LOW, PRICE_THRESHOLD_HIGH);
    ESP_LOGI(TAG, "Daily runtime: Min=%d hours, Max=%d hours", MIN_DAILY_RUNTIME_HOURS, MAX_DAILY_RUNTIME_HOURS);
}
