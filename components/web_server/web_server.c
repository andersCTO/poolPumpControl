#include "web_server.h"
#include "config.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "price_fetcher.h"
#include "pump_controller.h"
#include "wifi_manager.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "WEB_SERVER";
static httpd_handle_t s_server = NULL;

static const char *DASHBOARD_HTML =
    "<!DOCTYPE html>"
    "<html><head>"
    "<meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<meta http-equiv=\"refresh\" content=\"30\">"
    "<title>Pool Pump Controller</title>"
    "<style>"
    "body{font-family:sans-serif;margin:0;padding:16px;background:#f0f4f8;color:#333}"
    ".card{background:#fff;border-radius:8px;padding:16px;margin:8px 0;box-shadow:0 1px 3px rgba(0,0,0,.12)}"
    "h1{margin:0 0 16px;font-size:1.4em;color:#1a365d}"
    ".label{font-size:.85em;color:#718096}"
    ".value{font-size:1.3em;font-weight:bold;margin:2px 0 8px}"
    ".running{color:#38a169}.stopped{color:#e53e3e}"
    ".low{color:#38a169}.high{color:#e53e3e}.mid{color:#d69e2e}"
    ".grid{display:grid;grid-template-columns:1fr 1fr;gap:8px}"
    "</style>"
    "</head><body>"
    "<h1>Pool Pump Controller</h1>"
    "<div class=\"card\">"
    "<div class=\"label\">Pump Status</div>"
    "<div class=\"value %s\">%s</div>"
    "<div class=\"label\">Mode</div>"
    "<div class=\"value\">%s (%d RPM)</div>"
    "</div>"
    "<div class=\"card grid\">"
    "<div><div class=\"label\">Runtime Today</div>"
    "<div class=\"value\">%d min</div></div>"
    "<div><div class=\"label\">Current Hour</div>"
    "<div class=\"value\">%02d:00</div></div>"
    "</div>"
    "<div class=\"card\">"
    "<div class=\"label\">Electricity Price</div>"
    "<div class=\"value %s\">%.3f EUR/kWh</div>"
    "<div class=\"label\">Price Period</div>"
    "<div class=\"value\">%s</div>"
    "</div>"
    "<div class=\"card\">"
    "<div class=\"label\">WiFi</div>"
    "<div class=\"value\">%s</div>"
    "</div>"
    "</body></html>";

static const char *mode_to_string(pump_mode_t mode) {
    switch (mode) {
        case PUMP_MODE_OFF:
            return "OFF";
        case PUMP_MODE_NIGHT:
            return "Night";
        case PUMP_MODE_DAY:
            return "Day";
        case PUMP_MODE_BACKWASH:
            return "Backwash";
        default:
            return "Unknown";
    }
}

static esp_err_t dashboard_get_handler(httpd_req_t *req) {
    pump_status_t pump_status;
    pump_controller_get_status(&pump_status);

    scheduler_status_t sched_status;
    pump_scheduler_get_status(&sched_status);

    float current_price = price_fetcher_get_current_price();
    bool wifi_ok = wifi_manager_is_connected();

    const char *pump_class = sched_status.pump_running ? "running" : "stopped";
    const char *pump_text = sched_status.pump_running ? "RUNNING" : "STOPPED";
    const char *mode_str = mode_to_string(pump_status.mode);

    const char *price_class;
    const char *price_period;
    if (current_price <= 0.0f) {
        price_class = "mid";
        price_period = "No data";
    } else if (current_price < PRICE_THRESHOLD_LOW) {
        price_class = "low";
        price_period = "Low";
    } else if (current_price > PRICE_THRESHOLD_HIGH) {
        price_class = "high";
        price_period = "High";
    } else {
        price_class = "mid";
        price_period = "Medium";
    }

    char buf[2048];
    int len = snprintf(buf,
                       sizeof(buf),
                       DASHBOARD_HTML,
                       pump_class,
                       pump_text,
                       mode_str,
                       pump_status.current_rpm,
                       sched_status.daily_runtime_minutes,
                       sched_status.current_hour >= 0 ? sched_status.current_hour : 0,
                       price_class,
                       current_price,
                       price_period,
                       wifi_ok ? "Connected" : "Disconnected");

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, buf, len);
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

    char buf[512];
    int len = snprintf(buf,
                       sizeof(buf),
                       "{\"pump_running\":%s,"
                       "\"mode\":\"%s\","
                       "\"rpm\":%d,"
                       "\"daily_runtime_minutes\":%d,"
                       "\"current_hour\":%d,"
                       "\"price_eur_kwh\":%.3f,"
                       "\"low_price_period\":%s,"
                       "\"wifi_connected\":%s}",
                       sched_status.pump_running ? "true" : "false",
                       mode_to_string(pump_status.mode),
                       pump_status.current_rpm,
                       sched_status.daily_runtime_minutes,
                       sched_status.current_hour,
                       current_price,
                       is_low ? "true" : "false",
                       wifi_ok ? "true" : "false");

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

esp_err_t web_server_init(void) {
    if (s_server != NULL) {
        ESP_LOGW(TAG, "Web server already running");
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192; // Increase from default 4096 to handle larger responses

    ESP_LOGI(TAG, "Starting web server on port %d", config.server_port);
    esp_err_t ret = httpd_start(&s_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server: %s", esp_err_to_name(ret));
        return ret;
    }

    httpd_register_uri_handler(s_server, &uri_dashboard);
    httpd_register_uri_handler(s_server, &uri_api_status);

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
