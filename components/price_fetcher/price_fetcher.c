#include "price_fetcher.h"
#include "cJSON.h"
#include "config.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "PRICE_FETCHER";

#define MAX_HTTP_OUTPUT_BUFFER 2048

static price_data_t daily_prices[24];
static price_data_t tomorrow_prices[24];
static float current_price = 0.0f;

// State tracking
static price_refresh_state_t s_refresh_state = {
    .status = PRICE_FETCH_STATUS_IDLE,
    .last_fetch_time = 0,
    .last_attempt_time = 0,
    .consecutive_failures = 0,
    .cached_day = 0,
    .tomorrow_available = false,
};

// Mutex for thread-safe access to price data and state
static SemaphoreHandle_t s_price_mutex = NULL;

static esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    static char *output_buffer;
    static int output_len;

    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                if (output_buffer == NULL) {
                    output_buffer = (char *)malloc(esp_http_client_get_content_length(evt->client));
                    output_len = 0;
                    if (output_buffer == NULL) {
                        ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                }
                memcpy(output_buffer + output_len, evt->data, evt->data_len);
                output_len += evt->data_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Parse JSON response
                cJSON *root = cJSON_Parse(output_buffer);
                if (root != NULL) {
                    cJSON *records = cJSON_GetObjectItem(root, "records");
                    if (records != NULL && cJSON_IsArray(records)) {
                        int num_records = cJSON_GetArraySize(records);
                        for (int i = 0; i < num_records && i < 24; i++) {
                            cJSON *record = cJSON_GetArrayItem(records, i);
                            if (record != NULL) {
                                cJSON *price = cJSON_GetObjectItem(record, "SpotPriceEUR");
                                if (price != NULL && cJSON_IsNumber(price)) {
                                    daily_prices[i].price_eur_kwh = price->valuedouble;
                                    daily_prices[i].hour = i;
                                }
                            }
                        }
                    }
                    cJSON_Delete(root);
                }
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
        default:
            ESP_LOGD(TAG, "Unhandled HTTP event: %d", evt->event_id);
            break;
    }
    return ESP_OK;
}

esp_err_t price_fetcher_init(void) {
    ESP_LOGI(TAG, "Initializing price fetcher");

    // Create mutex for thread-safe access
    if (s_price_mutex == NULL) {
        s_price_mutex = xSemaphoreCreateMutex();
        if (s_price_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create price mutex");
            return ESP_ERR_NO_MEM;
        }
    }

    // Initialize price arrays to zero
    memset(daily_prices, 0, sizeof(daily_prices));
    memset(tomorrow_prices, 0, sizeof(tomorrow_prices));

    return ESP_OK;
}

esp_err_t price_fetcher_get_today_prices(price_data_t prices[24]) {
    ESP_LOGI(TAG, "Fetching today's electricity prices");

    esp_http_client_config_t config = {
        .url = PRICE_API_URL,
        .event_handler = _http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG,
                 "HTTP GET Status = %d, content_length = %lld",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
        memcpy(prices, daily_prices, sizeof(daily_prices));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

float price_fetcher_get_current_price(void) {
    // Get current hour
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    int hour = timeinfo.tm_hour;
    if (hour >= 0 && hour < 24) {
        current_price = daily_prices[hour].price_eur_kwh;
    }

    return current_price;
}

bool price_fetcher_is_low_price_period(void) {
    float current = price_fetcher_get_current_price();
    return (current > 0 && current < PRICE_THRESHOLD_LOW);
}

bool price_fetcher_is_data_valid(void) {
    if (s_price_mutex == NULL) {
        return false;
    }

    bool valid = false;
    if (xSemaphoreTake(s_price_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Data is valid if we have a successful fetch within the stale threshold
        if (s_refresh_state.last_fetch_time > 0) {
            time_t now;
            time(&now);
            time_t age_hours = (now - s_refresh_state.last_fetch_time) / 3600;
            valid = (age_hours < PRICE_STALE_THRESHOLD_HOURS);
        }
        xSemaphoreGive(s_price_mutex);
    }
    return valid;
}

time_t price_fetcher_get_last_fetch_time(void) {
    time_t fetch_time = 0;
    if (s_price_mutex != NULL && xSemaphoreTake(s_price_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        fetch_time = s_refresh_state.last_fetch_time;
        xSemaphoreGive(s_price_mutex);
    }
    return fetch_time;
}

void price_fetcher_get_refresh_state(price_refresh_state_t *state) {
    if (state == NULL) {
        return;
    }

    if (s_price_mutex != NULL && xSemaphoreTake(s_price_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memcpy(state, &s_refresh_state, sizeof(price_refresh_state_t));
        xSemaphoreGive(s_price_mutex);
    } else {
        memset(state, 0, sizeof(price_refresh_state_t));
    }
}

esp_err_t price_fetcher_trigger_refresh(void) {
    if (s_price_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(s_price_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        s_refresh_state.status = PRICE_FETCH_STATUS_FETCHING;
        s_refresh_state.last_attempt_time = 0; // Reset to trigger immediate fetch
        xSemaphoreGive(s_price_mutex);
    }

    return ESP_OK;
}

// Internal function to update state after fetch attempt
void price_fetcher_update_state(bool success) {
    if (s_price_mutex == NULL) {
        return;
    }

    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    if (xSemaphoreTake(s_price_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        s_refresh_state.last_attempt_time = now;

        if (success) {
            s_refresh_state.status = PRICE_FETCH_STATUS_SUCCESS;
            s_refresh_state.last_fetch_time = now;
            s_refresh_state.consecutive_failures = 0;
            s_refresh_state.cached_day = (uint8_t)timeinfo.tm_mday;
            ESP_LOGI(TAG, "Price fetch successful, cached_day=%d", s_refresh_state.cached_day);
        } else {
            s_refresh_state.status = PRICE_FETCH_STATUS_FAILED;
            if (s_refresh_state.consecutive_failures < 255) {
                s_refresh_state.consecutive_failures++;
            }
            ESP_LOGW(TAG, "Price fetch failed, consecutive failures: %d", s_refresh_state.consecutive_failures);
        }

        xSemaphoreGive(s_price_mutex);
    }
}

// Handle midnight rollover - move tomorrow's prices to today
void price_fetcher_handle_midnight_rollover(void) {
    if (s_price_mutex == NULL) {
        return;
    }

    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    if (xSemaphoreTake(s_price_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Check if day has changed
        if (s_refresh_state.cached_day != 0 && s_refresh_state.cached_day != (uint8_t)timeinfo.tm_mday) {
            if (s_refresh_state.tomorrow_available) {
                ESP_LOGI(TAG, "Midnight rollover: moving tomorrow's prices to today");
                memcpy(daily_prices, tomorrow_prices, sizeof(daily_prices));
                memset(tomorrow_prices, 0, sizeof(tomorrow_prices));
                s_refresh_state.tomorrow_available = false;
                s_refresh_state.cached_day = (uint8_t)timeinfo.tm_mday;
            } else {
                ESP_LOGW(TAG, "Midnight rollover: no tomorrow prices cached");
                s_refresh_state.cached_day = (uint8_t)timeinfo.tm_mday;
            }
        }
        xSemaphoreGive(s_price_mutex);
    }
}