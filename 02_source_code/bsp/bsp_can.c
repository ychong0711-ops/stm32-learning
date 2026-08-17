// bsp_can.c — CAN 통신 BSP 구현 (STM32F4 HAL + FreeRTOS)입니다.
// CAN1(PB8/PB9)을 수신 인터럽트 + 링 버퍼 + 세마포어 구조로 구동합니다.
#include "bsp_can.h"  // CAN BSP 헤더를 포함합니다.

#include "stm32f4xx_hal.h"  // STM32F4 HAL 드라이버 헤더를 포함합니다.
#include "FreeRTOS.h"        // FreeRTOS 커널 헤더를 포함합니다.
#include "semphr.h"          // 세마포어 API 를 사용하기 위해 포함합니다.
#include "task.h"            // 태스크/임계구역 API 를 사용하기 위해 포함합니다.
#include "bsp_gpio.h"        // E2E 측정용 GPIO 토글 함수를 사용하기 위해 포함합니다.

#define CAN_RX_RING_SIZE  16U  // CAN 수신 링 버퍼의 크기를 16개로 정의합니다.

static CAN_HandleTypeDef s_hcan1;  // CAN1 의 HAL 핸들 구조체입니다.
static CAN_Message_t s_rxRing[CAN_RX_RING_SIZE];  // CAN 수신 메시지를 저장하는 링 버퍼입니다.
static volatile uint8_t s_rxHead = 0U;  // 링 버퍼 쓰기 인덱스(ISR 전용)입니다.
static volatile uint8_t s_rxTail = 0U;  // 링 버퍼 읽기 인덱스(태스크 전용)입니다.
static volatile uint8_t s_rxCount = 0U;  // 링 버퍼에 쌓인 메시지 수입니다.
static SemaphoreHandle_t s_xRxSemaphore = NULL;  // 수신 완료를 알리는 바이너리 세마포어 핸들입니다.

static void prvCanGpioInit(void);  // CAN 핀(GPIO)을 초기화하는 내부 함수 프로토타입입니다.
static void prvCanSetTiming(uint32_t ulBaudrate);  // 보레이트별 CAN 타이밍을 설정하는 내부 함수 프로토타입입니다.

