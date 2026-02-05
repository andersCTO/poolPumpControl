#ifndef MOCK_ESP_WIFI_H
#define MOCK_ESP_WIFI_H

#include "esp_err.h"
#include "esp_netif.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef enum { WIFI_MODE_NULL = 0, WIFI_MODE_STA, WIFI_MODE_AP, WIFI_MODE_APSTA, WIFI_MODE_MAX } wifi_mode_t;

typedef enum { WIFI_IF_STA = 0, WIFI_IF_AP, WIFI_IF_MAX } wifi_interface_t;

typedef enum { WIFI_AUTH_OPEN = 0, WIFI_AUTH_WPA2_PSK = 3 } wifi_auth_mode_t;

typedef struct {
    struct {
        uint8_t ssid[32];
        uint8_t password[64];
        struct {
            wifi_auth_mode_t authmode;
        } threshold;
        struct {
            bool capable;
            bool required;
        } pmf_cfg;
    } sta;
} wifi_config_t;

typedef struct {
    int dummy;
} wifi_init_config_t;

#define WIFI_INIT_CONFIG_DEFAULT()                                                                                     \
    { .dummy = 0 }

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

// Test helpers
void mock_esp_wifi_set_connected(bool connected);
bool mock_esp_wifi_is_connected(void);
void mock_esp_wifi_reset(void);

#endif // MOCK_ESP_WIFI_H
