#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "config.h"
#include "wifi_manager.h"
#include "price_fetcher.h"
#include "pump_controller.h"
#include "relay_control.h"

static const char *TAG = "POOL_PUMP_MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Pool Pump Controller starting...");
        
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
        
    // Initialize networking
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
        
    // Initialize components
    config_init();
    wifi_manager_init();
    relay_control_init();
    pump_controller_init();
    price_fetcher_init();
        
    ESP_LOGI(TAG, "Pool Pump Controller initialized successfully");
        
    // Start main application task
    xTaskCreate(&pump_scheduler_task, "pump_scheduler", 4096, NULL, 5, NULL);
}
