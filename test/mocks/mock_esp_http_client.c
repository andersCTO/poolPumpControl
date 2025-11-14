/**
 * @file mock_esp_http_client.c
 * @brief Mock implementation of ESP-IDF HTTP client functions for unit testing
 */

#include "mock_esp_http_client.h"
#include "esp_err.h"
#include <string.h>
#include <stdlib.h>

// Mock state
static char *mock_response_data = NULL;
static size_t mock_response_length = 0;
static int mock_status_code = 200;
static int mock_content_length = 0;
static esp_http_client_config_t mock_config;

// Mock function implementations
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *config)
{
    if (config == NULL) {
        return NULL;
    }

    memcpy(&mock_config, config, sizeof(esp_http_client_config_t));
    return (esp_http_client_handle_t)1; // Return non-NULL handle
}

esp_err_t esp_http_client_cleanup(esp_http_client_handle_t client)
{
    (void)client; // Unused in mock
    return ESP_OK;
}

esp_err_t esp_http_client_perform(esp_http_client_handle_t client)
{
    (void)client; // Unused in mock

    // Simulate HTTP request - trigger event handler if set
    if (mock_config.event_handler) {
        // Create mock events
        esp_http_client_event_t event = {
            .event_id = HTTP_EVENT_ON_DATA,
            .client = client,
            .data = mock_response_data,
            .data_len = mock_response_length,
            .user_data = mock_config.user_data
        };

        mock_config.event_handler(&event);

        // Finish event
        event.event_id = HTTP_EVENT_ON_FINISH;
        event.data = NULL;
        event.data_len = 0;
        mock_config.event_handler(&event);
    }

    return ESP_OK;
}

int esp_http_client_get_status_code(esp_http_client_handle_t client)
{
    (void)client; // Unused in mock
    return mock_status_code;
}

int esp_http_client_get_content_length(esp_http_client_handle_t client)
{
    (void)client; // Unused in mock
    return mock_content_length;
}

esp_err_t esp_http_client_set_header(esp_http_client_handle_t client, const char *key, const char *value)
{
    (void)client; // Unused in mock
    (void)key;    // Unused in mock
    (void)value;  // Unused in mock
    return ESP_OK;
}

esp_err_t esp_http_client_get_header(esp_http_client_handle_t client, const char *key, char **value)
{
    (void)client; // Unused in mock
    (void)key;    // Unused in mock
    (void)value;  // Unused in mock
    return ESP_OK;
}

// Test control functions
void mock_http_client_set_response_data(const char *data, size_t length)
{
    if (mock_response_data) {
        free(mock_response_data);
    }

    if (data && length > 0) {
        mock_response_data = malloc(length + 1);
        if (mock_response_data) {
            memcpy(mock_response_data, data, length);
            mock_response_data[length] = '\0';
            mock_response_length = length;
        }
    } else {
        mock_response_data = NULL;
        mock_response_length = 0;
    }
}

void mock_http_client_set_status_code(int status_code)
{
    mock_status_code = status_code;
}

void mock_http_client_set_content_length(int length)
{
    mock_content_length = length;
}

void mock_http_client_reset(void)
{
    if (mock_response_data) {
        free(mock_response_data);
        mock_response_data = NULL;
    }
    mock_response_length = 0;
    mock_status_code = 200;
    mock_content_length = 0;
    memset(&mock_config, 0, sizeof(mock_config));
}