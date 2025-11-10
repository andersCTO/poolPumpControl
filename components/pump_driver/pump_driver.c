#include "pool_pump/pump_driver.h"

#include "esp_log.h"

static const char *TAG = "pump_driver";
static pump_driver_mode_t s_current_mode = PUMP_DRIVER_MODE_NIGHT;

esp_err_t pump_driver_init(void)
{
    ESP_LOGI(TAG, "Initializing pump driver");
    s_current_mode = PUMP_DRIVER_MODE_SELF_PRIME;
    return ESP_OK;
}

esp_err_t pump_driver_set_mode(pump_driver_mode_t mode)
{
    ESP_LOGI(TAG, "Setting pump mode %d", mode);
    s_current_mode = mode;
    return ESP_OK;
}

pump_driver_mode_t pump_driver_get_mode(void)
{
    return s_current_mode;
}

esp_err_t pump_driver_request_prime(bool enable)
{
    ESP_LOGI(TAG, "Priming request: %s", enable ? "enabled" : "disabled");
    return ESP_OK;
}
