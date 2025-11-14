#include "pool_pump/price_client.h"

#include "pool_pump/networking.h"

#include "esp_log.h"

static const char *TAG = "price_client";

esp_err_t price_client_init(void) {
    ESP_LOGI(TAG, "Initializing price client");
    return ESP_OK;
}

esp_err_t price_client_fetch_schedule(price_client_schedule_t *schedule) {
    if (schedule == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Fetching day-ahead electricity prices");
    schedule->count = 0;
    return ESP_OK;
}
