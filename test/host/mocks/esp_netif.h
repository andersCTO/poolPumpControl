#ifndef MOCK_ESP_NETIF_H
#define MOCK_ESP_NETIF_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef void *esp_netif_t;

// IP address structure
typedef struct {
    uint32_t addr;
} esp_ip4_addr_t;

typedef struct {
    esp_ip4_addr_t ip;
    esp_ip4_addr_t netmask;
    esp_ip4_addr_t gw;
} esp_netif_ip_info_t;

typedef struct {
    esp_netif_t *esp_netif;
    esp_netif_ip_info_t ip_info;
    bool ip_changed;
} ip_event_got_ip_t;

// IP address formatting macros
#define IPSTR "%d.%d.%d.%d"
#define IP2STR(ipaddr)                                                                                                 \
    ((ipaddr)->addr & 0xff), (((ipaddr)->addr >> 8) & 0xff), (((ipaddr)->addr >> 16) & 0xff),                          \
        (((ipaddr)->addr >> 24) & 0xff)

static inline esp_err_t esp_netif_init(void) { return ESP_OK; }
static inline esp_netif_t *esp_netif_create_default_wifi_sta(void) { return NULL; }

#endif // MOCK_ESP_NETIF_H
