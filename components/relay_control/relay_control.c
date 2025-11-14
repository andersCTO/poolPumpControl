#include "relay_control.h"
#include "config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "RELAY_CONTROL";

static bool relay_states[RELAY_MAX] = {false};

static gpio_num_t relay_pin_from_num(relay_num_t relay_num) {
    switch (relay_num) {
        case RELAY_1:
            return RELAY_1_PIN;
        case RELAY_2:
            return RELAY_2_PIN;
        case RELAY_3:
            return RELAY_3_PIN;
        case RELAY_4:
            return RELAY_4_PIN;
        default:
            return GPIO_NUM_NC;
    }
}

esp_err_t relay_control_init(void) {
    ESP_LOGI(TAG, "Initializing relay control...");

    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    // Configure all relay pins
    io_conf.pin_bit_mask =
        (1ULL << RELAY_1_PIN) | (1ULL << RELAY_2_PIN) | (1ULL << RELAY_3_PIN) | (1ULL << RELAY_4_PIN);

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO pins");
        return ret;
    }

    // Initialize all relays to OFF state
    relay_control_all_off();

    ESP_LOGI(TAG, "Relay control initialized successfully");
    return ESP_OK;
}

esp_err_t relay_control_set(relay_num_t relay_num, bool state) {
    if (relay_num >= RELAY_MAX) {
        ESP_LOGE(TAG, "Invalid relay number: %d", relay_num);
        return ESP_ERR_INVALID_ARG;
    }

    gpio_num_t pin = relay_pin_from_num(relay_num);
    esp_err_t ret = gpio_set_level(pin, state);
    if (ret == ESP_OK) {
        relay_states[relay_num] = state;
        ESP_LOGI(TAG, "Relay %d set to %s", relay_num, state ? "ON" : "OFF");
    }

    return ret;
}

esp_err_t relay_control_get(relay_num_t relay_num, bool *state) {
    if (relay_num >= RELAY_MAX || state == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *state = relay_states[relay_num];
    return ESP_OK;
}

esp_err_t relay_control_all_off(void) {
    ESP_LOGI(TAG, "Turning off all relays");

    for (int i = 0; i < RELAY_MAX; i++) {
        gpio_set_level(relay_pin_from_num((relay_num_t)i), 0);
        relay_states[i] = false;
    }

    return ESP_OK;
}

esp_err_t relay_control_set_pump_mode(int mode) {
    // First turn off all pump relays
    relay_control_set(RELAY_1, false); // Night mode off
    relay_control_set(RELAY_2, false); // Day mode off
    relay_control_set(RELAY_3, false); // Backwash mode off

    // Small delay to ensure clean switching
    vTaskDelay(pdMS_TO_TICKS(100));

    switch (mode) {
        case 0: // OFF
            ESP_LOGI(TAG, "Pump mode set to OFF");
            break;
        case 1: // Night mode (1400 RPM)
            relay_control_set(RELAY_1, true);
            ESP_LOGI(TAG, "Pump mode set to NIGHT (1400 RPM)");
            break;
        case 2: // Day mode (2000 RPM)
            relay_control_set(RELAY_2, true);
            ESP_LOGI(TAG, "Pump mode set to DAY (2000 RPM)");
            break;
        case 3: // Backwash mode (2900 RPM)
            relay_control_set(RELAY_3, true);
            ESP_LOGI(TAG, "Pump mode set to BACKWASH (2900 RPM)");
            break;
        default:
            ESP_LOGE(TAG, "Invalid pump mode: %d", mode);
            return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}
