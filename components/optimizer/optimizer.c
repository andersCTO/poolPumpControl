#include "optimizer.h"
#include "config.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "OPTIMIZER";

// Pump mode specifications (indexed by pump_mode_t)
// Flow in liters per 15-minute slot = LPH / 4
static const pump_mode_spec_t s_mode_specs[] = {
    [PUMP_MODE_OFF] = {0, 0},
    [PUMP_MODE_NIGHT] = {PUMP_FLOW_NIGHT_LPH / 4, PUMP_POWER_NIGHT_W},
    [PUMP_MODE_DAY] = {PUMP_FLOW_DAY_LPH / 4, PUMP_POWER_DAY_W},
    [PUMP_MODE_BACKWASH] = {PUMP_FLOW_BACKWASH_LPH / 4, PUMP_POWER_BACKWASH_W},
};

// Current configuration
static optimizer_config_t s_config = {
    .pool_volume_liters = POOL_VOLUME_LITERS,
    .circulation_factor = POOL_CIRCULATION_FACTOR,
    .op_start_hour = PUMP_OP_START_HOUR,
    .op_end_hour = PUMP_OP_END_HOUR,
};

// Slot info for sorting
typedef struct {
    uint8_t index;        // Original slot index (0-95)
    float price;          // Price in SEK/kWh
    bool within_op_hours; // Whether slot is within operating hours
} slot_info_t;

// Compare slots by price (ascending) for qsort
static int compare_slots_by_price(const void *a, const void *b) {
    const slot_info_t *sa = (const slot_info_t *)a;
    const slot_info_t *sb = (const slot_info_t *)b;

    // Invalid prices (negative) go to the end
    if (sa->price < 0 && sb->price >= 0) return 1;
    if (sa->price >= 0 && sb->price < 0) return -1;
    if (sa->price < 0 && sb->price < 0) return 0;

    if (sa->price < sb->price) return -1;
    if (sa->price > sb->price) return 1;
    return 0;
}

const pump_mode_spec_t *optimizer_get_mode_specs(void) { return s_mode_specs; }

esp_err_t optimizer_init(const optimizer_config_t *config) {
    if (config != NULL) {
        s_config = *config;
    }

    ESP_LOGI(TAG,
             "Optimizer initialized: pool=%ld L, circ=%dx, hours=%d-%d",
             (long)s_config.pool_volume_liters,
             s_config.circulation_factor,
             s_config.op_start_hour,
             s_config.op_end_hour);

    return ESP_OK;
}

void optimizer_get_config(optimizer_config_t *config) {
    if (config != NULL) {
        *config = s_config;
    }
}

int32_t optimizer_get_volume_target(void) { return s_config.pool_volume_liters * s_config.circulation_factor; }

