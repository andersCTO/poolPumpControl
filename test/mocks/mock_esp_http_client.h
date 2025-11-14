/**
 * @file mock_esp_http_client.h
 * @brief Mock implementation of ESP-IDF HTTP client functions for unit testing
 */

#ifndef MOCK_ESP_HTTP_CLIENT_H
#define MOCK_ESP_HTTP_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Mock HTTP client types
typedef void* esp_http_client_handle_t;

typedef enum {
    HTTP_EVENT_ERROR = 0,
    HTTP_EVENT_ON_CONNECTED,
    HTTP_EVENT_HEADER_SENT,
    HTTP_EVENT_ON_HEADER,
    HTTP_EVENT_ON_DATA,
    HTTP_EVENT_ON_FINISH,
    HTTP_EVENT_DISCONNECTED,
} esp_http_client_event_id_t;

typedef struct {
    esp_http_client_event_id_t event_id;
    esp_http_client_handle_t client;
    void *data;
    int data_len;
    void *user_data;
    char *header_key;
    char *header_value;
} esp_http_client_event_t;

typedef void (*http_event_handle_cb)(esp_http_client_event_t *evt);

typedef struct {
    const char *url;
    const char *method;
    http_event_handle_cb event_handler;
    void *user_data;
    int timeout_ms;
    bool disable_auto_redirect;
} esp_http_client_config_t;

// Mock function declarations
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *config);
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t client);
esp_err_t esp_http_client_perform(esp_http_client_handle_t client);
int esp_http_client_get_status_code(esp_http_client_handle_t client);
int esp_http_client_get_content_length(esp_http_client_handle_t client);
esp_err_t esp_http_client_set_header(esp_http_client_handle_t client, const char *key, const char *value);
esp_err_t esp_http_client_get_header(esp_http_client_handle_t client, const char *key, char **value);

// Test control functions
void mock_http_client_set_response_data(const char *data, size_t length);
void mock_http_client_set_status_code(int status_code);
void mock_http_client_set_content_length(int length);
void mock_http_client_reset(void);

#endif // MOCK_ESP_HTTP_CLIENT_H