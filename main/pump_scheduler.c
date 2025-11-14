#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <time.h>
#include <sys/time.h>

#include "config.h"
#include "pump_controller.h"
#include "price_fetcher.h"
#include "wifi_manager.h"

static const char *TAG = "PUMP_SCHEDULER";

static bool is_within_operating_hours(void)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
        
    // Allow operation between 6 AM and 10 PM
    return (timeinfo.tm_hour >= 6 && timeinfo.tm_hour < 22);
}

static pump_mode_t determine_optimal_mode(void)
{
    if (!wifi_manager_is_connected()) {
        ESP_LOGW(TAG, "WiFi not connected, using default day mode");
        return PUMP_MODE_DAY;
    }
        
    float current_price = price_fetcher_get_current_price();
        
    if (current_price < PRICE_THRESHOLD_LOW) {
        ESP_LOGI(TAG, "Low price period (%.3f EUR/kWh), using day mode", current_price);
        return PUMP_MODE_DAY;
    } else if (current_price > PRICE_THRESHOLD_HIGH) {
        ESP_LOGI(TAG, "High price period (%.3f EUR/kWh), using night mode", current_price);
        return PUMP_MODE_NIGHT;
    } else {
        ESP_LOGI(TAG, "Medium price period (%.3f EUR/kWh), using day mode", current_price);
        return PUMP_MODE_DAY;
    }
}

void pump_scheduler_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Pump scheduler task started");
        
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(60000); // Run every minute
        
    bool pump_running = false;
    int daily_runtime_minutes = 0;
    int last_hour = -1;
        
    while (1) {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
                
        // Reset daily counter at midnight
        if (timeinfo.tm_hour == 0 && last_hour == 23) {
            daily_runtime_minutes = 0;
            ESP_LOGI(TAG, "New day started, resetting runtime counter");
        }
        last_hour = timeinfo.tm_hour;
                
        // Check if we're within operating hours
        if (!is_within_operating_hours()) {
            if (pump_running) {
                ESP_LOGI(TAG, "Outside operating hours, stopping pump");
                pump_controller_stop();
                pump_running = false;
            }
        } else {
            // Check if we've reached minimum daily runtime
            int min_runtime_minutes = MIN_DAILY_RUNTIME_HOURS * 60;
            int max_runtime_minutes = MAX_DAILY_RUNTIME_HOURS * 60;
                        
            if (daily_runtime_minutes < min_runtime_minutes) {
                // Must run to meet minimum requirements
                if (!pump_running) {
                    pump_mode_t mode = determine_optimal_mode();
                    ESP_LOGI(TAG, "Starting pump to meet minimum runtime (%d/%d min)", 
                             daily_runtime_minutes, min_runtime_minutes);
                    pump_controller_set_mode(mode);
                    pump_controller_start();
                    pump_running = true;
                }
            } else if (daily_runtime_minutes >= max_runtime_minutes) {
                // Reached maximum, stop for today
                if (pump_running) {
                    ESP_LOGI(TAG, "Maximum daily runtime reached, stopping pump");
                    pump_controller_stop();
                    pump_running = false;
                }
            } else {
                // Optional operation based on electricity prices
                if (price_fetcher_is_low_price_period() && !pump_running) {
                    pump_mode_t mode = determine_optimal_mode();
                    ESP_LOGI(TAG, "Low price period detected, starting pump");
                    pump_controller_set_mode(mode);
                    pump_controller_start();
                    pump_running = true;
                } else if (!price_fetcher_is_low_price_period() && pump_running) {
                    ESP_LOGI(TAG, "Price increased, stopping optional operation");
                    pump_controller_stop();
                    pump_running = false;
                }
            }
        }
                
        // Update runtime counter
        if (pump_running) {
            daily_runtime_minutes++;
        }
                
        // Log status every 15 minutes
        static int log_counter = 0;
        if (++log_counter >= 15) {
            log_counter = 0;
            pump_status_t status;
            pump_controller_get_status(&status);
            ESP_LOGI(TAG, "Status: %s, Mode: %d, Runtime today: %d min, Price: %.3f EUR/kWh", 
                     pump_running ? "RUNNING" : "STOPPED", 
                     status.mode, 
                     daily_runtime_minutes,
                     price_fetcher_get_current_price());
        }
                
        vTaskDelayUntil(&last_wake_time, frequency);
    }
}