esp_err_t optimizer_compute_daily(const price_interval_t prices[PRICE_INTERVALS_PER_DAY],
                                  optimizer_schedule_t *schedule) {
    if (prices == NULL || schedule == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize schedule
    memset(schedule, 0, sizeof(optimizer_schedule_t));
    for (int i = 0; i < PRICE_INTERVALS_PER_DAY; i++) {
        schedule->slot_modes[i] = PUMP_MODE_OFF;
    }

    // Operating hours boundaries
    int op_start_slot = s_config.op_start_hour * 4;
    int op_end_slot = s_config.op_end_hour * 4;
    int num_op_slots = op_end_slot - op_start_slot; // 64 slots for 6-22

    // Volume target
    int32_t volume_target = optimizer_get_volume_target();

    // Collect slots within operating hours with valid prices
    slot_info_t *slots = malloc(num_op_slots * sizeof(slot_info_t));
    if (slots == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for slot sorting");
        return ESP_ERR_NO_MEM;
    }

    int valid_slot_count = 0;
    for (int i = op_start_slot; i < op_end_slot; i++) {
        slots[i - op_start_slot].index = i;
        slots[i - op_start_slot].price = prices[i].price_sek_kwh;
        slots[i - op_start_slot].within_op_hours = true;
        if (prices[i].price_sek_kwh >= 0) {
            valid_slot_count++;
        }
    }

    if (valid_slot_count == 0) {
        ESP_LOGW(TAG, "No valid prices available, using fallback schedule");
        // Fallback: run LOW mode during typical cheap hours (00-06 excluded, so start of op hours)
        for (int i = op_start_slot; i < op_start_slot + 32 && i < op_end_slot; i++) {
            schedule->slot_modes[i] = PUMP_MODE_NIGHT;
        }
        schedule->total_volume_liters = 32 * s_mode_specs[PUMP_MODE_NIGHT].flow_liters_per_slot;
        schedule->valid = false;
        free(slots);
        return ESP_OK;
    }

    // Sort slots by price (cheapest first)
    qsort(slots, num_op_slots, sizeof(slot_info_t), compare_slots_by_price);

    // Flow per slot for each mode
    int32_t low_volume_per_slot = s_mode_specs[PUMP_MODE_NIGHT].flow_liters_per_slot;
    int32_t high_volume_per_slot = s_mode_specs[PUMP_MODE_BACKWASH].flow_liters_per_slot;

    // Maximum volume if we run LOW for all available slots
    int32_t max_low_volume = valid_slot_count * low_volume_per_slot;

    ESP_LOGI(TAG,
             "Target: %ld L, Max LOW capacity: %ld L (%d slots × %ld L)",
             (long)volume_target,
             (long)max_low_volume,
             valid_slot_count,
             (long)low_volume_per_slot);

    if (max_low_volume >= volume_target) {
        // Case 1: LOW mode alone can meet or exceed target
        // Only use as many LOW slots as needed (cheapest first)
        int slots_needed = (volume_target + low_volume_per_slot - 1) / low_volume_per_slot; // Ceiling division
        ESP_LOGI(TAG, "LOW mode sufficient: need %d slots to meet target", slots_needed);

        int slots_assigned = 0;
        for (int i = 0; i < num_op_slots && slots_assigned < slots_needed; i++) {
            // Skip slots with invalid prices
            if (slots[i].price < 0) continue;

            int slot_idx = slots[i].index;
            schedule->slot_modes[slot_idx] = PUMP_MODE_NIGHT;
            slots_assigned++;
        }
    } else {
        // Case 2: Need HIGH mode to meet target
        // First, assign LOW to all valid slots
        for (int i = 0; i < num_op_slots; i++) {
            if (slots[i].price < 0) continue;
            int slot_idx = slots[i].index;
            schedule->slot_modes[slot_idx] = PUMP_MODE_NIGHT;
        }

        // Calculate shortfall and upgrade cheapest slots to HIGH
        int32_t shortfall = volume_target - max_low_volume;
        int32_t extra_volume_per_upgrade = high_volume_per_slot - low_volume_per_slot;
        int upgrades_needed = (shortfall + extra_volume_per_upgrade - 1) / extra_volume_per_upgrade; // Ceiling division

        ESP_LOGI(TAG, "Shortfall: %ld L, need %d HIGH upgrades", (long)shortfall, upgrades_needed);

        // Upgrade the cheapest slots from LOW to HIGH
        int upgrades_done = 0;
        for (int i = 0; i < num_op_slots && upgrades_done < upgrades_needed; i++) {
            if (slots[i].price < 0) continue;

            int slot_idx = slots[i].index;
            schedule->slot_modes[slot_idx] = PUMP_MODE_BACKWASH;
            upgrades_done++;
        }
    }

    // Calculate total volume and cost
    int32_t total_volume = 0;
    float total_cost = 0.0f;

    for (int i = 0; i < PRICE_INTERVALS_PER_DAY; i++) {
        pump_mode_t mode = (pump_mode_t)schedule->slot_modes[i];
        if (mode != PUMP_MODE_OFF) {
            total_volume += s_mode_specs[mode].flow_liters_per_slot;

            // Cost = power (kW) * time (0.25h) * total_price (SEK/kWh)
            // Total price = spot price + additional costs (grid fees, taxes)
            float spot_price = prices[i].price_sek_kwh;
            if (spot_price < 0) spot_price = 0; // Treat invalid as free for calculation
            float total_price = spot_price + PRICE_ADDITIONAL_COST_SEK;
            float power_kw = s_mode_specs[mode].power_watts / 1000.0f;
            float cost_sek = power_kw * 0.25f * total_price;
            total_cost += cost_sek;
        }
    }

    schedule->total_volume_liters = total_volume;
    schedule->total_cost_cents = (int32_t)(total_cost * 100.0f);
    schedule->valid = true;

    // Log summary
    int night_count = 0, backwash_count = 0, day_count = 0;
    for (int i = 0; i < PRICE_INTERVALS_PER_DAY; i++) {
        switch (schedule->slot_modes[i]) {
            case PUMP_MODE_NIGHT:
                night_count++;
                break;
            case PUMP_MODE_DAY:
                day_count++;
                break;
            case PUMP_MODE_BACKWASH:
                backwash_count++;
                break;
            default:
                break;
        }
    }

    ESP_LOGI(TAG, "Schedule computed: %d LOW + %d MEDIUM + %d HIGH slots", night_count, day_count, backwash_count);
    ESP_LOGI(TAG, "Total volume: %ld L, Estimated cost: %.2f SEK", (long)total_volume, total_cost);

    free(slots);
    return ESP_OK;
}

pump_mode_t optimizer_get_slot_mode(const optimizer_schedule_t *schedule, int slot_index) {
    if (schedule == NULL || slot_index < 0 || slot_index >= PRICE_INTERVALS_PER_DAY) {
        return PUMP_MODE_OFF;
    }
    return (pump_mode_t)schedule->slot_modes[slot_index];
}
