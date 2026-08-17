#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""make_stubs.py — gcc -fsyntax-only 컴파일 체크용 스텁 헤더 생성.
실제 STM32CubeF4 HAL / FreeRTOS 헤더 없이도, 이 프로젝트의 코드가
문법·타입적으로 올바른지 gcc 로 검사할 수 있게 한다. (기능 에뮬레이션 아님)"""
import os

OUT = '/home/user/stubs'
os.makedirs(OUT, exist_ok=True)

F = {}

F['FreeRTOSConfig.h'] = r'''#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H
#define configMAX_PRIORITIES 8
#endif
'''

F['projdefs.h'] = r'''#ifndef PROJDEFS_H
#define PROJDEFS_H
#define pdFALSE ((BaseType_t)0)
#define pdTRUE  ((BaseType_t)1)
#define pdPASS  pdTRUE
#define pdFAIL  pdFALSE
#ifndef configASSERT
#define configASSERT(x) do { if (!(x)) { for(;;){} } } while (0)
#endif
#define portYIELD_FROM_ISR(x) do { (void)(x); } while (0)
#define taskDISABLE_INTERRUPTS() do { } while (0)
#endif
'''

F['FreeRTOS.h'] = r'''#ifndef FREERTOS_H
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
'''

F['task.h'] = r'''#ifndef TASK_H
#define TASK_H
#include "FreeRTOS.h"
typedef void *TaskHandle_t;
BaseType_t xTaskCreate(void (*pxTaskCode)(void *), const char *pcName,
                       UBaseType_t usStackDepth, void *pvParameters,
                       UBaseType_t uxPriority, TaskHandle_t *pxCreatedTask);
void vTaskDelay(TickType_t xTicksToDelay);
void vTaskDelayUntil(TickType_t *pxPreviousWakeTime, TickType_t xTimeIncrement);
void vTaskSuspend(TaskHandle_t xTaskToSuspend);
void vTaskSuspendAll(void);
BaseType_t xTaskResumeAll(void);
void vTaskStartScheduler(void);
TickType_t xTaskGetTickCount(void);
uint32_t ulTaskNotifyTake(BaseType_t xClearCountOnExit, TickType_t xTicksToWait);
void vTaskNotifyGiveFromISR(TaskHandle_t xTaskToNotify, BaseType_t *pxHigherPriorityTaskWoken);
#define taskENTER_CRITICAL() do { } while (0)
#define taskEXIT_CRITICAL() do { } while (0)
#endif
'''

F['queue.h'] = r'''#ifndef QUEUE_H
#define QUEUE_H
#include "FreeRTOS.h"
typedef void *QueueHandle_t;
QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize);
BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait);
BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait);
#endif
'''

F['semphr.h'] = r'''#ifndef SEMPHR_H
#define SEMPHR_H
#include "queue.h"
typedef QueueHandle_t SemaphoreHandle_t;
SemaphoreHandle_t xSemaphoreCreateMutex(void);
SemaphoreHandle_t xSemaphoreCreateBinary(void);
BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait);
BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore);
BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken);
#endif
'''

F['stm32f4xx_hal.h'] = r'''#ifndef STM32F4XX_HAL_H
#define STM32F4XX_HAL_H
#include <stddef.h>
#include <stdint.h>

typedef enum { HAL_OK = 0x00U, HAL_ERROR = 0x01U, HAL_BUSY = 0x02U, HAL_TIMEOUT = 0x03U } HAL_StatusTypeDef;

#define DISABLE 0x00000000U
#define ENABLE  0x00000001U

/* ---- generic registers ---- */
typedef struct { volatile uint32_t CSR; } RCC_TypeDef;
#define RCC ((RCC_TypeDef *)0x40023800UL)

#define HAL_MAX_DELAY 0xFFFFFFFFU

/* ---- NVIC ---- */
typedef enum IRQn { CAN1_RX0_IRQn = 20 } IRQn_Type;
#define NVIC_PRIORITYGROUP_0 0x00000007U
#define NVIC_PRIORITYGROUP_1 0x00000006U
#define NVIC_PRIORITYGROUP_2 0x00000005U
#define NVIC_PRIORITYGROUP_3 0x00000004U
#define NVIC_PRIORITYGROUP_4 0x00000003U
void NVIC_SystemReset(void);
void HAL_NVIC_SetPriority(IRQn_Type IRQn, uint32_t PreemptPriority, uint32_t SubPriority);
void HAL_NVIC_EnableIRQ(IRQn_Type IRQn);
void HAL_NVIC_SetPriorityGrouping(uint32_t PriorityGroup);

/* ---- GPIO ---- */
typedef struct { } GPIO_TypeDef;
typedef struct { uint32_t Pin; uint32_t Mode; uint32_t Pull; uint32_t Speed; uint32_t Alternate; } GPIO_InitTypeDef;
typedef enum { GPIO_PIN_RESET = 0u, GPIO_PIN_SET } GPIO_PinState;
#define GPIO_PIN_0  ((uint16_t)0x0001)
#define GPIO_PIN_1  ((uint16_t)0x0002)
#define GPIO_PIN_2  ((uint16_t)0x0004)
#define GPIO_PIN_3  ((uint16_t)0x0008)
#define GPIO_PIN_4  ((uint16_t)0x0010)
#define GPIO_PIN_5  ((uint16_t)0x0020)
#define GPIO_PIN_6  ((uint16_t)0x0040)
#define GPIO_PIN_7  ((uint16_t)0x0080)
#define GPIO_PIN_8  ((uint16_t)0x0100)
#define GPIO_PIN_9  ((uint16_t)0x0200)
#define GPIO_PIN_10 ((uint16_t)0x0400)
#define GPIO_PIN_11 ((uint16_t)0x0800)
#define GPIO_PIN_12 ((uint16_t)0x1000)
#define GPIO_PIN_13 ((uint16_t)0x2000)
#define GPIO_PIN_14 ((uint16_t)0x4000)
#define GPIO_PIN_15 ((uint16_t)0x8000)
#define GPIO_MODE_OUTPUT_PP 0x00000001U
#define GPIO_MODE_AF_PP     0x00000002U
#define GPIO_MODE_ANALOG    0x00000003U
#define GPIO_MODE_AF_OD     0x00000004U
#define GPIO_NOPULL   0x00000000U
#define GPIO_PULLUP   0x00000001U
#define GPIO_PULLDOWN 0x00000002U
#define GPIO_SPEED_FREQ_LOW        0x00000000U
#define GPIO_SPEED_FREQ_MEDIUM     0x00000001U
#define GPIO_SPEED_FREQ_HIGH       0x00000002U
#define GPIO_SPEED_FREQ_VERY_HIGH  0x00000003U
#define GPIO_AF0_SYSTEM   0x0U
#define GPIO_AF1_TIM1     0x1U
#define GPIO_AF4_I2C1     0x4U
#define GPIO_AF7_USART2   0x7U
#define GPIO_AF9_CAN1     0x9U
#define GPIOA ((GPIO_TypeDef *)0x40020000UL)
#define GPIOB ((GPIO_TypeDef *)0x40020400UL)
#define GPIOC ((GPIO_TypeDef *)0x40020800UL)
#define GPIOD ((GPIO_TypeDef *)0x40020C00UL)
void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init);
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

/* ---- RCC 클록 활성화 매크로 ---- */
#define __HAL_RCC_GPIOA_CLK_ENABLE()   do { } while (0)
#define __HAL_RCC_GPIOB_CLK_ENABLE()   do { } while (0)
#define __HAL_RCC_GPIOC_CLK_ENABLE()   do { } while (0)
#define __HAL_RCC_GPIOD_CLK_ENABLE()   do { } while (0)
#define __HAL_RCC_CAN1_CLK_ENABLE()    do { } while (0)
#define __HAL_RCC_ADC1_CLK_ENABLE()    do { } while (0)
#define __HAL_RCC_TIM1_CLK_ENABLE()    do { } while (0)
#define __HAL_RCC_USART2_CLK_ENABLE()  do { } while (0)
#define __HAL_RCC_I2C1_CLK_ENABLE()    do { } while (0)
#define __HAL_RCC_BKPSRAM_CLK_ENABLE() do { } while (0)
#define __HAL_RCC_PWR_CLK_ENABLE()     do { } while (0)

/* ---- PWR ---- */
#define PWR_REGULATOR_VOLTAGE_SCALE1 1U
#define __HAL_PWR_VOLTAGESCALING_CONFIG(x) do { } while (0)
void HAL_PWR_EnableBkUpAccess(void);

/* ---- CAN ---- */
typedef struct { } CAN_TypeDef;
typedef struct {
    uint32_t Prescaler, Mode, SyncJumpWidth, TimeSeg1, TimeSeg2;
    uint32_t TimeTriggeredMode, AutoBusOff, AutoWakeUp, AutoRetransmission;
    uint32_t ReceiveFifoLocked, TransmitFifoPriority;
} CAN_InitTypeDef;
typedef struct { CAN_TypeDef *Instance; CAN_InitTypeDef Init; } CAN_HandleTypeDef;
typedef struct {
    uint32_t FilterIdHigh, FilterIdLow, FilterMaskIdHigh, FilterMaskIdLow;
    uint32_t FilterFIFOAssignment, FilterBank, FilterMode, FilterScale;
    uint32_t FilterActivation, SlaveStartFilterBank;
} CAN_FilterTypeDef;
typedef struct {
    uint32_t StdId, ExtId, IDE, RTR, DLC, Timestamp, FilterMatchIndex;
} CAN_RxHeaderTypeDef;
#define CAN_MODE_NORMAL 0x00000000U
#define CAN_SJW_1TQ 0x00000000U
#define CAN_BS1_10TQ 0x00000009U
#define CAN_BS1_13TQ 0x0000000CU
#define CAN_BS2_2TQ  0x00000001U
#define CAN_BS2_4TQ  0x00000003U
#define CAN_FILTERMODE_IDMASK 0x00000000U
#define CAN_FILTERSCALE_32BIT 0x00000001U
#define CAN_RX_FIFO0 0x00000000U
#define CAN_ID_STD 0x00000000U
#define CAN_ID_EXT 0x00000004U
#define CAN_IT_RX_FIFO0_MSG_PENDING 0x00000002U
HAL_StatusTypeDef HAL_CAN_Init(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef HAL_CAN_ActivateNotification(CAN_HandleTypeDef *hcan, uint32_t ActiveITs);
HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *hcan, CAN_FilterTypeDef *sFilterConfig);
HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef *hcan, uint32_t RxFifo, CAN_RxHeaderTypeDef *pRxHeader, uint8_t aData[]);

/* ---- ADC ---- */
typedef struct { } ADC_TypeDef;
typedef struct {
    uint32_t ClockPrescaler, Resolution, ScanConvMode, ContinuousConvMode;
    uint32_t DiscontinuousConvMode, ExternalTrigConvEdge, ExternalTrigConv;
    uint32_t DataAlign, NbrOfConversion, DMAContinuousRequests, EOCSelection;
} ADC_InitTypeDef;
typedef struct { ADC_TypeDef *Instance; ADC_InitTypeDef Init; } ADC_HandleTypeDef;
typedef struct { uint32_t Channel, Rank, SamplingTime; } ADC_ChannelConfTypeDef;
#define ADC_CLOCK_SYNC_PCLK_DIV4 0x00000003U
#define ADC_RESOLUTION_12B 0x00000000U
#define ADC_EXTERNALTRIGCONVEDGE_NONE 0x00000000U
#define ADC_SOFTWARE_START 0x00000000U
#define ADC_DATAALIGN_RIGHT 0x00000000U
#define ADC_EOC_SINGLE_CONV 0x00000000U
#define ADC_CHANNEL_0 0x00000000U
#define ADC_CHANNEL_1 0x00000001U
#define ADC_SAMPLETIME_480CYCLES 0x00000007U
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *hadc, ADC_ChannelConfTypeDef *sConfig);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc, uint32_t Timeout);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *hadc);

/* ---- TIM ---- */
typedef struct { } TIM_TypeDef;
typedef struct {
    uint32_t Prescaler, CounterMode, Period, ClockDivision;
    uint32_t RepetitionCounter, AutoReloadPreload;
} TIM_Base_InitTypeDef;
typedef struct { TIM_TypeDef *Instance; TIM_Base_InitTypeDef Init; } TIM_HandleTypeDef;
typedef struct {
    uint32_t OCMode, Pulse, OCPolarity, OCNPolarity, OCFastMode, OCIdleState, OCNIdleState;
} TIM_OC_InitTypeDef;
typedef struct {
    uint32_t OffStateRunMode, OffStateIDLEMode, LockLevel, DeadTime;
    uint32_t BreakState, BreakPolarity, AutomaticOutput;
} TIM_BreakDeadTimeConfigTypeDef;
#define TIM_COUNTERMODE_UP 0x00000000U
#define TIM_CLOCKDIVISION_DIV1 0x00000000U
#define TIM_AUTORELOAD_PRELOAD_DISABLE 0x00000000U
#define TIM_CHANNEL_1 0x00000000U
#define TIM_OCMODE_PWM1 0x00000006U
#define TIM_OCPOLARITY_HIGH 0x00000000U
#define TIM_OCNPOLARITY_HIGH 0x00000000U
#define TIM_OCFAST_DISABLE 0x00000000U
#define TIM_OCIDLESTATE_RESET 0x00000000U
#define TIM_OCNIDLESTATE_RESET 0x00000000U
#define TIM_OSSR_DISABLE 0x00000000U
#define TIM_OSSI_DISABLE 0x00000000U
#define TIM_LOCKLEVEL_OFF 0x00000000U
#define TIM_BREAK_DISABLE 0x00000000U
#define TIM_BREAKPOLARITY_HIGH 0x00000000U
#define TIM_AUTOMATICOUTPUT_DISABLE 0x00000000U
uint32_t HAL_RCC_GetPCLK2Freq(void);
HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIM_PWM_ConfigChannel(TIM_HandleTypeDef *htim, TIM_OC_InitTypeDef *sConfigOC, uint32_t Channel);
HAL_StatusTypeDef HAL_TIMEx_ConfigBreakDeadTime(TIM_HandleTypeDef *htim, TIM_BreakDeadTimeConfigTypeDef *sBreakDeadTimeConfig);
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
#define __HAL_TIM_MOE_ENABLE(__HANDLE__) do { (void)(__HANDLE__); } while (0)
#define __HAL_TIM_GET_AUTORELOAD(__HANDLE__) ((__HANDLE__)->Init.Period)
#define __HAL_TIM_SET_COMPARE(__HANDLE__, __CHANNEL__, __CCR__) do { (void)(__HANDLE__); (void)(__CHANNEL__); (void)(__CCR__); } while (0)

/* ---- UART ---- */
typedef struct { } USART_TypeDef;
typedef struct {
    uint32_t BaudRate, WordLength, StopBits, Parity, Mode, HwFlowCtl, OverSampling;
} UART_InitTypeDef;
typedef struct { USART_TypeDef *Instance; UART_InitTypeDef Init; } UART_HandleTypeDef;
#define UART_WORDLENGTH_8B 0x00000000U
#define UART_STOPBITS_1 0x00000000U
#define UART_PARITY_NONE 0x00000000U
#define UART_MODE_TX_RX 0x0000000CU
#define UART_HWCONTROL_NONE 0x00000000U
#define UART_OVERSAMPLING_16 0x00000000U
HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);

/* ---- IWDG ---- */
typedef struct { } IWDG_TypeDef;
typedef struct { uint32_t Prescaler, Reload; } IWDG_InitTypeDef;
typedef struct { IWDG_TypeDef *Instance; IWDG_InitTypeDef Init; } IWDG_HandleTypeDef;
#define IWDG_PRESCALER_4   0x00000000U
#define IWDG_PRESCALER_8   0x00000001U
#define IWDG_PRESCALER_16  0x00000002U
#define IWDG_PRESCALER_32  0x00000003U
#define IWDG_PRESCALER_64  0x00000004U
#define IWDG_PRESCALER_128 0x00000005U
#define IWDG_PRESCALER_256 0x00000006U
HAL_StatusTypeDef HAL_IWDG_Init(IWDG_HandleTypeDef *hiwdg);
HAL_StatusTypeDef HAL_IWDG_Refresh(IWDG_HandleTypeDef *hiwdg);
#define __HAL_RCC_CLEAR_RESET_FLAGS() do { } while (0)

/* ---- FLASH ---- */
typedef struct {
    uint32_t TypeErase, Banks, Sector, NbSectors, VoltageRange;
} FLASH_EraseInitTypeDef;
#define FLASH_TYPEERASE_SECTORS 0x00000000U
#define FLASH_VOLTAGE_RANGE_3 0x00000002U
#define FLASH_TYPEPROGRAM_WORD 0x00000002U
#define FLASH_SECTOR_0 0U
#define FLASH_SECTOR_1 1U
#define FLASH_SECTOR_2 2U
#define FLASH_SECTOR_3 3U
#define FLASH_SECTOR_4 4U
#define FLASH_SECTOR_5 5U
#define FLASH_SECTOR_6 6U
#define FLASH_SECTOR_7 7U
HAL_StatusTypeDef HAL_FLASH_Unlock(void);
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *pEraseInit, uint32_t *SectorError);
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint32_t Data);
#define __HAL_FLASH_DATA_CACHE_DISABLE()         do { } while (0)
#define __HAL_FLASH_DATA_CACHE_ENABLE()          do { } while (0)
#define __HAL_FLASH_DATA_CACHE_RESET()           do { } while (0)
#define __HAL_FLASH_INSTRUCTION_CACHE_DISABLE()  do { } while (0)
#define __HAL_FLASH_INSTRUCTION_CACHE_ENABLE()   do { } while (0)
#define __HAL_FLASH_INSTRUCTION_CACHE_RESET()    do { } while (0)

/* ---- I2C ---- */
typedef struct { } I2C_TypeDef;
typedef struct {
    uint32_t ClockSpeed, DutyCycle, OwnAddress1, AddressingMode;
    uint32_t DualAddressMode, OwnAddress2, GeneralCallMode, NoStretchMode;
} I2C_InitTypeDef;
typedef struct { I2C_TypeDef *Instance; I2C_InitTypeDef Init; } I2C_HandleTypeDef;
#define I2C_MEMADD_SIZE_8BIT 0x00000001U
#define I2C_DUTYCYCLE_2 0x00000000U
#define I2C_ADDRESSINGMODE_7BIT 0x00004000U
#define I2C_DUALADDRESS_DISABLE 0x00000000U
#define I2C_GENERALCALL_DISABLE 0x00000000U
#define I2C_NOSTRETCH_DISABLE 0x00000000U
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                   uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint16_t MemAddress,
                                    uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);

/* ---- RCC / Clock ---- */
typedef struct {
    uint32_t OscillatorType, HSEState;
    struct { uint32_t PLLState, PLLSource, PLLM, PLLN, PLLP, PLLQ; } PLL;
} RCC_OscInitTypeDef;
typedef struct {
    uint32_t ClockType, SYSCLKSource, AHBCLKDivider, APB1CLKDivider, APB2CLKDivider;
} RCC_ClkInitTypeDef;
#define RCC_OSCILLATORTYPE_HSE 0x00000001U
#define RCC_HSE_ON 0x00000001U
#define RCC_PLL_ON 0x00000001U
#define RCC_PLLSOURCE_HSE 0x00400000U
#define RCC_PLLP_DIV2 0x00000002U
#define RCC_CLOCKTYPE_HCLK   0x00000001U
#define RCC_CLOCKTYPE_SYSCLK 0x00000002U
#define RCC_CLOCKTYPE_PCLK1  0x00000004U
#define RCC_CLOCKTYPE_PCLK2  0x00000008U
#define RCC_SYSCLKSOURCE_PLLCLK 0x00000002U
#define RCC_SYSCLK_DIV1 0x00000000U
#define RCC_HCLK_DIV4 0x00000005U
#define RCC_HCLK_DIV2 0x00000004U
#define FLASH_LATENCY_5 0x00000005U
HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef *RCC_OscInitStruct);
HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef *RCC_ClkInitStruct, uint32_t FLatency);

/* ---- peripheral instances (레지스터 주소는 형식적) ---- */
#define ADC1    ((ADC_TypeDef *)0x40012000UL)
#define CAN1    ((CAN_TypeDef *)0x40006400UL)
#define TIM1    ((TIM_TypeDef *)0x40010000UL)
#define USART2  ((USART_TypeDef *)0x40004400UL)
#define I2C1    ((I2C_TypeDef *)0x40005400UL)
#define IWDG    ((IWDG_TypeDef *)0x40003000UL)

/* ---- delay ---- */
void HAL_Delay(uint32_t Delay);

#endif /* STM32F4XX_HAL_H */
'''

# 강제 포함 헤더(-include)용: main_fixed.c 가 직접 HAL 헤더를 포함하지 않으므로,
# CMSIS/HAL 이 제공하는 시스템 심볼을 여기에 선언한다.
F['extra_defs.h'] = r'''#ifndef EXTRA_DEFS_H
#define EXTRA_DEFS_H
void NVIC_SystemReset(void);
#define NVIC_PriorityGroup_4 0x00000003U
#endif
'''

for name, content in F.items():
    with open(os.path.join(OUT, name), 'w', encoding='utf-8') as fp:
        fp.write(content)
    print('stub 생성:', name, len(content), 'bytes')
