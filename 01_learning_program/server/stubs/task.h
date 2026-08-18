#ifndef TASK_H
#define TASK_H
#include "FreeRTOS.h"
typedef void *TaskHandle_t;
BaseType_t xTaskCreate(void (*pxTaskCode)(void *), const char *pcName,
                       UBaseType_t usStackDepth, void *pvParameters,
                       UBaseType_t uxPriority, TaskHandle_t *pxCreatedTask);
void vTaskDelay(TickType_t xTicksToDelay);
void vTaskDelayUntil(TickType_t *pxPreviousWakeTime, TickType_t xTimeIncrement);
void vTaskSuspend(TaskHandle_t xTaskToSuspend);
void vTaskResume(TaskHandle_t xTaskToResume);
void vTaskSuspendAll(void);
BaseType_t xTaskResumeAll(void);
void vTaskStartScheduler(void);
TickType_t xTaskGetTickCount(void);
uint32_t ulTaskNotifyTake(BaseType_t xClearCountOnExit, TickType_t xTicksToWait);
void vTaskNotifyGiveFromISR(TaskHandle_t xTaskToNotify, BaseType_t *pxHigherPriorityTaskWoken);
#define taskENTER_CRITICAL() do { } while (0)
#define taskEXIT_CRITICAL() do { } while (0)
#endif
