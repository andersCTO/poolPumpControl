#include "price_fetcher.h"
#include "cJSON.h"
#include "config.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "PRICE_FETCHER";

#define MAX_HTTP_RESPONSE_SIZE 20480
#define URL_BUFFER_SIZE 128

static price_data_t daily_prices[24];
static price_data_t tomorrow_prices[24];
static price_interval_t interval_prices[PRICE_INTERVALS_PER_DAY];
static float current_price = 0.0f;
static char s_http_response[MAX_HTTP_RESPONSE_SIZE];
static int s_http_response_len = 0;

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

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            s_http_response_len = 0;
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (s_http_response_len + evt->data_len < MAX_HTTP_RESPONSE_SIZE) {
                memcpy(s_http_response + s_http_response_len, evt->data, evt->data_len);
                s_http_response_len += evt->data_len;
                s_http_response[s_http_response_len] = '\0';
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH, total len=%d", s_http_response_len);
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            break;
    }
    return ESP_OK;
}

// Parse JSON response from elprisetjustnu.se
// Format: [{"SEK_per_kWh": 0.75, "time_start": "2024-01-15T00:00:00+01:00", ...}, ...]
// API returns 96 entries (15-minute intervals)
static int parse_price_response(const char *json_str,
                                price_data_t prices[24],
                                price_interval_t intervals[PRICE_INTERVALS_PER_DAY]) {
    int hourly_count = 0;
    int interval_count = 0;

    // Initialize all prices to -1 (invalid)
    for (int i = 0; i < 24; i++) {
        prices[i].hour = i;
        prices[i].price_sek_kwh = -1.0f;
    }
    for (int i = 0; i < PRICE_INTERVALS_PER_DAY; i++) {
        intervals[i].interval = i;
        intervals[i].price_sek_kwh = -1.0f;
    }

    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON response");
        return 0;
    }

    if (!cJSON_IsArray(root)) {
        ESP_LOGE(TAG, "JSON response is not an array");
        cJSON_Delete(root);
        return 0;
    }

    int array_size = cJSON_GetArraySize(root);
    ESP_LOGI(TAG, "Parsing %d price entries", array_size);

    for (int i = 0; i < array_size && i < PRICE_INTERVALS_PER_DAY; i++) {
        cJSON *item = cJSON_GetArrayItem(root, i);
        if (item == NULL) continue;

        cJSON *sek_price = cJSON_GetObjectItem(item, "SEK_per_kWh");
        cJSON *time_start = cJSON_GetObjectItem(item, "time_start");

        if (sek_price != NULL && cJSON_IsNumber(sek_price) && time_start != NULL && cJSON_IsString(time_start)) {
            // Extract hour and minutes from time_start (format: "2024-01-15T14:00:00+01:00")
            const char *time_str = time_start->valuestring;
            int hour = -1;
            int minutes = -1;

            // Find 'T' and parse hour and minutes
            const char *t_pos = strchr(time_str, 'T');
            if (t_pos != NULL && strlen(t_pos) >= 6) {
                hour = (t_pos[1] - '0') * 10 + (t_pos[2] - '0');
                minutes = (t_pos[4] - '0') * 10 + (t_pos[5] - '0');
            }

            if (hour >= 0 && hour < 24 && minutes >= 0 && minutes < 60) {
                float price = (float)sek_price->valuedouble;

                // Store in interval array (96 slots)
                int interval_idx = hour * 4 + (minutes / 15);
                if (interval_idx < PRICE_INTERVALS_PER_DAY && intervals[interval_idx].price_sek_kwh < 0) {
                    intervals[interval_idx].interval = interval_idx;
                    intervals[interval_idx].price_sek_kwh = price;
                    interval_count++;
                }

                // Also store top-of-hour in hourly array
                if (minutes == 0 && prices[hour].price_sek_kwh < 0) {
                    prices[hour].hour = hour;
                    prices[hour].price_sek_kwh = price;
                    hourly_count++;
                    ESP_LOGD(TAG, "Hour %02d: %.2f SEK/kWh", hour, price);
                }
            }
        }
    }

    cJSON_Delete(root);
    ESP_LOGI(TAG, "Parsed %d hourly + %d interval prices", hourly_count, interval_count);
    return hourly_count;
}

