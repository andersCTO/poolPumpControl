#include "nvs.h"
#include "nvs_flash.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Simple in-memory key-value store for NVS mock
#define MAX_NVS_ENTRIES 32
#define MAX_KEY_LEN 16
#define MAX_VAL_LEN 128

typedef struct {
    char key[MAX_KEY_LEN];
    char str_val[MAX_VAL_LEN];
    int32_t int_val;
    bool is_string;
    bool in_use;
} nvs_entry_t;

static nvs_entry_t nvs_store[MAX_NVS_ENTRIES];
static bool nvs_initialized = false;
static uint32_t next_handle = 1;

esp_err_t nvs_flash_init(void) {
    nvs_initialized = true;
    return ESP_OK;
}

esp_err_t nvs_flash_erase(void) {
    memset(nvs_store, 0, sizeof(nvs_store));
    return ESP_OK;
}

esp_err_t nvs_open(const char *namespace_name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle) {
    (void)namespace_name;
    (void)open_mode;
    if (!nvs_initialized) return ESP_FAIL;
    *out_handle = next_handle++;
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle) { (void)handle; }

esp_err_t nvs_commit(nvs_handle_t handle) {
    (void)handle;
    return ESP_OK;
}

static nvs_entry_t *find_entry(const char *key) {
    for (int i = 0; i < MAX_NVS_ENTRIES; i++) {
        if (nvs_store[i].in_use && strcmp(nvs_store[i].key, key) == 0) {
            return &nvs_store[i];
        }
    }
    return NULL;
}

static nvs_entry_t *alloc_entry(const char *key) {
    // Check if key already exists
    nvs_entry_t *entry = find_entry(key);
    if (entry) return entry;

    // Find free slot
    for (int i = 0; i < MAX_NVS_ENTRIES; i++) {
        if (!nvs_store[i].in_use) {
            nvs_store[i].in_use = true;
            strncpy(nvs_store[i].key, key, MAX_KEY_LEN - 1);
            nvs_store[i].key[MAX_KEY_LEN - 1] = '\0';
            return &nvs_store[i];
        }
    }
    return NULL;
}

esp_err_t nvs_set_str(nvs_handle_t handle, const char *key, const char *value) {
    (void)handle;
    nvs_entry_t *entry = alloc_entry(key);
    if (!entry) return ESP_ERR_NO_MEM;
    strncpy(entry->str_val, value, MAX_VAL_LEN - 1);
    entry->str_val[MAX_VAL_LEN - 1] = '\0';
    entry->is_string = true;
    return ESP_OK;
}

esp_err_t nvs_get_str(nvs_handle_t handle, const char *key, char *out_value, size_t *length) {
    (void)handle;
    nvs_entry_t *entry = find_entry(key);
    if (!entry || !entry->is_string) return ESP_ERR_NVS_NOT_FOUND;
    size_t slen = strlen(entry->str_val) + 1;
    if (*length < slen) return ESP_ERR_INVALID_SIZE;
    strcpy(out_value, entry->str_val);
    *length = slen;
    return ESP_OK;
}

esp_err_t nvs_set_i32(nvs_handle_t handle, const char *key, int32_t value) {
    (void)handle;
    nvs_entry_t *entry = alloc_entry(key);
    if (!entry) return ESP_ERR_NO_MEM;
    entry->int_val = value;
    entry->is_string = false;
    return ESP_OK;
}

esp_err_t nvs_get_i32(nvs_handle_t handle, const char *key, int32_t *out_value) {
    (void)handle;
    nvs_entry_t *entry = find_entry(key);
    if (!entry || entry->is_string) return ESP_ERR_NVS_NOT_FOUND;
    *out_value = entry->int_val;
    return ESP_OK;
}

esp_err_t nvs_erase_key(nvs_handle_t handle, const char *key) {
    (void)handle;
    nvs_entry_t *entry = find_entry(key);
    if (!entry) return ESP_ERR_NVS_NOT_FOUND;
    memset(entry, 0, sizeof(nvs_entry_t));
    return ESP_OK;
}

void mock_nvs_reset(void) {
    memset(nvs_store, 0, sizeof(nvs_store));
    nvs_initialized = false;
    next_handle = 1;
}
