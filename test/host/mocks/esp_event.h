#ifndef MOCK_ESP_EVENT_H
#define MOCK_ESP_EVENT_H

#include "esp_err.h"
#include <stdint.h>

typedef void *esp_event_loop_handle_t;
typedef const char *esp_event_base_t;
typedef void *esp_event_handler_instance_t;

#define ESP_EVENT_ANY_ID (-1)
#define WIFI_EVENT ((esp_event_base_t) "WIFI_EVENT")
#define IP_EVENT ((esp_event_base_t) "IP_EVENT")

// WiFi event IDs
#define WIFI_EVENT_STA_START 0
#define WIFI_EVENT_STA_CONNECTED 1
#define WIFI_EVENT_STA_DISCONNECTED 2

// IP event IDs
#define IP_EVENT_STA_GOT_IP 0

typedef void (*esp_event_handler_t)(void *event_handler_arg,
                                    esp_event_base_t event_base,
                                    int32_t event_id,
                                    void *event_data);

static inline esp_err_t esp_event_loop_create_default(void) { return ESP_OK; }

static inline esp_err_t esp_event_handler_instance_register(esp_event_base_t event_base,
                                                            int32_t event_id,
                                                            esp_event_handler_t event_handler,
                                                            void *event_handler_arg,
                                                            esp_event_handler_instance_t *instance) {
    (void)event_base;
    (void)event_id;
    (void)event_handler;
    (void)event_handler_arg;
    (void)instance;
    return ESP_OK;
}

#endif // MOCK_ESP_EVENT_H