BaseType_t CAN_Init(uint32_t ulBaudrate)  // CAN 초기화 함수를 정의합니다.
{  // CAN 초기화 함수 본문을 시작합니다.
    if (s_xRxSemaphore == NULL)  // 수신 세마포어가 아직 생성되지 않았는지 확인합니다.
    {  // 세마포어 생성 블록을 시작합니다.
        s_xRxSemaphore = xSemaphoreCreateBinary();  // 수신 알림용 바이너리 세마포어를 생성합니다.
        configASSERT(s_xRxSemaphore);  // 세마포어 생성 실패 시 시스템을 중단시킵니다.
    }  // 세마포어 생성 블록을 종료합니다.

    prvCanGpioInit();  // CAN GPIO 핀과 인터럽트를 초기화합니다.

    s_hcan1.Instance = CAN1;  // HAL 핸들에 CAN1 인스턴스를 연결합니다.
    prvCanSetTiming(ulBaudrate);  // 보레이트에 맞는 프리스케일러/타임퀀텀을 설정합니다.
    s_hcan1.Init.Mode = CAN_MODE_NORMAL;  // CAN 동작 모드를 노멀 모드로 설정합니다.
    s_hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;  // 동기 점프 폭을 1TQ 로 설정합니다.
    s_hcan1.Init.TimeSeg1 = CAN_BS1_13TQ;  // 타임 세그먼트1 을 13TQ 로 설정합니다. (아래에서 재설정)
    s_hcan1.Init.TimeSeg2 = CAN_BS2_2TQ;  // 타임 세그먼트2 를 2TQ 로 설정합니다. (아래에서 재설정)
    s_hcan1.Init.TimeTriggeredMode = DISABLE;  // 타임 트리거 모드를 비활성화합니다.
    s_hcan1.Init.AutoBusOff = DISABLE;  // 자동 버스오프 기능을 비활성화합니다.
    s_hcan1.Init.AutoWakeUp = DISABLE;  // 자동 웨이크업 기능을 비활성화합니다.
    s_hcan1.Init.AutoRetransmission = ENABLE;  // 송신 실패 시 자동 재전송을 활성화합니다.
    s_hcan1.Init.ReceiveFifoLocked = DISABLE;  // 수신 FIFO 잠금을 비활성화합니다.
    s_hcan1.Init.TransmitFifoPriority = DISABLE;  // 송신 FIFO 우선순위를 비활성화합니다.

    if (HAL_CAN_Init(&s_hcan1) != HAL_OK)  // CAN 주변장치 초기화에 성공했는지 확인합니다.
    {  // 초기화 실패 처리 블록을 시작합니다.
        return pdFAIL;  // 초기화 실패를 반환합니다.
    }  // 초기화 실패 처리 블록을 종료합니다.

    CAN_FilterConfig(0x000U, 0x000U);  // 기본 필터를 전체 수신으로 설정합니다.

    if (HAL_CAN_Start(&s_hcan1) != HAL_OK)  // CAN 컨트롤러 시작에 성공했는지 확인합니다.
    {  // 시작 실패 처리 블록을 시작합니다.
        return pdFAIL;  // 시작 실패를 반환합니다.
    }  // 시작 실패 처리 블록을 종료합니다.
    if (HAL_CAN_ActivateNotification(&s_hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)  // RX FIFO0 인터럽트 활성화에 성공했는지 확인합니다.
    {  // 인터럽트 활성화 실패 처리 블록을 시작합니다.
        return pdFAIL;  // 인터럽트 활성화 실패를 반환합니다.
    }  // 인터럽트 활성화 실패 처리 블록을 종료합니다.

    return pdPASS;  // CAN 초기화 성공을 반환합니다.
}  // CAN 초기화 함수를 종료합니다.

void CAN_FilterConfig(uint32_t ulFilterId, uint32_t ulFilterMask)  // CAN 수신 필터 설정 함수를 정의합니다.
{  // 필터 설정 함수 본문을 시작합니다.
    CAN_FilterTypeDef sFilterConfig;  // HAL 필터 설정 구조체를 선언합니다.

    sFilterConfig.FilterBank = 0;  // 필터 뱅크 0 을 사용하도록 설정합니다.
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;  // ID 마스크 모드로 설정합니다.
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;  // 32bit 스케일로 설정합니다.
    sFilterConfig.FilterIdHigh = (uint16_t)((ulFilterId << 5) & 0xFFFFU);  // ID 상위 비트를 설정합니다. (5bit 왼쪽 시프트)
    sFilterConfig.FilterIdLow = 0x0000U;  // ID 하위 비트를 0으로 설정합니다.
    sFilterConfig.FilterMaskIdHigh = (uint16_t)((ulFilterMask << 5) & 0xFFFFU);  // 마스크 상위 비트를 설정합니다.
    sFilterConfig.FilterMaskIdLow = 0x0000U;  // 마스크 하위 비트를 0으로 설정합니다.
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;  // 매칭 메시지를 RX FIFO0 으로 보내도록 설정합니다.
    sFilterConfig.FilterActivation = ENABLE;  // 필터를 활성화합니다.
    sFilterConfig.SlaveStartFilterBank = 14;  // 슬레이브 시작 필터 뱅크를 14로 설정합니다. (단일 CAN 이라 무관)

    (void)HAL_CAN_ConfigFilter(&s_hcan1, &sFilterConfig);  // 설정한 필터를 CAN1 에 적용합니다.
}  // 필터 설정 함수를 종료합니다.

BaseType_t CAN_Receive(CAN_Message_t *pxMsg, TickType_t xTimeout)  // CAN 수신 함수를 정의합니다.
{  // CAN 수신 함수 본문을 시작합니다.
    if (pxMsg == NULL)  // 출력 메시지 포인터가 유효한지 확인합니다.
    {  // NULL 포인터 처리 블록을 시작합니다.
        return pdFAIL;  // 유효하지 않으면 실패를 반환합니다.
    }  // NULL 포인터 처리 블록을 종료합니다.

    if (xSemaphoreTake(s_xRxSemaphore, xTimeout) != pdTRUE)  // 메시지 도착을 타임아웃 동안 대기합니다.
    {  // 대기 실패(타임아웃) 처리 블록을 시작합니다.
        return pdFAIL;  // 타임아웃이면 실패를 반환합니다.
    }  // 대기 실패 처리 블록을 종료합니다.

    taskENTER_CRITICAL();  // ISR 과의 경쟁을 막기 위해 임계 구역에 진입합니다.
    if (s_rxCount == 0U)  // 버퍼가 실제로 비어있는지 방어적으로 확인합니다.
    {  // 버퍼 빈 경우 처리 블록을 시작합니다.
        taskEXIT_CRITICAL();  // 임계 구역에서 빠져나옵니다.
        return pdFAIL;  // 방어적으로 실패를 반환합니다.
    }  // 버퍼 빈 경우 처리 블록을 종료합니다.
    *pxMsg = s_rxRing[s_rxTail];  // 읽기 인덱스의 메시지를 출력 변수에 복사합니다.
    s_rxTail = (uint8_t)((s_rxTail + 1U) % CAN_RX_RING_SIZE);  // 읽기 인덱스를 순환 증가시킵니다.
    s_rxCount--;  // 버퍼에 남은 메시지 수를 감소시킵니다.
    taskEXIT_CRITICAL();  // 임계 구역에서 빠져나옵니다.

    return pdPASS;  // 수신 성공을 반환합니다.
}  // CAN 수신 함수를 종료합니다.

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)  // CAN RX FIFO0 메시지 대기 콜백(ISR)을 정의합니다.
{  // 콜백 함수 본문을 시작합니다.
    CAN_RxHeaderTypeDef xRxHeader;  // 수신 헤더(ID, DLC 등)를 저장할 구조체입니다.
    uint8_t ucData[8] = {0};  // 수신 데이터를 저장할 8바이트 버퍼입니다.
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;  // 문맥 전환 필요 여부를 저장하는 변수입니다.

    if (hcan->Instance != CAN1)  // 이 인터럽트가 CAN1 의 것인지 확인합니다.
    {  // CAN1 이 아닌 경우 처리 블록을 시작합니다.
        return;  // 다른 CAN 인스턴스의 인터럽트라면 그냥 반환합니다.
    }  // CAN1 이 아닌 경우 처리 블록을 종료합니다.

    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &xRxHeader, ucData) != HAL_OK)  // FIFO0 에서 메시지를 읽어옵니다.
    {  // 메시지 읽기 실패 처리 블록을 시작합니다.
        return;  // 읽기에 실패하면 반환합니다.
    }  // 메시지 읽기 실패 처리 블록을 종료합니다.

    if (s_rxCount < CAN_RX_RING_SIZE)  // 링 버퍼에 공간이 남아있는지 확인합니다.
    {  // 버퍼 저장 블록을 시작합니다.
        CAN_Message_t *pxSlot = &s_rxRing[s_rxHead];  // 쓰기 인덱스의 슬롯을 가져옵니다.
        uint8_t ucLen = (xRxHeader.DLC > 8U) ? 8U : (uint8_t)xRxHeader.DLC;  // DLC 가 8 을 넘지 않도록 제한합니다.

        pxSlot->ID = (xRxHeader.IDE == CAN_ID_EXT) ? xRxHeader.ExtId : xRxHeader.StdId;  // 확장/표준 ID 를 선택해 저장합니다.
        pxSlot->DLC = ucLen;  // 데이터 길이를 저장합니다.
        for (uint8_t i = 0U; i < ucLen; i++)  // 데이터 길이만큼 반복합니다.
        {  // 데이터 복사 반복문 본문을 시작합니다.
            pxSlot->data[i] = ucData[i];  // 수신 데이터를 링 버퍼 슬롯에 복사합니다.
        }  // 데이터 복사 반복문을 종료합니다.

        s_rxHead = (uint8_t)((s_rxHead + 1U) % CAN_RX_RING_SIZE);  // 쓰기 인덱스를 순환 증가시킵니다.
        s_rxCount++;  // 버퍼에 쌓인 메시지 수를 증가시킵니다.
    }  // 버퍼 저장 블록을 종료합니다.

    GPIO_Toggle(BSP_GPIO_PIN_MEAS_E2E);  // E2E 응답시간 측정용 토글 핀을 반전합니다. (지점 A)

    if (s_xRxSemaphore != NULL)  // 수신 세마포어 핸들이 유효한지 확인합니다.
    {  // 세마포어 give 블록을 시작합니다.
        (void)xSemaphoreGiveFromISR(s_xRxSemaphore, &xHigherPriorityTaskWoken);  // 대기 중인 태스크를 깨웁니다.
    }  // 세마포어 give 블록을 종료합니다.
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);  // 필요하면 문맥 전환을 요청합니다.
}  // 콜백 함수를 종료합니다.

