#ifndef MOCK_ESP_HTTP_SERVER_H
#define MOCK_ESP_HTTP_SERVER_H

#include "esp_err.h"
#include <stddef.h>

typedef void *httpd_handle_t;

typedef enum { HTTP_GET = 0, HTTP_POST, HTTP_PUT, HTTP_DELETE } httpd_method_t;

typedef struct httpd_req {
    httpd_handle_t handle;
    int method;
    char uri[128];
    void *user_ctx;
    // For test introspection
    char *response_buf;
    size_t response_len;
    char content_type[64];
} httpd_req_t;

typedef esp_err_t (*httpd_uri_handler_t)(httpd_req_t *r);

typedef struct {
    const char *uri;
    httpd_method_t method;
    httpd_uri_handler_t handler;
    void *user_ctx;
} httpd_uri_t;

typedef struct {
    unsigned server_port;
    size_t stack_size;
} httpd_config_t;

#define HTTPD_DEFAULT_CONFIG()                                                                                         \
    { .server_port = 80, .stack_size = 4096 }

esp_err_t httpd_start(httpd_handle_t *handle, const httpd_config_t *config);
esp_err_t httpd_stop(httpd_handle_t handle);
esp_err_t httpd_register_uri_handler(httpd_handle_t handle, const httpd_uri_t *uri_handler);
esp_err_t httpd_resp_set_type(httpd_req_t *r, const char *type);
esp_err_t httpd_resp_set_status(httpd_req_t *r, const char *status);
esp_err_t httpd_resp_send(httpd_req_t *r, const char *buf, int buf_len);
esp_err_t httpd_resp_send_500(httpd_req_t *r);

// Test helpers
void mock_httpd_reset(void);
int mock_httpd_get_registered_uri_count(void);
const httpd_uri_t *mock_httpd_get_registered_uri(int index);
const char *mock_httpd_get_last_response(void);
const char *mock_httpd_get_last_content_type(void);

#endif // MOCK_ESP_HTTP_SERVER_H
