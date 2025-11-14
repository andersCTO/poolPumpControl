#include "pool_pump/app.h"

#include "pool_pump/networking.h"
#include "pool_pump/price_client.h"
#include "pool_pump/pump_driver.h"
#include "pool_pump/scheduler.h"
#include "pool_pump/sensors.h"
#include "pool_pump/storage.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "pool_pump_app";

esp_err_t pool_pump_app_init(void) {
    esp_err_t err = storage_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Storage init failed: %s", esp_err_to_name(err));
        return err;
    }

    err = networking_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Networking init failed: %s", esp_err_to_name(err));
        return err;
    }

    err = sensors_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Sensor init failed: %s", esp_err_to_name(err));
        return err;
    }

    err = pump_driver_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Pump driver init failed: %s", esp_err_to_name(err));
        return err;
    }

    err = scheduler_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Scheduler init failed: %s", esp_err_to_name(err));
        return err;
    }

    err = price_client_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Price client init failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

void pool_pump_app_run(void) {
    while (true) {
        if (networking_connect() == ESP_OK) {
            price_client_schedule_t schedule = {0};
            if (price_client_fetch_schedule(&schedule) == ESP_OK) {
                scheduler_plan_schedule(&schedule);
            }
        }

        scheduler_execute();
        sensors_poll();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
