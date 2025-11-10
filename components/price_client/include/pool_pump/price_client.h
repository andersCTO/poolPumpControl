#pragma once

#include <stddef.h>
#include <time.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    time_t timestamp;
    float price_eur_per_mwh;
} price_client_entry_t;

typedef struct {
    price_client_entry_t entries[48];
    size_t count;
} price_client_schedule_t;

esp_err_t price_client_init(void);
esp_err_t price_client_fetch_schedule(price_client_schedule_t *schedule);

#ifdef __cplusplus
}
#endif

