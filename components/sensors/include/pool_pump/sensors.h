#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sensors_init(void);
void sensors_poll(void);
esp_err_t sensors_get_water_temperature(float *temperature_c);

#ifdef __cplusplus
}
#endif

