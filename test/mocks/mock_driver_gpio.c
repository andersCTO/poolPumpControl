/**
 * @file mock_driver_gpio.c
 * @brief Mock implementation of ESP-IDF GPIO functions for unit testing
 */

#include "mock_driver_gpio.h"
#include "esp_err.h"
#include <string.h>

// Mock GPIO state - simulate up to 40 GPIO pins
#define MAX_GPIO_PINS 40
static int mock_gpio_levels[MAX_GPIO_PINS];
static bool mock_gpio_initialized[MAX_GPIO_PINS];

// Mock function implementations
esp_err_t gpio_config(const gpio_config_t *pGPIOConfig) {
    (void)pGPIOConfig; // Unused in mock - assume success
    return ESP_OK;
}

esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level) {
    if (gpio_num < 0 || gpio_num >= MAX_GPIO_PINS) {
        return ESP_ERR_INVALID_ARG;
    }

    mock_gpio_levels[gpio_num] = level;
    mock_gpio_initialized[gpio_num] = true;
    return ESP_OK;
}

int gpio_get_level(gpio_num_t gpio_num) {
    if (gpio_num < 0 || gpio_num >= MAX_GPIO_PINS) {
        return -1;
    }

    return mock_gpio_levels[gpio_num];
}

esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode) {
    (void)mode; // Unused in mock
    if (gpio_num < 0 || gpio_num >= MAX_GPIO_PINS) {
        return ESP_ERR_INVALID_ARG;
    }

    mock_gpio_initialized[gpio_num] = true;
    return ESP_OK;
}

// Test control functions
void mock_gpio_set_pin_level(gpio_num_t pin, int level) {
    if (pin >= 0 && pin < MAX_GPIO_PINS) {
        mock_gpio_levels[pin] = level;
        mock_gpio_initialized[pin] = true;
    }
}

int mock_gpio_get_pin_level(gpio_num_t pin) {
    if (pin >= 0 && pin < MAX_GPIO_PINS) {
        return mock_gpio_levels[pin];
    }
    return -1;
}

void mock_gpio_reset(void) {
    memset(mock_gpio_levels, 0, sizeof(mock_gpio_levels));
    memset(mock_gpio_initialized, 0, sizeof(mock_gpio_initialized));
}