#ifndef NVS_STORAGE_H
#define NVS_STORAGE_H

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief Initialize NVS storage
 * @return ESP_OK on success
 */
esp_err_t nvs_storage_init(void);

/**
 * @brief Store WiFi credentials
 * @param ssid WiFi SSID
 * @param password WiFi password
 * @return ESP_OK on success
 */
esp_err_t nvs_storage_set_wifi_credentials(const char *ssid, const char *password);

/**
 * @brief Retrieve WiFi credentials
 * @param ssid Buffer for SSID (min 32 bytes)
 * @param password Buffer for password (min 64 bytes)
 * @return ESP_OK on success
 */
esp_err_t nvs_storage_get_wifi_credentials(char *ssid, char *password);

/**
 * @brief Store pump configuration
 * @param mode Current pump mode
 * @param daily_runtime Daily runtime in minutes
 * @return ESP_OK on success
 */
esp_err_t nvs_storage_set_pump_config(uint8_t mode, uint16_t daily_runtime);

/**
 * @brief Retrieve pump configuration
 * @param mode Pointer to store pump mode
 * @param daily_runtime Pointer to store daily runtime
 * @return ESP_OK on success
 */
esp_err_t nvs_storage_get_pump_config(uint8_t *mode, uint16_t *daily_runtime);

/**
 * @brief Store daily statistics
 * @param date Date in YYYYMMDD format
 * @param runtime_minutes Runtime in minutes
 * @param avg_price Average electricity price
 * @return ESP_OK on success
 */
esp_err_t nvs_storage_set_daily_stats(uint32_t date, uint16_t runtime_minutes, float avg_price);

/**
 * @brief Erase a key from NVS
 * @param key Key to erase
 * @return ESP_OK on success
 */
esp_err_t nvs_storage_erase(const char *key);

/**
 * @brief Save a string value to NVS
 * @param key Key to save
 * @param value String value to save
 * @return ESP_OK on success
 */
esp_err_t nvs_storage_save_string(const char *key, const char *value);

/**
 * @brief Load a string value from NVS
 * @param key Key to load
 * @param buffer Buffer to store the value
 * @param buffer_size Size of the buffer
 * @return ESP_OK on success
 */
esp_err_t nvs_storage_load_string(const char *key, char *buffer, size_t buffer_size);

/**
 * @brief Save an integer value to NVS
 * @param key Key to save
 * @param value Integer value to save
 * @return ESP_OK on success
 */
esp_err_t nvs_storage_save_int(const char *key, int32_t value);

/**
 * @brief Load an integer value from NVS
 * @param key Key to load
 * @param value Pointer to store the value
 * @return ESP_OK on success
 */
esp_err_t nvs_storage_load_int(const char *key, int32_t *value);

#endif // NVS_STORAGE_H
