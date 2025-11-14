/**
 * @file mock_driver_gpio.h
 * @brief Mock implementation of ESP-IDF GPIO functions for unit testing
 */

#ifndef MOCK_DRIVER_GPIO_H
#define MOCK_DRIVER_GPIO_H

#include <stdbool.h>
#include <stdint.h>

// Mock GPIO types
typedef int gpio_num_t;
typedef int gpio_mode_t;
typedef int gpio_pullup_t;
typedef int gpio_pulldown_t;
typedef int gpio_int_type_t;

// Mock GPIO constants
#define GPIO_MODE_DISABLE 0
#define GPIO_MODE_INPUT 1
#define GPIO_MODE_OUTPUT 2
#define GPIO_MODE_OUTPUT_OD 3
#define GPIO_MODE_INPUT_OUTPUT_OD 4
#define GPIO_MODE_INPUT_OUTPUT 5

#define GPIO_PULLUP_DISABLE 0
#define GPIO_PULLUP_ENABLE 1
#define GPIO_PULLDOWN_DISABLE 0
#define GPIO_PULLDOWN_ENABLE 1
#define GPIO_INTR_DISABLE 0

#define GPIO_NUM_NC -1

// Mock GPIO pin bit mask type
typedef uint64_t gpio_config_t;

// Mock function declarations
esp_err_t gpio_config(const gpio_config_t *pGPIOConfig);
esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level);
int gpio_get_level(gpio_num_t gpio_num);
esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode);

// Test control functions
void mock_gpio_set_pin_level(gpio_num_t pin, int level);
int mock_gpio_get_pin_level(gpio_num_t pin);
void mock_gpio_reset(void);

#endif // MOCK_DRIVER_GPIO_H