static void prvCanGpioInit(void)  // CAN GPIO 초기화 함수를 정의합니다.
{  // CAN GPIO 초기화 함수 본문을 시작합니다.
    GPIO_InitTypeDef GPIO_InitStruct = {0};  // GPIO 초기화 구조체를 0으로 선언합니다.

    __HAL_RCC_GPIOB_CLK_ENABLE();  // GPIOB 클록을 활성화합니다.
    __HAL_RCC_CAN1_CLK_ENABLE();  // CAN1 클록을 활성화합니다.

    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;  // PB8(RX)과 PB9(TX) 핀을 선택합니다.
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;  // 대체 기능 푸시풀 모드로 설정합니다.
    GPIO_InitStruct.Pull = GPIO_NOPULL;  // 내부 풀업/풀다운을 사용하지 않습니다.
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;  // 핀 속도를 최고속으로 설정합니다.
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;  // 대체 기능을 AF9(CAN1)로 설정합니다.
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);  // 설정을 GPIOB 에 적용합니다.

    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 5, 0);  // CAN RX0 인터럽트 우선순위를 5로 설정합니다. (FromISR 호출 안전 대역)
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);  // CAN RX0 인터럽트를 활성화합니다.
}  // CAN GPIO 초기화 함수를 종료합니다.

