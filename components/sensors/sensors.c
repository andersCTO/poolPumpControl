#include "pool_pump/sensors.h"

#include "esp_log.h"

static const char *TAG = "sensors";

esp_err_t sensors_init(void)
{
    ESP_LOGI(TAG, "Initializing temperature and flow sensors");
    return ESP_OK;
}

void sensors_poll(void)
{
    ESP_LOGD(TAG, "Polling sensors for new readings");
}

esp_err_t sensors_get_water_temperature(float *temperature_c)
{
    if (temperature_c == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *temperature_c = 0.0f;
    return ESP_OK;
}
