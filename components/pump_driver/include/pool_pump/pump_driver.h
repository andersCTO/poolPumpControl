#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PUMP_DRIVER_MODE_NIGHT = 0,
    PUMP_DRIVER_MODE_DAY,
    PUMP_DRIVER_MODE_BACKWASH,
    PUMP_DRIVER_MODE_SELF_PRIME
} pump_driver_mode_t;

esp_err_t pump_driver_init(void);
esp_err_t pump_driver_set_mode(pump_driver_mode_t mode);
pump_driver_mode_t pump_driver_get_mode(void);
esp_err_t pump_driver_request_prime(bool enable);

#ifdef __cplusplus
}
#endif
