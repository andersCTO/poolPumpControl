#include "nvs_storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "nvs_storage";
static const char *NVS_NAMESPACE = "pool_pump";

esp_err_t nvs_storage_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "NVS storage initialized");
    } else {
        ESP_LOGE(TAG, "Failed to initialize NVS storage: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t nvs_storage_set_wifi_credentials(const char* ssid, const char* password)
{
    esp_err_t ret = nvs_storage_save_string("wifi_ssid", ssid);
    if (ret != ESP_OK) return ret;
    return nvs_storage_save_string("wifi_pass", password);
}

esp_err_t nvs_storage_get_wifi_credentials(char* ssid, char* password)
{
    esp_err_t ret = nvs_storage_load_string("wifi_ssid", ssid, 32);
    if (ret != ESP_OK) return ret;
    return nvs_storage_load_string("wifi_pass", password, 64);
}

esp_err_t nvs_storage_set_pump_config(uint8_t mode, uint16_t daily_runtime)
{
    esp_err_t ret = nvs_storage_save_int("pump_mode", mode);
    if (ret != ESP_OK) return ret;
    return nvs_storage_save_int("daily_runtime", daily_runtime);
}

esp_err_t nvs_storage_get_pump_config(uint8_t* mode, uint16_t* daily_runtime)
{
    int32_t temp_mode, temp_runtime;
    esp_err_t ret = nvs_storage_load_int("pump_mode", &temp_mode);
    if (ret != ESP_OK) return ret;
    ret = nvs_storage_load_int("daily_runtime", &temp_runtime);
    if (ret != ESP_OK) return ret;
    *mode = (uint8_t)temp_mode;
    *daily_runtime = (uint16_t)temp_runtime;
    return ESP_OK;
}

esp_err_t nvs_storage_set_daily_stats(uint32_t date, uint16_t runtime_minutes, float avg_price)
{
    // For now, just store the runtime. Could be extended to store more stats
    return nvs_storage_save_int("last_runtime", runtime_minutes);
}

esp_err_t nvs_storage_save_string(const char* key, const char* value)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_set_str(handle, key, value);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write string: %s", esp_err_to_name(ret));
        nvs_close(handle);
        return ret;
    }

    ret = nvs_commit(handle);
    nvs_close(handle);

    ESP_LOGI(TAG, "Saved string: %s = %s", key, value);
    return ret;
}

esp_err_t nvs_storage_load_string(const char* key, char* buffer, size_t buffer_size)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }

    size_t required_size = buffer_size;
    ret = nvs_get_str(handle, key, buffer, &required_size);
    nvs_close(handle);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Loaded string: %s = %s", key, buffer);
    } else {
        ESP_LOGE(TAG, "Failed to read string: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t nvs_storage_save_int(const char* key, int32_t value)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;

    ret = nvs_set_i32(handle, key, value);
    if (ret == ESP_OK) {
        ret = nvs_commit(handle);
    }

    nvs_close(handle);
    ESP_LOGI(TAG, "Saved int: %s = %ld", key, (long)value);
    return ret;
}

esp_err_t nvs_storage_load_int(const char* key, int32_t* value)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK) return ret;

    ret = nvs_get_i32(handle, key, value);
    nvs_close(handle);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Loaded int: %s = %ld", key, (long)*value);
    }

    return ret;
}

esp_err_t nvs_storage_erase(const char* key)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;

    ret = nvs_erase_key(handle, key);
    if (ret == ESP_OK) {
        ret = nvs_commit(handle);
    }

    nvs_close(handle);
    ESP_LOGI(TAG, "Erased key: %s", key);
    return ret;
}