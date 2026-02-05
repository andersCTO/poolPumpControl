#ifndef MOCK_ESP_LOG_H
#define MOCK_ESP_LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef enum {
    ESP_LOG_NONE,
    ESP_LOG_ERROR,
    ESP_LOG_WARN,
    ESP_LOG_INFO,
    ESP_LOG_DEBUG,
    ESP_LOG_VERBOSE
} esp_log_level_t;

#define ESP_LOGE(tag, fmt, ...) (void)(tag)
#define ESP_LOGW(tag, fmt, ...) (void)(tag)
#define ESP_LOGI(tag, fmt, ...) (void)(tag)
#define ESP_LOGD(tag, fmt, ...) (void)(tag)
#define ESP_LOGV(tag, fmt, ...) (void)(tag)

#endif // MOCK_ESP_LOG_H
