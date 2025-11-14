/**
 * @file mock_esp_wifi.h
 * @brief Mock implementation of ESP-IDF WiFi functions for unit testing
 */

#ifndef MOCK_ESP_WIFI_H
#define MOCK_ESP_WIFI_H

#include <stdint.h>
#include <stdbool.h>

// Mock WiFi types and structures
typedef enum {
    WIFI_MODE_NULL = 0,
    WIFI_MODE_STA,
    WIFI_MODE_AP,
    WIFI_MODE_APSTA,
} wifi_mode_t;

typedef enum {
    WIFI_AUTH_OPEN = 0,
    WIFI_AUTH_WEP,
    WIFI_AUTH_WPA_PSK,
    WIFI_AUTH_WPA2_PSK,
    WIFI_AUTH_WPA_WPA2_PSK,
    WIFI_AUTH_WPA2_ENTERPRISE,
    WIFI_AUTH_WPA3_PSK,
    WIFI_AUTH_WPA2_WPA3_PSK,
    WIFI_AUTH_WEP_SHARED,
    WIFI_AUTH_MAX
} wifi_auth_mode_t;

typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
    wifi_auth_mode_t threshold_authmode;
    struct {
        bool capable;
        bool required;
    } pmf_cfg;
} wifi_sta_config_t;

typedef struct {
    wifi_mode_t mode;
    wifi_sta_config_t sta;
} wifi_config_t;

typedef enum {
    WIFI_IF_STA = 0,
    WIFI_IF_AP,
} wifi_interface_t;

// Mock function declarations
esp_err_t esp_wifi_init(const wifi_init_config_t *config);
esp_err_t esp_wifi_deinit(void);
esp_err_t esp_wifi_set_mode(wifi_mode_t mode);
esp_err_t esp_wifi_get_mode(wifi_mode_t *mode);
esp_err_t esp_wifi_set_config(wifi_interface_t interface, const wifi_config_t *conf);
esp_err_t esp_wifi_get_config(wifi_interface_t interface, wifi_config_t *conf);
esp_err_t esp_wifi_start(void);
esp_err_t esp_wifi_stop(void);
esp_err_t esp_wifi_connect(void);
esp_err_t esp_wifi_disconnect(void);

// Test control functions
void mock_esp_wifi_set_connected(bool connected);
bool mock_esp_wifi_is_connected(void);
void mock_esp_wifi_reset(void);

#endif // MOCK_ESP_WIFI_H