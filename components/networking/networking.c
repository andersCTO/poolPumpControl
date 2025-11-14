#include "pool_pump/networking.h"

#include "esp_log.h"

static const char *TAG = "networking";

esp_err_t networking_init(void) {
    ESP_LOGI(TAG, "Configuring Wi-Fi interfaces");
    return ESP_OK;
}

esp_err_t networking_connect(void) {
    ESP_LOGI(TAG, "Connecting to configured Wi-Fi network");
    return ESP_OK;
}

esp_err_t networking_get_time(void) {
    ESP_LOGI(TAG, "Syncing SNTP time");
    return ESP_OK;
}
