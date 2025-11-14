#pragma once

#include "esp_err.h"

#include "pool_pump/price_client.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t scheduler_init(void);
esp_err_t scheduler_plan_schedule(const price_client_schedule_t *schedule);
void scheduler_execute(void);

#ifdef __cplusplus
}
#endif

