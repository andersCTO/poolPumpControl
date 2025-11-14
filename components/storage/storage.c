#include "pool_pump/storage.h"

#include <string.h>

#include "esp_log.h"

static const char *TAG = "storage";
static storage_network_config_t s_network_config = {0};

esp_err_t storage_init(void) {
    ESP_LOGI(TAG, "Initializing non-volatile storage");
    memset(&s_network_config, 0, sizeof(s_network_config));
    return ESP_OK;
}

esp_err_t storage_load_network_config(storage_network_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *config = s_network_config;
    return ESP_OK;
}

esp_err_t storage_save_network_config(const storage_network_config_t *config) {
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    s_network_config = *config;
    ESP_LOGI(TAG, "Persisted network configuration");
    return ESP_OK;
}
