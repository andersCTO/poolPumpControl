#ifndef MOCK_FREERTOS_H
#define MOCK_FREERTOS_H

#include <stdint.h>

typedef uint32_t TickType_t;
typedef int BaseType_t;
typedef unsigned int UBaseType_t;

#define pdTRUE 1
#define pdFALSE 0
#define pdPASS pdTRUE
#define pdFAIL pdFALSE

#define portMAX_DELAY 0xFFFFFFFFUL

#define pdMS_TO_TICKS(xTimeInMs) ((TickType_t)(xTimeInMs))

#endif // MOCK_FREERTOS_H