static void prvCanSetTiming(uint32_t ulBaudrate)  // 보레이트별 CAN 타이밍 설정 함수를 정의합니다.
{  // 타이밍 설정 함수 본문을 시작합니다.
    switch (ulBaudrate)  // 요청 보레이트에 따라 분기합니다.
    {  // 보레이트 switch 블록을 시작합니다.
        case 125000U:  // 125kbps 요청인 경우입니다.
            s_hcan1.Init.Prescaler = 20U;  // 프리스케일러 20 → 2.25MHz 로 설정합니다.
            s_hcan1.Init.TimeSeg1 = CAN_BS1_13TQ;  // 타임 세그먼트1 을 13TQ 로 설정합니다.
            s_hcan1.Init.TimeSeg2 = CAN_BS2_4TQ;  // 타임 세그먼트2 를 4TQ 로 설정합니다. (합 18TQ)
            break;  // 125kbps 케이스를 종료합니다.
        case 250000U:  // 250kbps 요청인 경우입니다.
            s_hcan1.Init.Prescaler = 10U;  // 프리스케일러 10 → 4.5MHz 로 설정합니다.
            s_hcan1.Init.TimeSeg1 = CAN_BS1_13TQ;  // 타임 세그먼트1 을 13TQ 로 설정합니다.
            s_hcan1.Init.TimeSeg2 = CAN_BS2_4TQ;  // 타임 세그먼트2 를 4TQ 로 설정합니다.
            break;  // 250kbps 케이스를 종료합니다.
        case 1000000U:  // 1Mbps 요청인 경우입니다.
            s_hcan1.Init.Prescaler = 3U;  // 프리스케일러 3 → 15MHz 로 설정합니다.
            s_hcan1.Init.TimeSeg1 = CAN_BS1_10TQ;  // 타임 세그먼트1 을 10TQ 로 설정합니다.
            s_hcan1.Init.TimeSeg2 = CAN_BS2_4TQ;  // 타임 세그먼트2 를 4TQ 로 설정합니다. (합 15TQ)
            break;  // 1Mbps 케이스를 종료합니다.
        case 500000U:  // 500kbps 요청인 경우입니다.
        default:  // 그 외(또는 500kbps) 요청인 경우입니다.
            s_hcan1.Init.Prescaler = 5U;  // 프리스케일러 5 → 9MHz 로 설정합니다.
            s_hcan1.Init.TimeSeg1 = CAN_BS1_13TQ;  // 타임 세그먼트1 을 13TQ 로 설정합니다.
            s_hcan1.Init.TimeSeg2 = CAN_BS2_4TQ;  // 타임 세그먼트2 를 4TQ 로 설정합니다. (합 18TQ, 샘플포인트 약 77.8%)
            break;  // 500kbps(기본값) 케이스를 종료합니다.
    }  // 보레이트 switch 블록을 종료합니다.
}  // 타이밍 설정 함수를 종료합니다.
