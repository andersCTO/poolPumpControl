#ifndef MOCK_ESP_NETIF_H
#define MOCK_ESP_NETIF_H

#include "esp_err.h"

typedef void *esp_netif_t;

static inline esp_err_t esp_netif_init(void) { return ESP_OK; }
static inline esp_netif_t *esp_netif_create_default_wifi_sta(void) { return NULL; }

#endif // MOCK_ESP_NETIF_H
