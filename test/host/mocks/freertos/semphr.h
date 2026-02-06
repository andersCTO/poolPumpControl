#ifndef MOCK_FREERTOS_SEMPHR_H
#define MOCK_FREERTOS_SEMPHR_H

#include "FreeRTOS.h"

typedef void *SemaphoreHandle_t;

static inline SemaphoreHandle_t xSemaphoreCreateMutex(void) { return (SemaphoreHandle_t)1; }

static inline BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait) {
    (void)xSemaphore;
    (void)xTicksToWait;
    return pdTRUE;
}

static inline BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore) {
    (void)xSemaphore;
    return pdTRUE;
}

static inline void vSemaphoreDelete(SemaphoreHandle_t xSemaphore) { (void)xSemaphore; }

#endif // MOCK_FREERTOS_SEMPHR_H
