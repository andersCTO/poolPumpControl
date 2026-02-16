#ifndef MOCK_FREERTOS_TASK_H
#define MOCK_FREERTOS_TASK_H

#include "FreeRTOS.h"

typedef void *TaskHandle_t;

static inline void vTaskDelay(TickType_t xTicksToDelay) { (void)xTicksToDelay; }

static inline void vTaskDelete(TaskHandle_t xTaskToDelete) { (void)xTaskToDelete; }

static inline void vTaskDelayUntil(TickType_t *pxPreviousWakeTime, TickType_t xTimeIncrement) {
    (void)pxPreviousWakeTime;
    (void)xTimeIncrement;
}

static inline TickType_t xTaskGetTickCount(void) { return 0; }

static inline BaseType_t xTaskCreate(void (*pxTaskCode)(void *),
                                     const char *pcName,
                                     uint32_t usStackDepth,
                                     void *pvParameters,
                                     UBaseType_t uxPriority,
                                     TaskHandle_t *pxCreatedTask) {
    (void)pxTaskCode;
    (void)pcName;
    (void)usStackDepth;
    (void)pvParameters;
    (void)uxPriority;
    (void)pxCreatedTask;
    return pdPASS;
}

#endif // MOCK_FREERTOS_TASK_H
