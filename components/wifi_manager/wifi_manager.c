#include "wifi_manager.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "WIFI_MANAGER";

static bool wifi_connected = false;

esp_err_t wifi_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing WiFi manager...");

    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize netif: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "WiFi manager initialized");
    return ESP_OK;
}

esp_err_t wifi_manager_connect(const char* ssid, const char* password)
{
    ESP_LOGI(TAG, "Connecting to WiFi: %s", ssid);

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) return ret;

    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) return ret;

    ret = esp_wifi_start();
    if (ret != ESP_OK) return ret;

    ret = esp_wifi_connect();
    if (ret != ESP_OK) return ret;

    ESP_LOGI(TAG, "WiFi connect initiated");
    return ESP_OK;
}

bool wifi_manager_is_connected(void)
{
    return wifi_connected;
}

esp_err_t wifi_manager_disconnect(void)
{
    ESP_LOGI(TAG, "Disconnecting WiFi");
    wifi_connected = false;
    return esp_wifi_disconnect();
}