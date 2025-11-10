#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t networking_init(void);
esp_err_t networking_connect(void);
esp_err_t networking_get_time(void);

#ifdef __cplusplus
}
#endif

