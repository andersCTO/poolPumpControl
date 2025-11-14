#ifndef BLUETOOTH_CONFIG_H
#define BLUETOOTH_CONFIG_H

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief Bluetooth Configuration Module
 *
 * Provides BLE interface for mobile app configuration
 * Initial focus: WiFi credentials configuration
 * Future: All system settings configurable via mobile app
 */

// Configuration types
typedef enum {
    BT_CONFIG_TYPE_WIFI_CREDENTIALS = 0,
    BT_CONFIG_TYPE_PUMP_SETTINGS,
    BT_CONFIG_TYPE_PRICE_THRESHOLDS,
    BT_CONFIG_TYPE_SCHEDULE,
    BT_CONFIG_TYPE_SYSTEM_INFO
} bt_config_type_t;

// WiFi credentials structure
typedef struct {
    char ssid[32];
    char password[64];
} bt_wifi_credentials_t;

// Pump settings structure
typedef struct {
    uint16_t speed_night;
    uint16_t speed_day;
    uint16_t speed_backwash;
    uint8_t min_runtime_hours;
    uint8_t max_runtime_hours;
} bt_pump_settings_t;

// Price threshold settings
typedef struct {
    float threshold_low;
    float threshold_high;
    uint8_t fetch_interval_hours;
} bt_price_settings_t;

// System information
typedef struct {
    char device_name[32];
    char firmware_version[16];
    uint32_t uptime_seconds;
    bool wifi_connected;
    int8_t wifi_rssi;
} bt_system_info_t;

// Configuration callback function type
typedef void (*bt_config_callback_t)(bt_config_type_t type, void *data, size_t len);

/**
 * @brief Initialize Bluetooth configuration module
 *
 * Sets up BLE GATT server with configuration services
 *
 * @param device_name Name visible to mobile devices
 * @param callback Callback for configuration changes
 * @return ESP_OK on success
 */
esp_err_t bluetooth_config_init(const char *device_name, bt_config_callback_t callback);

/**
 * @brief Deinitialize Bluetooth configuration
 *
 * @return ESP_OK on success
 */
esp_err_t bluetooth_config_deinit(void);

/**
 * @brief Start BLE advertising
 *
 * Makes device discoverable to mobile apps
 *
 * @return ESP_OK on success
 */
esp_err_t bluetooth_config_start_advertising(void);

/**
 * @brief Stop BLE advertising
 *
 * @return ESP_OK on success
 */
esp_err_t bluetooth_config_stop_advertising(void);

/**
 * @brief Check if a device is connected
 *
 * @return true if connected, false otherwise
 */
bool bluetooth_config_is_connected(void);

/**
 * @brief Send system information to connected device
 *
 * @param info System information structure
 * @return ESP_OK on success
 */
esp_err_t bluetooth_config_send_system_info(const bt_system_info_t *info);

/**
 * @brief Send notification to connected device
 *
 * @param message Notification message
 * @return ESP_OK on success
 */
esp_err_t bluetooth_config_send_notification(const char *message);

#endif // BLUETOOTH_CONFIG_H
