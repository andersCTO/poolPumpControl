#include "bluetooth_config.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "BT_CONFIG";

// UUIDs for BLE services and characteristics
#define GATTS_SERVICE_UUID_CONFIG 0x00FF
#define GATTS_CHAR_UUID_WIFI_SSID 0xFF01
#define GATTS_CHAR_UUID_WIFI_PASS 0xFF02
#define GATTS_CHAR_UUID_PUMP_SETTINGS 0xFF03
#define GATTS_CHAR_UUID_PRICE_SETTINGS 0xFF04
#define GATTS_CHAR_UUID_SYSTEM_INFO 0xFF05
#define GATTS_CHAR_UUID_NOTIFICATION 0xFF06

#define GATTS_NUM_HANDLE 20

// BLE configuration state
static struct {
    bool initialized;
    bool advertising;
    bool connected;
    uint16_t conn_id;
    uint16_t gatts_if;
    esp_gatt_if_t gatt_if;
    bt_config_callback_t callback;
    char device_name[32];
} bt_config_state = {0};

// Characteristic handles
static uint16_t char_handle_wifi_ssid;
static uint16_t char_handle_wifi_pass;
static uint16_t char_handle_pump_settings;
static uint16_t char_handle_price_settings;
static uint16_t char_handle_system_info;
static uint16_t char_handle_notification;

// BLE advertising data
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 0,
    .p_service_uuid = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

// GAP event handler
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "Advertising data set, start advertising");
            esp_ble_gap_start_advertising(&adv_params);
            bt_config_state.advertising = true;
            break;

        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Advertising start failed");
            } else {
                ESP_LOGI(TAG, "Advertising started successfully");
            }
            break;

        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            ESP_LOGI(TAG, "Advertising stopped");
            bt_config_state.advertising = false;
            break;

        default:
            break;
    }
}

// GATT event handler
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:
            ESP_LOGI(TAG, "GATT server registered, app_id: %d", param->reg.app_id);
            bt_config_state.gatt_if = gatts_if;

            // Set device name
            esp_ble_gap_set_device_name(bt_config_state.device_name);
            esp_ble_gap_config_adv_data(&adv_data);
            break;

        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(TAG, "Device connected, conn_id: %d", param->connect.conn_id);
            bt_config_state.connected = true;
            bt_config_state.conn_id = param->connect.conn_id;
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(TAG, "Device disconnected");
            bt_config_state.connected = false;
            // Restart advertising after disconnect
            esp_ble_gap_start_advertising(&adv_params);
            break;

        case ESP_GATTS_WRITE_EVT:
            ESP_LOGI(TAG, "Write event, handle: %d, len: %d", param->write.handle, param->write.len);

            // Handle WiFi SSID write
            if (param->write.handle == char_handle_wifi_ssid) {
                bt_wifi_credentials_t creds = {0};
                size_t len = param->write.len < sizeof(creds.ssid) ? param->write.len : sizeof(creds.ssid) - 1;
                memcpy(creds.ssid, param->write.value, len);
                ESP_LOGI(TAG, "Received WiFi SSID: %s", creds.ssid);
                if (bt_config_state.callback) {
                    bt_config_state.callback(BT_CONFIG_TYPE_WIFI_CREDENTIALS, &creds, sizeof(creds));
                }
            }
            // Handle WiFi password write
            else if (param->write.handle == char_handle_wifi_pass) {
                bt_wifi_credentials_t creds = {0};
                size_t len = param->write.len < sizeof(creds.password) ? param->write.len : sizeof(creds.password) - 1;
                memcpy(creds.password, param->write.value, len);
                ESP_LOGI(TAG, "Received WiFi password (length: %d)", len);
                if (bt_config_state.callback) {
                    bt_config_state.callback(BT_CONFIG_TYPE_WIFI_CREDENTIALS, &creds, sizeof(creds));
                }
            }

            // Send response
            if (param->write.need_rsp) {
                esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
            }
            break;

        case ESP_GATTS_READ_EVT:
            ESP_LOGI(TAG, "Read event, handle: %d", param->read.handle);
            // Send response with empty data for now
            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
            rsp.attr_value.handle = param->read.handle;
            rsp.attr_value.len = 0;
            esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
            break;

        default:
            break;
    }
}

esp_err_t bluetooth_config_init(const char *device_name, bt_config_callback_t callback) {
    if (bt_config_state.initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing Bluetooth configuration");

    // Store configuration
    strncpy(bt_config_state.device_name, device_name, sizeof(bt_config_state.device_name) - 1);
    bt_config_state.callback = callback;

    // Initialize NVS (required for Bluetooth)
    esp_err_t ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (ret) {
        ESP_LOGW(TAG, "BT controller memory release failed: %s", esp_err_to_name(ret));
    }

    // Initialize Bluetooth controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "Bluetooth controller init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Enable Bluetooth controller in BLE mode
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "Bluetooth controller enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Initialize Bluedroid stack
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register callbacks
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "GAP callback register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "GATTS callback register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register application
    ret = esp_ble_gatts_app_register(0);
    if (ret) {
        ESP_LOGE(TAG, "GATTS app register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    bt_config_state.initialized = true;
    ESP_LOGI(TAG, "Bluetooth configuration initialized successfully");

    return ESP_OK;
}

esp_err_t bluetooth_config_deinit(void) {
    if (!bt_config_state.initialized) {
        return ESP_OK;
    }

    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();

    bt_config_state.initialized = false;
    bt_config_state.connected = false;
    bt_config_state.advertising = false;

    ESP_LOGI(TAG, "Bluetooth configuration deinitialized");

    return ESP_OK;
}

esp_err_t bluetooth_config_start_advertising(void) {
    if (!bt_config_state.initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (bt_config_state.advertising) {
        ESP_LOGW(TAG, "Already advertising");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Starting BLE advertising");
    return esp_ble_gap_start_advertising(&adv_params);
}

esp_err_t bluetooth_config_stop_advertising(void) {
    if (!bt_config_state.advertising) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Stopping BLE advertising");
    return esp_ble_gap_stop_advertising();
}

bool bluetooth_config_is_connected(void) { return bt_config_state.connected; }

esp_err_t bluetooth_config_send_system_info(const bt_system_info_t *info) {
    if (!bt_config_state.connected) {
        return ESP_ERR_INVALID_STATE;
    }

    // TODO: Implement notification sending
    ESP_LOGI(TAG, "Sending system info (not implemented yet)");

    return ESP_OK;
}

esp_err_t bluetooth_config_send_notification(const char *message) {
    if (!bt_config_state.connected) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Notification: %s", message);

    return ESP_OK;
}
