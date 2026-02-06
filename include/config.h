#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

// WiFi Configuration
#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASSWORD_MAX_LEN 64

// Pump Speed Settings (RPM) - actual speeds from inverter digital inputs
// These are the fixed speeds when DI2/3/4 are connected to COM (per manual)
#define PUMP_SPEED_NIGHT 1200    // DI4 → COM (Low speed)
#define PUMP_SPEED_DAY 2400      // DI3 → COM (Medium speed)
#define PUMP_SPEED_BACKWASH 2900 // DI2 → COM (High speed)

// LilyGO T-Relay Pin Configuration (https://github.com/Xinyuan-LilyGO/LilyGo-T-Relay)
#define RELAY_1_PIN 21
#define RELAY_2_PIN 19
#define RELAY_3_PIN 18
#define RELAY_4_PIN 5
#define STATUS_LED_PIN 25

// Digital Input Pins for Inverter Control (from RB344 Vario manual Section 5.4)
// When digital input is connected to COM, it triggers a fixed speed:
//   DI2 → COM = 2900 rpm (High/Backwash)
//   DI3 → COM = 2400 rpm (Medium/Day)
//   DI4 → COM = 1200 rpm (Low/Night)
#define INVERTER_DI2_PIN RELAY_1_PIN // High speed (2900 RPM) - Backwash
#define INVERTER_DI3_PIN RELAY_2_PIN // Medium speed (2400 RPM) - Day
#define INVERTER_DI4_PIN RELAY_3_PIN // Low speed (1200 RPM) - Night

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

// Scheduler status
typedef struct {
    bool pump_running;
    int daily_runtime_minutes;
    int current_hour;
} scheduler_status_t;

// Function declarations
void config_init(void);
void pump_scheduler_task(void *pvParameters);
void pump_scheduler_get_status(scheduler_status_t *status);

#endif // CONFIG_H
