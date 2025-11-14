#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t pool_pump_app_init(void);
void pool_pump_app_run(void);

#ifdef __cplusplus
}
#endif
