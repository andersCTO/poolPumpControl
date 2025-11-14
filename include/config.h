#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASSWORD_MAX_LEN 64

// Pump Speed Settings (RPM)
#define PUMP_SPEED_NIGHT 1400
#define PUMP_SPEED_DAY 2000
#define PUMP_SPEED_BACKWASH 2900

// Relay Pin Configuration for LilyGO T-Relay
#define RELAY_1_PIN 21
#define RELAY_2_PIN 19
#define RELAY_3_PIN 18
#define RELAY_4_PIN 5

// Digital Input Pins for Inverter Control
#define INVERTER_DI2_PIN RELAY_1_PIN // Night mode (1400 RPM)
#define INVERTER_DI3_PIN RELAY_2_PIN // Day mode (2000 RPM)
#define INVERTER_DI4_PIN RELAY_3_PIN // Backwash mode (2900 RPM)

// Price Fetcher Configuration
#define PRICE_API_URL "https://api.energidataservice.dk/dataset/Elspotprices"
#define PRICE_FETCH_INTERVAL_HOURS 1
#define PRICE_THRESHOLD_LOW 0.10  // EUR/kWh
#define PRICE_THRESHOLD_HIGH 0.30 // EUR/kWh

// Pump Operation Settings
#define MIN_DAILY_RUNTIME_HOURS 4
#define MAX_DAILY_RUNTIME_HOURS 12
#define BACKWASH_DURATION_MINUTES 10

// NVS Storage Keys
#define NVS_NAMESPACE "pool_pump"
#define NVS_KEY_WIFI_SSID "wifi_ssid"
#define NVS_KEY_WIFI_PASS "wifi_pass"
#define NVS_KEY_PUMP_MODE "pump_mode"
#define NVS_KEY_SCHEDULE "schedule"

// Function declarations
void config_init(void);
void pump_scheduler_task(void *pvParameters);

#endif // CONFIG_H
