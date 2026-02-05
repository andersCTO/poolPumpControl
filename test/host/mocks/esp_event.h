#ifndef MOCK_ESP_EVENT_H
#define MOCK_ESP_EVENT_H

#include "esp_err.h"

typedef void *esp_event_loop_handle_t;

static inline esp_err_t esp_event_loop_create_default(void) { return ESP_OK; }

#endif // MOCK_ESP_EVENT_H
