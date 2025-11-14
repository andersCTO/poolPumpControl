/**
 * @file mock_esp_wifi.c
 * @brief Mock implementation of ESP-IDF WiFi functions for unit testing
 */

#include "mock_esp_wifi.h"
#include "esp_err.h"
#include <string.h>
#include <stdbool.h>

// Mock state
static bool mock_connected = false;
static wifi_mode_t mock_mode = WIFI_MODE_NULL;
static wifi_config_t mock_config = {0};

// Mock function implementations
esp_err_t esp_wifi_init(const wifi_init_config_t *config)
{
    (void)config; // Unused in mock
    return ESP_OK;
}

esp_err_t esp_wifi_deinit(void)
{
    return ESP_OK;
}

esp_err_t esp_wifi_set_mode(wifi_mode_t mode)
{
    mock_mode = mode;
    return ESP_OK;
}

esp_err_t esp_wifi_get_mode(wifi_mode_t *mode)
{
    if (mode == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *mode = mock_mode;
    return ESP_OK;
}

esp_err_t esp_wifi_set_config(wifi_interface_t interface, const wifi_config_t *conf)
{
    (void)interface; // Unused in mock
    if (conf == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    memcpy(&mock_config, conf, sizeof(wifi_config_t));
    return ESP_OK;
}

esp_err_t esp_wifi_get_config(wifi_interface_t interface, wifi_config_t *conf)
{
    (void)interface; // Unused in mock
    if (conf == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    memcpy(conf, &mock_config, sizeof(wifi_config_t));
    return ESP_OK;
}

esp_err_t esp_wifi_start(void)
{
    return ESP_OK;
}

esp_err_t esp_wifi_stop(void)
{
    return ESP_OK;
}

esp_err_t esp_wifi_connect(void)
{
    mock_connected = true;
    return ESP_OK;
}

esp_err_t esp_wifi_disconnect(void)
{
    mock_connected = false;
    return ESP_OK;
}

// Test control functions
void mock_esp_wifi_set_connected(bool connected)
{
    mock_connected = connected;
}

bool mock_esp_wifi_is_connected(void)
{
    return mock_connected;
}

void mock_esp_wifi_reset(void)
{
    mock_connected = false;
    mock_mode = WIFI_MODE_NULL;
    memset(&mock_config, 0, sizeof(wifi_config_t));
}