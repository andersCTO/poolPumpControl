#include "web_server.h"
#include "config.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "optimizer.h"
#include "price_fetcher.h"
#include "pump_controller.h"
#include "wifi_manager.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *TAG = "WEB_SERVER";
static httpd_handle_t s_server = NULL;

static const char *DASHBOARD_HTML_START =
    "<!DOCTYPE html>"
    "<html><head>"
    "<meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<meta http-equiv=\"refresh\" content=\"60\">"
    "<title>Pool Pump Controller</title>"
    "<style>"
    "body{font-family:sans-serif;margin:0;padding:16px;background:#f0f4f8;color:#333}"
    ".card{background:#fff;border-radius:8px;padding:16px;margin:8px 0;box-shadow:0 1px 3px rgba(0,0,0,.12)}"
    "h1{margin:0 0 16px;font-size:1.4em;color:#1a365d}"
    "h2{margin:0 0 12px;font-size:1.1em;color:#2d3748}"
    ".label{font-size:.85em;color:#718096}"
    ".value{font-size:1.3em;font-weight:bold;margin:2px 0 8px}"
    ".running{color:#38a169}.stopped{color:#e53e3e}"
    ".low{color:#38a169}.high{color:#e53e3e}.mid{color:#d69e2e}"
    ".grid{display:grid;grid-template-columns:1fr 1fr;gap:8px}"
    ".grid3{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px}"
    ".grid4{display:grid;grid-template-columns:1fr 1fr 1fr 1fr;gap:8px}"
    ".prices{display:grid;grid-template-columns:repeat(6,1fr);gap:4px;margin-top:8px}"
    ".hour{text-align:center;padding:8px 4px;border-radius:4px;font-size:.8em}"
    ".hour .h{font-weight:bold}.hour .p{font-size:.9em}"
    ".hour.low{background:#c6f6d5}.hour.mid{background:#fefcbf}.hour.high{background:#fed7d7}"
    ".hour.now{border:2px solid #3182ce}"
    ".small{font-size:.75em;color:#a0aec0}"
    ".sched{margin-top:8px}"
    ".sched-row{display:flex;align-items:center;margin:2px 0}"
    ".sched-hour{width:32px;font-weight:bold;font-size:.8em}"
    ".sched-slots{display:flex;flex:1;gap:1px}"
    ".slot{flex:1;height:20px;border-radius:2px;position:relative}"
    ".slot.off{background:#e2e8f0}"
    ".slot.night{background:#90cdf4}"
    ".slot.day{background:#faf089}"
    ".slot.backwash{background:#fbd38d}"
    ".slot.now{border:2px solid #3182ce;margin:-2px}"
    ".legend{display:flex;gap:12px;margin-top:8px;font-size:.75em;flex-wrap:wrap}"
    ".legend-item{display:flex;align-items:center;gap:4px}"
    ".legend-box{width:12px;height:12px;border-radius:2px}"
    ".summary{background:#edf2f7;border-radius:4px;padding:12px;margin-top:12px}"
    "</style>"
    "</head><body>"
    "<h1>Pool Pump Controller</h1>";

static const char *DASHBOARD_HTML_END = "</body></html>";

static const char *mode_to_string(pump_mode_t mode) {
    switch (mode) {
        case PUMP_MODE_OFF:
            return "OFF";
        case PUMP_MODE_NIGHT:
            return "Low";
        case PUMP_MODE_DAY:
            return "Medium";
        case PUMP_MODE_BACKWASH:
            return "High";
        default:
            return "Unknown";
    }
}

static const char *fetch_status_to_string(price_fetch_status_t status) {
    switch (status) {
        case PRICE_FETCH_STATUS_IDLE:
            return "idle";
        case PRICE_FETCH_STATUS_FETCHING:
            return "fetching";
        case PRICE_FETCH_STATUS_SUCCESS:
            return "success";
        case PRICE_FETCH_STATUS_FAILED:
            return "failed";
        case PRICE_FETCH_STATUS_NO_WIFI:
            return "no_wifi";
        default:
            return "unknown";
    }
}

static const char *get_price_class(float price, float low_threshold, float high_threshold) {
    if (price < 0) return "mid";
    if (price < low_threshold) return "low";
    if (price > high_threshold) return "high";
    return "mid";
}

static const char *get_mode_slot_class(pump_mode_t mode) {
    switch (mode) {
        case PUMP_MODE_NIGHT:
            return "night";
        case PUMP_MODE_DAY:
            return "day";
        case PUMP_MODE_BACKWASH:
            return "backwash";
        default:
            return "off";
    }
}

