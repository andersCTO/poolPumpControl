#include "esp_http_server.h"
#include <stdlib.h>
#include <string.h>

#define MAX_URI_HANDLERS 8
static httpd_uri_t registered_uris[MAX_URI_HANDLERS];
static int registered_uri_count = 0;
static char last_response[4096];
static size_t last_response_len = 0;
static char last_content_type[64];
static int server_handle_val = 0;

esp_err_t httpd_start(httpd_handle_t *handle, const httpd_config_t *config) {
    (void)config;
    server_handle_val = 1;
    *handle = (httpd_handle_t)(intptr_t)server_handle_val;
    return ESP_OK;
}

esp_err_t httpd_stop(httpd_handle_t handle) {
    (void)handle;
    server_handle_val = 0;
    return ESP_OK;
}

esp_err_t httpd_register_uri_handler(httpd_handle_t handle, const httpd_uri_t *uri_handler) {
    (void)handle;
    if (registered_uri_count >= MAX_URI_HANDLERS) return ESP_FAIL;
    registered_uris[registered_uri_count] = *uri_handler;
    registered_uri_count++;
    return ESP_OK;
}

esp_err_t httpd_resp_set_type(httpd_req_t *r, const char *type) {
    strncpy(r->content_type, type, sizeof(r->content_type) - 1);
    r->content_type[sizeof(r->content_type) - 1] = '\0';
    strncpy(last_content_type, type, sizeof(last_content_type) - 1);
    last_content_type[sizeof(last_content_type) - 1] = '\0';
    return ESP_OK;
}

esp_err_t httpd_resp_set_status(httpd_req_t *r, const char *status) {
    (void)r;
    (void)status;
    return ESP_OK;
}

esp_err_t httpd_resp_send(httpd_req_t *r, const char *buf, int buf_len) {
    size_t len = buf_len > 0 ? (size_t)buf_len : strlen(buf);
    if (len >= sizeof(last_response)) {
        len = sizeof(last_response) - 1;
    }
    memcpy(last_response, buf, len);
    last_response[len] = '\0';
    last_response_len = len;
    r->response_buf = last_response;
    r->response_len = len;
    return ESP_OK;
}

esp_err_t httpd_resp_send_500(httpd_req_t *r) {
    (void)r;
    return ESP_OK;
}

void mock_httpd_reset(void) {
    registered_uri_count = 0;
    memset(registered_uris, 0, sizeof(registered_uris));
    memset(last_response, 0, sizeof(last_response));
    last_response_len = 0;
    memset(last_content_type, 0, sizeof(last_content_type));
    server_handle_val = 0;
}

int mock_httpd_get_registered_uri_count(void) { return registered_uri_count; }

const httpd_uri_t *mock_httpd_get_registered_uri(int index) {
    if (index < 0 || index >= registered_uri_count) return NULL;
    return &registered_uris[index];
}

const char *mock_httpd_get_last_response(void) { return last_response; }

const char *mock_httpd_get_last_content_type(void) { return last_content_type; }
