#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "esp_err.h"
#include "price_fetcher.h"
#include "pump_controller.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Pump mode specifications with flow rate and power consumption
 */
typedef struct {
    uint16_t flow_liters_per_slot; // Liters per 15-minute slot
    uint16_t power_watts;          // Power consumption in watts
} pump_mode_spec_t;

/**
 * @brief Daily pump schedule with mode assignments for each 15-minute slot
 */
typedef struct {
    uint8_t slot_modes[PRICE_INTERVALS_PER_DAY]; // pump_mode_t per slot
    int32_t total_volume_liters;                 // Total water volume for the day
    int32_t total_cost_cents;                    // Estimated cost in SEK cents (SEK * 100)
    bool valid;                                  // Whether schedule was successfully computed
} optimizer_schedule_t;

/**
 * @brief Optimizer configuration parameters
 */
typedef struct {
    int32_t pool_volume_liters; // Pool volume (default 60000)
    uint8_t circulation_factor; // How many times to circulate per day (default 2)
    uint8_t op_start_hour;      // Operating start hour (default 6)
    uint8_t op_end_hour;        // Operating end hour (default 22)
} optimizer_config_t;

/**
 * @brief Get pump mode specifications (flow rate and power for each mode)
 * @return Pointer to static array of pump_mode_spec_t indexed by pump_mode_t
 */
const pump_mode_spec_t *optimizer_get_mode_specs(void);

/**
 * @brief Initialize the optimizer with configuration
 * @param config Pointer to configuration (NULL for defaults)
 * @return ESP_OK on success
 */
esp_err_t optimizer_init(const optimizer_config_t *config);

/**
 * @brief Compute optimal daily pump schedule based on electricity prices
 *
 * Uses a greedy volume-gap filling algorithm:
 * 1. Assign NIGHT mode (most efficient per liter) to all operating slots
 * 2. Upgrade cheapest slots to BACKWASH until volume target is met
 *
 * @param prices Array of 96 price intervals (15-minute slots)
 * @param schedule Output schedule structure
 * @return ESP_OK on success, ESP_ERR_INVALID_ARG if prices invalid
 */
esp_err_t optimizer_compute_daily(const price_interval_t prices[PRICE_INTERVALS_PER_DAY],
                                  optimizer_schedule_t *schedule);

/**
 * @brief Get the pump mode for a specific time slot from a schedule
 * @param schedule Pointer to computed schedule
 * @param slot_index Slot index (0-95)
 * @return pump_mode_t for the slot, or PUMP_MODE_OFF if invalid
 */
pump_mode_t optimizer_get_slot_mode(const optimizer_schedule_t *schedule, int slot_index);

/**
 * @brief Get current optimizer configuration
 * @param config Output configuration structure
 */
void optimizer_get_config(optimizer_config_t *config);

/**
 * @brief Calculate volume target based on configuration
 * @return Target volume in liters (pool_volume * circulation_factor)
 */
int32_t optimizer_get_volume_target(void);

/**
 * @brief Get the currently computed daily schedule from the scheduler
 * @param schedule Output schedule structure
 * @return true if schedule is available, false if not yet computed
 *
 * Note: This function is implemented in pump_scheduler.c, not optimizer.c,
 * because it accesses the scheduler's internal state.
 */
bool pump_scheduler_get_schedule(optimizer_schedule_t *schedule);

#endif // OPTIMIZER_H
