#ifndef FREERTOS_H
#define FREERTOS_H
#include <stddef.h>
#include <stdint.h>
#include "FreeRTOSConfig.h"
#include "projdefs.h"
typedef long BaseType_t;
typedef unsigned long UBaseType_t;
typedef uint32_t TickType_t;
#define portMAX_DELAY ((TickType_t)0xffffffffUL)
#define pdMS_TO_TICKS(xTimeInMs) ((TickType_t)(xTimeInMs))
#endif
