#include "pool_pump/scheduler.h"

#include "pool_pump/pump_driver.h"

#include "esp_log.h"

static const char *TAG = "scheduler";

esp_err_t scheduler_init(void) {
    ESP_LOGI(TAG, "Initializing scheduler");
    return ESP_OK;
}

esp_err_t scheduler_plan_schedule(const price_client_schedule_t *schedule) {
    if (schedule == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Planning pump operation for %zu entries", schedule->count);
    return ESP_OK;
}

void scheduler_execute(void) {
    static pump_driver_mode_t last_mode = PUMP_DRIVER_MODE_NIGHT;
    pump_driver_mode_t mode = pump_driver_get_mode();
    if (mode != last_mode) {
        pump_driver_set_mode(mode);
        last_mode = mode;
    }
}
