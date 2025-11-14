#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initialize WiFi manager
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Connect to WiFi network
 * @param ssid WiFi network SSID
 * @param password WiFi network password
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_connect(const char *ssid, const char *password);

/**
 * @brief Check if WiFi is connected
 * @return true if connected, false otherwise
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Disconnect from WiFi
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_disconnect(void);

#endif // WIFI_MANAGER_H
