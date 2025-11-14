#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include "esp_err.h"
#include <stdbool.h>

typedef enum {
    PUMP_MODE_OFF = 0,
    PUMP_MODE_NIGHT,    // 1400 RPM
    PUMP_MODE_DAY,      // 2000 RPM
    PUMP_MODE_BACKWASH  // 2900 RPM
} pump_mode_t;

typedef struct {
    pump_mode_t mode;
    int runtime_hours;
    bool is_running;
    int current_rpm;
} pump_status_t;

/**
 * @brief Initialize pump controller
 * @return ESP_OK on success
 */
esp_err_t pump_controller_init(void);

/**
 * @brief Set pump operating mode
 * @param mode Pump operating mode
 * @return ESP_OK on success
 */
esp_err_t pump_controller_set_mode(pump_mode_t mode);

/**
 * @brief Get current pump status
 * @param status Pointer to status structure
 * @return ESP_OK on success
 */
esp_err_t pump_controller_get_status(pump_status_t* status);

/**
 * @brief Start pump operation
 * @return ESP_OK on success
 */
esp_err_t pump_controller_start(void);

/**
 * @brief Stop pump operation
 * @return ESP_OK on success
 */
esp_err_t pump_controller_stop(void);

/**
 * @brief Run backwash cycle
 * @param duration_minutes Duration of backwash in minutes
 * @return ESP_OK on success
 */
esp_err_t pump_controller_run_backwash(int duration_minutes);

#endif // PUMP_CONTROLLER_H
