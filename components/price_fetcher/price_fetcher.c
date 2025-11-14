#include "price_fetcher.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "config.h"
#include <string.h>

static const char *TAG = "PRICE_FETCHER";

#define MAX_HTTP_OUTPUT_BUFFER 2048

static price_data_t daily_prices[24];
static float current_price = 0.0f;

static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;
    static int output_len;

    switch(evt->event_id) {
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
        case HTTP_EVENT_ON_HEADERS_COMPLETE:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADERS_COMPLETE");
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                if (output_buffer == NULL) {
                    output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
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
    }
    return ESP_OK;
}

esp_err_t price_fetcher_init(void)
{
    ESP_LOGI(TAG, "Initializing price fetcher");
    // Initialize daily prices to zero
    memset(daily_prices, 0, sizeof(daily_prices));
    return ESP_OK;
}

esp_err_t price_fetcher_get_today_prices(price_data_t prices[24])
{
    ESP_LOGI(TAG, "Fetching today's electricity prices");

    esp_http_client_config_t config = {
        .url = PRICE_API_URL,
        .event_handler = _http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
        memcpy(prices, daily_prices, sizeof(daily_prices));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

float price_fetcher_get_current_price(void)
{
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

bool price_fetcher_is_low_price_period(void)
{
    float current = price_fetcher_get_current_price();
    return (current > 0 && current < PRICE_THRESHOLD_LOW);
}