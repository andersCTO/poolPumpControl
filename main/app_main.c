#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "price_fetcher.h"
#include "pump_controller.h"
#include "relay_control.h"
#include "web_server.h"
#include "wifi_manager.h"

static const char *TAG = "POOL_PUMP_MAIN";

static void init_sntp(void) {
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    // Set timezone to CET/CEST (Stockholm)
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
}

void app_main(void) {
    ESP_LOGI(TAG, "Pool Pump Controller starting...");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize components
    config_init();
    wifi_manager_init();
    relay_control_init();
    pump_controller_init();
    price_fetcher_init();
    web_server_init();

    // Initialize SNTP for time sync (required by scheduler regardless of WiFi source)
    init_sntp();

    // Connect to WiFi if credentials are configured
#ifdef CONFIG_POOL_PUMP_WIFI_SSID
    if (strlen(CONFIG_POOL_PUMP_WIFI_SSID) > 0) {
        ESP_LOGI(TAG, "Connecting to WiFi: %s", CONFIG_POOL_PUMP_WIFI_SSID);
        wifi_manager_connect(CONFIG_POOL_PUMP_WIFI_SSID, CONFIG_POOL_PUMP_WIFI_PASSWORD);
    } else {
        ESP_LOGW(TAG, "No WiFi credentials configured - use BLE to provision");
    }
#else
    ESP_LOGW(TAG, "No WiFi credentials configured - use BLE to provision");
#endif

    ESP_LOGI(TAG, "Pool Pump Controller initialized successfully");

    // Start main application tasks
    xTaskCreate(&pump_scheduler_task, "pump_scheduler", 4096, NULL, 5, NULL);
    xTaskCreate(&price_refresh_task, "price_refresh", 4096, NULL, 4, NULL);
}
