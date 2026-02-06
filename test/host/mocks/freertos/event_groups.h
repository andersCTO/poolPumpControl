#ifndef MOCK_FREERTOS_EVENT_GROUPS_H
#define MOCK_FREERTOS_EVENT_GROUPS_H

#include "FreeRTOS.h"

typedef void *EventGroupHandle_t;
typedef TickType_t EventBits_t;

#define BIT0 (1 << 0)
#define BIT1 (1 << 1)
#define BIT2 (1 << 2)
#define BIT3 (1 << 3)

static inline EventGroupHandle_t xEventGroupCreate(void) { return (EventGroupHandle_t)1; }

static inline EventBits_t xEventGroupSetBits(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToSet) {
    (void)xEventGroup;
    return uxBitsToSet;
}

static inline EventBits_t xEventGroupClearBits(EventGroupHandle_t xEventGroup, const EventBits_t uxBitsToClear) {
    (void)xEventGroup;
    (void)uxBitsToClear;
    return 0;
}

static inline EventBits_t xEventGroupWaitBits(EventGroupHandle_t xEventGroup,
                                              const EventBits_t uxBitsToWaitFor,
                                              const BaseType_t xClearOnExit,
                                              const BaseType_t xWaitForAllBits,
                                              TickType_t xTicksToWait) {
    (void)xEventGroup;
    (void)xClearOnExit;
    (void)xWaitForAllBits;
    (void)xTicksToWait;
    return uxBitsToWaitFor; // Return that the bits are set (simulating connected state)
}

static inline EventBits_t xEventGroupGetBits(EventGroupHandle_t xEventGroup) {
    (void)xEventGroup;
    return 0;
}

#endif // MOCK_FREERTOS_EVENT_GROUPS_H
