#include "pump_controller.h"
#include "relay_control.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <string.h>

static const char *TAG = "pump_controller";

// LilyGO T-Relay GPIO pins (adjust based on your hardware)
#define RELAY_1_GPIO  GPIO_NUM_21
#define RELAY_2_GPIO  GPIO_NUM_19
#define RELAY_3_GPIO  GPIO_NUM_18
#define RELAY_4_GPIO  GPIO_NUM_5

// Map modes to relay configurations for AquaForte Vario+ II
// Digital inputs: IN1, IN2, IN3 control speed
typedef struct {
    bool relay1;  // IN1
    bool relay2;  // IN2
    bool relay3;  // IN3
    int rpm;
} pump_config_t;

static const pump_config_t pump_configs[] = {
    [PUMP_MODE_OFF] = {false, false, false, 0},
    [PUMP_MODE_NIGHT] = {true, false, false, 1400},
    [PUMP_MODE_DAY] = {false, true, false, 2000},
    [PUMP_MODE_BACKWASH] = {true, true, false, 2900}
};

static pump_status_t current_status = {
    .mode = PUMP_MODE_OFF,
    .runtime_hours = 0,
    .is_running = false,
    .current_rpm = 0
};

esp_err_t pump_controller_init(void)
{
    esp_err_t ret = relay_control_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize relay control");
        return ret;
    }

    // Ensure pump starts in OFF state
    current_status.mode = PUMP_MODE_OFF;
    current_status.is_running = false;
    current_status.current_rpm = 0;

    ESP_LOGI(TAG, "Pump controller initialized");
    return ESP_OK;
}

esp_err_t pump_controller_set_mode(pump_mode_t mode)
{
    if (mode < PUMP_MODE_OFF || mode > PUMP_MODE_BACKWASH) {
        ESP_LOGE(TAG, "Invalid pump mode: %d", mode);
        return ESP_ERR_INVALID_ARG;
    }

    const pump_config_t *config = &pump_configs[mode];

    // Set relay states for the selected mode
    esp_err_t ret = ESP_OK;
    ret |= relay_control_set(RELAY_1, config->relay1);
    ret |= relay_control_set(RELAY_2, config->relay2);
    ret |= relay_control_set(RELAY_3, config->relay3);

    if (ret == ESP_OK) {
        current_status.mode = mode;
        current_status.current_rpm = config->rpm;
        ESP_LOGI(TAG, "Pump mode set to %d (RPM: %d)", mode, config->rpm);
    } else {
        ESP_LOGE(TAG, "Failed to set pump mode");
    }

    return ret;
}

esp_err_t pump_controller_get_status(pump_status_t* status)
{
    if (status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(status, &current_status, sizeof(pump_status_t));
    return ESP_OK;
}

esp_err_t pump_controller_start(void)
{
    if (current_status.mode == PUMP_MODE_OFF) {
        ESP_LOGW(TAG, "Cannot start pump in OFF mode");
        return ESP_ERR_INVALID_STATE;
    }

    current_status.is_running = true;
    ESP_LOGI(TAG, "Pump started in mode %d", current_status.mode);
    return ESP_OK;
}

esp_err_t pump_controller_stop(void)
{
    current_status.is_running = false;
    current_status.mode = PUMP_MODE_OFF;
    current_status.current_rpm = 0;

    // Turn off all relays
    esp_err_t ret = relay_control_all_off();

    ESP_LOGI(TAG, "Pump stopped");
    return ret;
}

esp_err_t pump_controller_run_backwash(int duration_minutes)
{
    ESP_LOGI(TAG, "Starting backwash cycle for %d minutes", duration_minutes);

    esp_err_t ret = pump_controller_set_mode(PUMP_MODE_BACKWASH);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = pump_controller_start();
    if (ret != ESP_OK) {
        return ret;
    }

    // Note: In a real implementation, you'd set up a timer
    // For now, this is just the basic structure

    return ESP_OK;
}