#ifndef MOCK_ESP_CRT_BUNDLE_H
#define MOCK_ESP_CRT_BUNDLE_H

#include "esp_err.h"

// Mock for TLS certificate bundle attachment
// In ESP-IDF, this function is used to attach the certificate bundle for HTTPS
typedef esp_err_t (*esp_crt_bundle_attach_fn)(void *conf);

esp_err_t esp_crt_bundle_attach(void *conf);

#endif // MOCK_ESP_CRT_BUNDLE_H
