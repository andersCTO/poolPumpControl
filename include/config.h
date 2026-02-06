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
// Using elprisetjustnu.se API for Swedish prices
// URL format: https://www.elprisetjustnu.se/api/v1/prices/YYYY/MM-DD_AREA.json
#define PRICE_API_BASE_URL "https://www.elprisetjustnu.se/api/v1/prices"
#define PRICE_AREA "SE3"
#define PRICE_FETCH_INTERVAL_HOURS 1
#define PRICE_THRESHOLD_LOW 0.50  // SEK/kWh (approx 0.05 EUR)
#define PRICE_THRESHOLD_HIGH 1.50 // SEK/kWh (approx 0.15 EUR)

// Price Refresh Configuration
#define PRICE_STALE_THRESHOLD_HOURS 3    // Data considered stale after this many hours
#define PRICE_TOMORROW_AVAILABLE_HOUR 13 // Hour (CET) when next-day prices become available
#define PRICE_FETCH_RETRY_BASE_SEC 30    // Initial retry delay on failure
#define PRICE_FETCH_RETRY_MAX_MIN 60     // Maximum retry delay in minutes

// Pool Configuration
#define POOL_VOLUME_LITERS 60000
#define POOL_CIRCULATION_FACTOR 2

// Pump Flow Rates (liters per hour)
#define PUMP_FLOW_NIGHT_LPH 5600
#define PUMP_FLOW_DAY_LPH 8000
#define PUMP_FLOW_BACKWASH_LPH 14400

// Pump Power Consumption (watts)
#define PUMP_POWER_NIGHT_W 88
#define PUMP_POWER_DAY_W 353
#define PUMP_POWER_BACKWASH_W 486

// Pump Operation Settings
#define MIN_DAILY_RUNTIME_HOURS 4
#define MAX_DAILY_RUNTIME_HOURS 12
#define BACKWASH_DURATION_MINUTES 10

// Operating hours for pump scheduling
#define PUMP_OP_START_HOUR 6
#define PUMP_OP_END_HOUR 22

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
