#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

#include "esp_err.h"
#include <stdbool.h>

typedef enum {
    RELAY_1 = 0,  // DI2 - Night mode (1400 RPM)
    RELAY_2,      // DI3 - Day mode (2000 RPM)
    RELAY_3,      // DI4 - Backwash mode (2900 RPM)
    RELAY_4,      // Reserved for heater control
    RELAY_MAX
} relay_num_t;

/**
 * @brief Initialize relay control system
 * @return ESP_OK on success
 */
esp_err_t relay_control_init(void);

/**
 * @brief Set relay state
 * @param relay_num Relay number (0-3)
 * @param state true to activate, false to deactivate
 * @return ESP_OK on success
 */
esp_err_t relay_control_set(relay_num_t relay_num, bool state);

/**
 * @brief Get relay state
 * @param relay_num Relay number (0-3)
 * @param state Pointer to store state
 * @return ESP_OK on success
 */
esp_err_t relay_control_get(relay_num_t relay_num, bool* state);

/**
 * @brief Turn off all relays
 * @return ESP_OK on success
 */
esp_err_t relay_control_all_off(void);

/**
 * @brief Set pump mode via relay control
 * @param mode 0=off, 1=night, 2=day, 3=backwash
 * @return ESP_OK on success
 */
esp_err_t relay_control_set_pump_mode(int mode);

#endif // RELAY_CONTROL_H