static esp_err_t dashboard_get_handler(httpd_req_t *req) {
    pump_status_t pump_status;
    pump_controller_get_status(&pump_status);

    scheduler_status_t sched_status;
    pump_scheduler_get_status(&sched_status);

    float current_price = price_fetcher_get_current_price();
    bool wifi_ok = wifi_manager_is_connected();

    price_stats_t stats;
    price_fetcher_get_stats(&stats);

    price_data_t prices[24];
    int valid_hours = price_fetcher_get_cached_prices(prices);

    price_refresh_state_t refresh_state;
    price_fetcher_get_refresh_state(&refresh_state);

    // Get optimizer schedule
    optimizer_schedule_t opt_schedule;
    bool has_schedule = pump_scheduler_get_schedule(&opt_schedule);

    const char *pump_class = sched_status.pump_running ? "running" : "stopped";
    const char *pump_text = sched_status.pump_running ? "RUNNING" : "STOPPED";
    const char *mode_str = mode_to_string(pump_status.mode);

    // Get current time
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    int current_hour = timeinfo.tm_hour;
    int current_interval = current_hour * 4 + (timeinfo.tm_min / 15);

    // Build response - larger buffer for schedule
    const int BUF_SIZE = 16384;
    char *buf = malloc(BUF_SIZE);
    if (buf == NULL) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    int len = 0;

// Safe snprintf accumulator: clamps len to BUF_SIZE to prevent overflow
#define SNPRINTF_SAFE(fmt, ...)                                                                                        \
    do {                                                                                                               \
        if (len < BUF_SIZE) {                                                                                          \
            int _w = snprintf(buf + len, BUF_SIZE - len, fmt, ##__VA_ARGS__);                                          \
            if (_w > 0) {                                                                                              \
                len += _w;                                                                                             \
                if (len > BUF_SIZE) len = BUF_SIZE;                                                                    \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

    SNPRINTF_SAFE("%s", DASHBOARD_HTML_START);

    // Pump status card
    SNPRINTF_SAFE("<div class=\"card\">"
                  "<div class=\"label\">Pump Status</div>"
                  "<div class=\"value %s\">%s</div>"
                  "<div class=\"label\">Mode</div>"
                  "<div class=\"value\">%s (%d RPM)</div>"
                  "</div>",
                  pump_class,
                  pump_text,
                  mode_str,
                  pump_status.current_rpm);

    // Runtime card
    SNPRINTF_SAFE("<div class=\"card grid\">"
                  "<div><div class=\"label\">Runtime Today</div>"
                  "<div class=\"value\">%d min</div></div>"
                  "<div><div class=\"label\">Current Time</div>"
                  "<div class=\"value\">%02d:%02d</div></div>"
                  "</div>",
                  sched_status.daily_runtime_minutes,
                  current_hour,
                  timeinfo.tm_min);

    // Schedule card - 15-minute intervals visualization
    SNPRINTF_SAFE("<div class=\"card\">"
                  "<h2>Optimized Pump Schedule</h2>");

    // Schedule summary if available
    if (has_schedule && opt_schedule.valid) {
        SNPRINTF_SAFE("<div class=\"summary\">"
                      "<div class=\"grid4\">"
                      "<div><div class=\"label\">Target</div>"
                      "<div class=\"value\">%ld L</div></div>"
                      "<div><div class=\"label\">Planned</div>"
                      "<div class=\"value\">%ld L</div></div>"
                      "<div><div class=\"label\">Est. Cost</div>"
                      "<div class=\"value\">%.2f SEK</div></div>"
                      "<div><div class=\"label\">Slot</div>"
                      "<div class=\"value\">%d/95</div></div>"
                      "</div></div>",
                      (long)optimizer_get_volume_target(),
                      (long)opt_schedule.total_volume_liters,
                      opt_schedule.total_cost_cents / 100.0f,
                      current_interval);
    }

    SNPRINTF_SAFE("<div class=\"sched\">");

    // Render 24 rows (one per hour), each with 4 quarter-hour slots
    for (int h = 0; h < 24; h++) {
        SNPRINTF_SAFE("<div class=\"sched-row\">"
                      "<div class=\"sched-hour\">%02d</div>"
                      "<div class=\"sched-slots\">",
                      h);
        for (int q = 0; q < 4; q++) {
            int idx = h * 4 + q;
            const char *slot_class;

            if (has_schedule && opt_schedule.valid) {
                // Use optimizer schedule with mode colors
                pump_mode_t mode = (pump_mode_t)opt_schedule.slot_modes[idx];
                slot_class = get_mode_slot_class(mode);
            } else {
                // Fallback to old behavior
                slot_class = "off";
            }

            const char *now_class = (idx == current_interval) ? " now" : "";
            SNPRINTF_SAFE("<div class=\"slot %s%s\"></div>", slot_class, now_class);
        }
        SNPRINTF_SAFE("</div></div>");
    }

    // Legend
    SNPRINTF_SAFE("</div>"
                  "<div class=\"legend\">"
                  "<div class=\"legend-item\"><div class=\"legend-box\" style=\"background:#90cdf4\"></div>Low "
                  "(1400 RPM)</div>"
                  "<div class=\"legend-item\"><div class=\"legend-box\" style=\"background:#faf089\"></div>Medium "
                  "(2000 RPM)</div>"
                  "<div class=\"legend-item\"><div class=\"legend-box\" style=\"background:#fbd38d\"></div>High "
                  "(2900 RPM)</div>"
                  "<div class=\"legend-item\"><div class=\"legend-box\" style=\"background:#e2e8f0\"></div>Off</div>"
                  "</div>"
                  "</div>");

    // Price card with area and current price
    const char *price_class = get_price_class(current_price, PRICE_THRESHOLD_LOW, PRICE_THRESHOLD_HIGH);
    SNPRINTF_SAFE("<div class=\"card\">"
                  "<h2>Electricity Price - %s</h2>"
                  "<div class=\"grid\">"
                  "<div><div class=\"label\">Current Price</div>"
                  "<div class=\"value %s\">%.2f SEK/kWh</div></div>"
                  "<div><div class=\"label\">Period</div>"
                  "<div class=\"value\">%s</div></div>"
                  "</div>",
                  PRICE_AREA,
                  price_class,
                  current_price,
                  current_price < 0 ? "No data"
                                    : (current_price < PRICE_THRESHOLD_LOW
                                           ? "Low"
                                           : (current_price > PRICE_THRESHOLD_HIGH ? "High" : "Medium")));

    // Statistics
    if (stats.valid_hours > 0) {
        SNPRINTF_SAFE("<div class=\"grid3\" style=\"margin-top:12px\">"
                      "<div><div class=\"label\">Min (%02d:00)</div>"
                      "<div class=\"value low\">%.2f</div></div>"
                      "<div><div class=\"label\">Avg</div>"
                      "<div class=\"value mid\">%.2f</div></div>"
                      "<div><div class=\"label\">Max (%02d:00)</div>"
                      "<div class=\"value high\">%.2f</div></div>"
                      "</div>",
                      stats.min_hour,
                      stats.min_price,
                      stats.avg_price,
                      stats.max_hour,
                      stats.max_price);
    }

    // 24-hour price grid
    SNPRINTF_SAFE("<div class=\"prices\">");
    for (int h = 0; h < 24; h++) {
        const char *hour_class = get_price_class(prices[h].price_sek_kwh, PRICE_THRESHOLD_LOW, PRICE_THRESHOLD_HIGH);
        const char *now_class = (h == current_hour) ? " now" : "";
        if (prices[h].price_sek_kwh >= 0) {
            SNPRINTF_SAFE("<div class=\"hour %s%s\"><div class=\"h\">%02d</div><div class=\"p\">%.2f</div></div>",
                          hour_class,
                          now_class,
                          h,
                          prices[h].price_sek_kwh);
        } else {
            SNPRINTF_SAFE(
                "<div class=\"hour mid%s\"><div class=\"h\">%02d</div><div class=\"p\">-</div></div>", now_class, h);
        }
    }
    SNPRINTF_SAFE("</div>");

    // Last update
    if (refresh_state.last_fetch_time > 0) {
        struct tm fetch_time;
        localtime_r(&refresh_state.last_fetch_time, &fetch_time);
        SNPRINTF_SAFE("<div class=\"small\" style=\"margin-top:8px\">Updated: %02d:%02d (%d hours)</div>",
                      fetch_time.tm_hour,
                      fetch_time.tm_min,
                      valid_hours);
    }

    SNPRINTF_SAFE("</div>"); // Close price card

    // WiFi status
    SNPRINTF_SAFE("<div class=\"card\">"
                  "<div class=\"label\">WiFi</div>"
                  "<div class=\"value\">%s</div>"
                  "</div>",
                  wifi_ok ? "Connected" : "Disconnected");

    SNPRINTF_SAFE("%s", DASHBOARD_HTML_END);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, buf, len);
    free(buf);

#undef SNPRINTF_SAFE

    return ESP_OK;
}

static esp_err_t favicon_get_handler(httpd_req_t *req) {
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t api_status_get_handler(httpd_req_t *req) {
    pump_status_t pump_status;
    pump_controller_get_status(&pump_status);

    scheduler_status_t sched_status;
    pump_scheduler_get_status(&sched_status);

    float current_price = price_fetcher_get_current_price();
    bool is_low = price_fetcher_is_low_price_period();
    bool wifi_ok = wifi_manager_is_connected();

    price_refresh_state_t refresh_state;
    price_fetcher_get_refresh_state(&refresh_state);
    bool price_valid = price_fetcher_is_data_valid();

    price_stats_t stats;
    price_fetcher_get_stats(&stats);

    // Get optimizer schedule
    optimizer_schedule_t opt_schedule;
    bool has_schedule = pump_scheduler_get_schedule(&opt_schedule);

    char buf[1536];
    int len = snprintf(buf,
                       sizeof(buf),
                       "{\"pump_running\":%s,"
                       "\"mode\":\"%s\","
                       "\"rpm\":%d,"
                       "\"daily_runtime_minutes\":%d,"
                       "\"current_hour\":%d,"
                       "\"price_area\":\"%s\","
                       "\"price_sek_kwh\":%.3f,"
                       "\"price_min\":%.3f,"
                       "\"price_max\":%.3f,"
                       "\"price_avg\":%.3f,"
                       "\"low_price_period\":%s,"
                       "\"wifi_connected\":%s,"
                       "\"price_last_fetch\":%lld,"
                       "\"price_fetch_status\":\"%s\","
                       "\"price_data_valid\":%s,"
                       "\"schedule_valid\":%s,"
                       "\"schedule_volume_target\":%ld,"
                       "\"schedule_volume_planned\":%ld,"
                       "\"schedule_cost_cents\":%ld}",
                       sched_status.pump_running ? "true" : "false",
                       mode_to_string(pump_status.mode),
                       pump_status.current_rpm,
                       sched_status.daily_runtime_minutes,
                       sched_status.current_hour,
                       PRICE_AREA,
                       current_price,
                       stats.min_price,
                       stats.max_price,
                       stats.avg_price,
                       is_low ? "true" : "false",
                       wifi_ok ? "true" : "false",
                       (long long)refresh_state.last_fetch_time,
                       fetch_status_to_string(refresh_state.status),
                       price_valid ? "true" : "false",
                       has_schedule && opt_schedule.valid ? "true" : "false",
                       (long)optimizer_get_volume_target(),
                       has_schedule ? (long)opt_schedule.total_volume_liters : 0L,
                       has_schedule ? (long)opt_schedule.total_cost_cents : 0L);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buf, len);
    return ESP_OK;
}

static const httpd_uri_t uri_dashboard = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = dashboard_get_handler,
};

static const httpd_uri_t uri_api_status = {
    .uri = "/api/status",
    .method = HTTP_GET,
    .handler = api_status_get_handler,
};

static const httpd_uri_t uri_favicon = {
    .uri = "/favicon.ico",
    .method = HTTP_GET,
    .handler = favicon_get_handler,
};

esp_err_t web_server_init(void) {
    if (s_server != NULL) {
        ESP_LOGW(TAG, "Web server already running");
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;

    ESP_LOGI(TAG, "Starting web server on port %d", config.server_port);
    esp_err_t ret = httpd_start(&s_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server: %s", esp_err_to_name(ret));
        return ret;
    }

    httpd_register_uri_handler(s_server, &uri_dashboard);
    httpd_register_uri_handler(s_server, &uri_api_status);
    httpd_register_uri_handler(s_server, &uri_favicon);

    ESP_LOGI(TAG, "Web server started successfully");
    return ESP_OK;
}

esp_err_t web_server_stop(void) {
    if (s_server == NULL) {
        return ESP_OK;
    }

    esp_err_t ret = httpd_stop(s_server);
    if (ret == ESP_OK) {
        s_server = NULL;
        ESP_LOGI(TAG, "Web server stopped");
    }
    return ret;
}