esp_err_t price_fetcher_init(void) {
    ESP_LOGI(TAG, "Initializing price fetcher for %s", PRICE_AREA);

    // Create mutex for thread-safe access
    if (s_price_mutex == NULL) {
        s_price_mutex = xSemaphoreCreateMutex();
        if (s_price_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create price mutex");
            return ESP_ERR_NO_MEM;
        }
    }

    // Initialize price arrays
    for (int i = 0; i < 24; i++) {
        daily_prices[i].hour = i;
        daily_prices[i].price_sek_kwh = -1.0f;
        tomorrow_prices[i].hour = i;
        tomorrow_prices[i].price_sek_kwh = -1.0f;
    }
    for (int i = 0; i < PRICE_INTERVALS_PER_DAY; i++) {
        interval_prices[i].interval = i;
        interval_prices[i].price_sek_kwh = -1.0f;
    }

    return ESP_OK;
}

esp_err_t price_fetcher_get_today_prices(price_data_t prices[24]) {
    // Build URL with today's date
    // Format: https://www.elprisetjustnu.se/api/v1/prices/YYYY/MM-DD_SE3.json
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    char url[URL_BUFFER_SIZE];
    snprintf(url,
             sizeof(url),
             "%s/%04d/%02d-%02d_%s.json",
             PRICE_API_BASE_URL,
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             PRICE_AREA);

    ESP_LOGI(TAG, "Fetching prices from: %s", url);

    s_http_response_len = 0;
    memset(s_http_response, 0, sizeof(s_http_response));

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", status_code, s_http_response_len);

        if (status_code == 200 && s_http_response_len > 0) {
            int parsed = parse_price_response(s_http_response, daily_prices, interval_prices);
            if (parsed > 0) {
                if (prices != NULL) {
                    memcpy(prices, daily_prices, sizeof(daily_prices));
                }
            } else {
                err = ESP_FAIL;
            }
        } else {
            ESP_LOGE(TAG, "HTTP error: status=%d", status_code);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

float price_fetcher_get_current_price(void) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    int hour = timeinfo.tm_hour;
    if (hour >= 0 && hour < 24 && daily_prices[hour].price_sek_kwh >= 0) {
        current_price = daily_prices[hour].price_sek_kwh;
    }

    return current_price;
}

int price_fetcher_get_cached_prices(price_data_t prices[24]) {
    int valid_count = 0;

    if (s_price_mutex != NULL && xSemaphoreTake(s_price_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memcpy(prices, daily_prices, sizeof(daily_prices));
        for (int i = 0; i < 24; i++) {
            if (daily_prices[i].price_sek_kwh >= 0) {
                valid_count++;
            }
        }
        xSemaphoreGive(s_price_mutex);
    }

    return valid_count;
}

void price_fetcher_get_stats(price_stats_t *stats) {
    if (stats == NULL) return;

    memset(stats, 0, sizeof(price_stats_t));
    stats->min_price = 999.0f;
    stats->max_price = -1.0f;
    stats->min_hour = -1;
    stats->max_hour = -1;

    float sum = 0.0f;

    if (s_price_mutex != NULL && xSemaphoreTake(s_price_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (int i = 0; i < 24; i++) {
            float price = daily_prices[i].price_sek_kwh;
            if (price >= 0) {
                stats->valid_hours++;
                sum += price;

                if (price < stats->min_price) {
                    stats->min_price = price;
                    stats->min_hour = i;
                }
                if (price > stats->max_price) {
                    stats->max_price = price;
                    stats->max_hour = i;
                }
            }
        }
        xSemaphoreGive(s_price_mutex);
    }

    if (stats->valid_hours > 0) {
        stats->avg_price = sum / stats->valid_hours;
    } else {
        stats->min_price = 0;
        stats->max_price = 0;
    }
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
        s_refresh_state.last_attempt_time = 0;
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
        if (s_refresh_state.cached_day != 0 && s_refresh_state.cached_day != (uint8_t)timeinfo.tm_mday) {
            if (s_refresh_state.tomorrow_available) {
                ESP_LOGI(TAG, "Midnight rollover: moving tomorrow's prices to today");
                memcpy(daily_prices, tomorrow_prices, sizeof(daily_prices));
                for (int i = 0; i < 24; i++) {
                    tomorrow_prices[i].price_sek_kwh = -1.0f;
                }
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

int price_fetcher_get_interval_prices(price_interval_t intervals[PRICE_INTERVALS_PER_DAY]) {
    int valid_count = 0;

    if (s_price_mutex != NULL && xSemaphoreTake(s_price_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        memcpy(intervals, interval_prices, sizeof(interval_prices));
        for (int i = 0; i < PRICE_INTERVALS_PER_DAY; i++) {
            if (interval_prices[i].price_sek_kwh >= 0) {
                valid_count++;
            }
        }
        xSemaphoreGive(s_price_mutex);
    }

    return valid_count;
}

// Helper to compare intervals for sorting by price
static int compare_intervals_by_price(const void *a, const void *b) {
    const price_interval_t *ia = (const price_interval_t *)a;
    const price_interval_t *ib = (const price_interval_t *)b;
    if (ia->price_sek_kwh < ib->price_sek_kwh) return -1;
    if (ia->price_sek_kwh > ib->price_sek_kwh) return 1;
    return 0;
}

void price_fetcher_compute_schedule(schedule_slot_t schedule[PRICE_INTERVALS_PER_DAY], int current_runtime) {
    // Operating hours: 6:00 - 22:00 (interval 24-87)
    const int OP_START_INTERVAL = 6 * 4; // 06:00
    const int OP_END_INTERVAL = 22 * 4;  // 22:00

    // Runtime requirements in 15-minute slots
    int min_slots = (MIN_DAILY_RUNTIME_HOURS * 60) / 15; // 16 slots (4 hours)
    int max_slots = (MAX_DAILY_RUNTIME_HOURS * 60) / 15; // 48 slots (12 hours)
    int current_slots = current_runtime / 15;

    // Initialize schedule
    for (int i = 0; i < PRICE_INTERVALS_PER_DAY; i++) {
        schedule[i].price_sek_kwh = interval_prices[i].price_sek_kwh;
        schedule[i].mode = 0; // PUMP_MODE_OFF
        if (i < OP_START_INTERVAL || i >= OP_END_INTERVAL) {
            schedule[i].status = SCHEDULE_OFF;
        } else {
            schedule[i].status = SCHEDULE_STOPPED;
        }
    }

    // Collect valid operating hour intervals and sort by price
    price_interval_t sortable[PRICE_INTERVALS_PER_DAY];
    int valid_count = 0;
    for (int i = OP_START_INTERVAL; i < OP_END_INTERVAL; i++) {
        if (interval_prices[i].price_sek_kwh >= 0) {
            sortable[valid_count].interval = i;
            sortable[valid_count].price_sek_kwh = interval_prices[i].price_sek_kwh;
            valid_count++;
        }
    }

    if (valid_count == 0) {
        // No valid prices, can't compute schedule
        return;
    }

    // Sort by price (cheapest first)
    qsort(sortable, valid_count, sizeof(price_interval_t), compare_intervals_by_price);

    // Calculate how many more slots we need to run
    int slots_needed = min_slots - current_slots;
    if (slots_needed < 0) slots_needed = 0;

    // Assign required slots (cheapest first)
    for (int i = 0; i < valid_count && slots_needed > 0; i++) {
        int idx = sortable[i].interval;
        schedule[idx].status = SCHEDULE_REQUIRED;
        // Determine mode based on price
        if (sortable[i].price_sek_kwh < PRICE_THRESHOLD_LOW) {
            schedule[idx].mode = 2; // PUMP_MODE_DAY (faster at low prices)
        } else {
            schedule[idx].mode = 2; // PUMP_MODE_DAY default
        }
        slots_needed--;
    }

    // Mark optional slots (low price but beyond minimum)
    int optional_slots = max_slots - min_slots;
    for (int i = 0; i < valid_count && optional_slots > 0; i++) {
        int idx = sortable[i].interval;
        if (schedule[idx].status == SCHEDULE_STOPPED && sortable[i].price_sek_kwh < PRICE_THRESHOLD_LOW) {
            schedule[idx].status = SCHEDULE_OPTIONAL;
            schedule[idx].mode = 2; // PUMP_MODE_DAY
            optional_slots--;
        }
    }
}
