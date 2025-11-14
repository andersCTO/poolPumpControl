#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char wifi_ssid[32];
    char wifi_password[64];
    char price_api_endpoint[128];
} storage_network_config_t;

esp_err_t storage_init(void);
esp_err_t storage_load_network_config(storage_network_config_t *config);
esp_err_t storage_save_network_config(const storage_network_config_t *config);

#ifdef __cplusplus
}
#endif
