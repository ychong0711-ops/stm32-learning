// bsp_gpio.c — GPIO BSP 구현 (논리 핀 → 물리 핀 매핑)입니다.
// 논리 핀 ID 를 (포트, 핀) 테이블로 변환하여 제어합니다.
#include "bsp_gpio.h"  // GPIO BSP 헤더를 포함합니다.

#include "stm32f4xx_hal.h"  // STM32F4 HAL 드라이버 헤더를 포함합니다.

typedef struct  // 논리 핀 매핑 테이블 항목 구조체 정의를 시작합니다.
{  // 구조체 멤버 선언을 시작합니다.
    uint32_t        ulLogicalPin;  // 논리 핀 ID 를 저장합니다.
    GPIO_TypeDef   *pPort;         // 물리 GPIO 포트 포인터를 저장합니다.
    uint16_t        usPin;         // 물리 GPIO 핀 번호를 저장합니다.
} BspPinMap_t;  // 매핑 테이블 항목 타입 이름을 BspPinMap_t 로 정의합니다.

static const BspPinMap_t s_pinMap[] =  // 논리 핀 매핑 테이블을 정의합니다.
{  // 매핑 테이블 초기화를 시작합니다.
    { BSP_GPIO_PIN_FAN,          GPIOB, GPIO_PIN_13 },  // 팬 제어 핀을 PB13 으로 매핑합니다.
    { BSP_GPIO_PIN_MEAS_JITTER,  GPIOA, GPIO_PIN_4  },  // 주기 지터 측정용 핀을 PA4 로 매핑합니다.
    { BSP_GPIO_PIN_MEAS_E2E,     GPIOA, GPIO_PIN_7  },  // E2E 응답 측정용 핀을 PA7 로 매핑합니다.
    { BSP_GPIO_PIN_LED_ERR,      GPIOA, GPIO_PIN_6  },  // 오류 LED 핀을 PA6 로 매핑합니다.
};  // 매핑 테이블 초기화를 종료합니다.

#define BSP_PIN_MAP_COUNT  (sizeof(s_pinMap) / sizeof(s_pinMap[0]))  // 매핑 테이블의 항목 개수를 계산하는 매크로입니다.

static const BspPinMap_t *prvFindPin(uint32_t ulPin)  // 논리 핀 ID 로 테이블 항목을 찾는 내부 함수를 정의합니다.
{  // 핀 검색 함수 본문을 시작합니다.
    for (uint32_t i = 0U; i < BSP_PIN_MAP_COUNT; i++)  // 매핑 테이블 전체를 순회합니다.
    {  // 순회 반복문 본문을 시작합니다.
        if (s_pinMap[i].ulLogicalPin == ulPin)  // 현재 항목의 논리 ID 가 찾는 값과 일치하는지 확인합니다.
        {  // 일치하는 항목 발견 블록을 시작합니다.
            return &s_pinMap[i];  // 일치하는 항목의 포인터를 반환합니다.
        }  // 일치하는 항목 발견 블록을 종료합니다.
    }  // 순회 반복문을 종료합니다.
    return NULL;  // 일치하는 항목이 없으면 NULL 을 반환합니다.
}  // 핀 검색 함수를 종료합니다.

void GPIO_Init(uint32_t ulPin)  // GPIO 초기화 함수를 정의합니다.
{  // GPIO 초기화 함수 본문을 시작합니다.
    const BspPinMap_t *pMap = prvFindPin(ulPin);  // 논리 핀 ID 로 매핑 항목을 찾습니다.
    if (pMap == NULL)  // 매핑 항목이 존재하지 않는지 확인합니다.
    {  // 미등록 핀 처리 블록을 시작합니다.
        return;  // 등록되지 않은 핀이면 그냥 반환합니다.
    }  // 미등록 핀 처리 블록을 종료합니다.

    GPIO_InitTypeDef GPIO_InitStruct = {0};  // GPIO 초기화 구조체를 0으로 선언합니다.

    if (pMap->pPort == GPIOA)  // 대상 포트가 GPIOA 인지 확인합니다.
    {  // GPIOA 클록 활성화 블록을 시작합니다.
        __HAL_RCC_GPIOA_CLK_ENABLE();  // GPIOA 클록을 활성화합니다.
    }  // GPIOA 클록 활성화 블록을 종료합니다.
    else if (pMap->pPort == GPIOB)  // 대상 포트가 GPIOB 인지 확인합니다.
    {  // GPIOB 클록 활성화 블록을 시작합니다.
        __HAL_RCC_GPIOB_CLK_ENABLE();  // GPIOB 클록을 활성화합니다.
    }  // GPIOB 클록 활성화 블록을 종료합니다.
    else if (pMap->pPort == GPIOC)  // 대상 포트가 GPIOC 인지 확인합니다.
    {  // GPIOC 클록 활성화 블록을 시작합니다.
        __HAL_RCC_GPIOC_CLK_ENABLE();  // GPIOC 클록을 활성화합니다.
    }  // GPIOC 클록 활성화 블록을 종료합니다.
    else if (pMap->pPort == GPIOD)  // 대상 포트가 GPIOD 인지 확인합니다.
    {  // GPIOD 클록 활성화 블록을 시작합니다.
        __HAL_RCC_GPIOD_CLK_ENABLE();  // GPIOD 클록을 활성화합니다.
    }  // GPIOD 클록 활성화 블록을 종료합니다.

    GPIO_InitStruct.Pin = pMap->usPin;  // 초기화할 물리 핀 번호를 설정합니다.
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  // 출력 푸시풀 모드로 설정합니다.
    GPIO_InitStruct.Pull = GPIO_NOPULL;  // 내부 풀업/풀다운을 사용하지 않습니다.
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;  // 핀 속도를 저속으로 설정합니다.
    HAL_GPIO_Init(pMap->pPort, &GPIO_InitStruct);  // 설정을 대상 포트에 적용합니다.

    HAL_GPIO_WritePin(pMap->pPort, pMap->usPin, GPIO_PIN_RESET);  // 초기 출력을 LOW 로 설정합니다.
}  // GPIO 초기화 함수를 종료합니다.

void GPIO_Write(uint32_t ulPin, uint8_t ucState)  // GPIO 출력 함수를 정의합니다.
{  // GPIO 출력 함수 본문을 시작합니다.
    const BspPinMap_t *pMap = prvFindPin(ulPin);  // 논리 핀 ID 로 매핑 항목을 찾습니다.
    if (pMap == NULL)  // 매핑 항목이 존재하지 않는지 확인합니다.
    {  // 미등록 핀 처리 블록을 시작합니다.
        return;  // 등록되지 않은 핀이면 그냥 반환합니다.
    }  // 미등록 핀 처리 블록을 종료합니다.
    HAL_GPIO_WritePin(pMap->pPort, pMap->usPin,  // 대상 핀에 상태를 출력합니다.
                      (ucState != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);  // 0이 아니면 SET, 0이면 RESET 을 출력합니다.
}  // GPIO 출력 함수를 종료합니다.

void GPIO_Toggle(uint32_t ulPin)  // GPIO 토글 함수를 정의합니다.
{  // GPIO 토글 함수 본문을 시작합니다.
    const BspPinMap_t *pMap = prvFindPin(ulPin);  // 논리 핀 ID 로 매핑 항목을 찾습니다.
    if (pMap == NULL)  // 매핑 항목이 존재하지 않는지 확인합니다.
    {  // 미등록 핀 처리 블록을 시작합니다.
        return;  // 등록되지 않은 핀이면 그냥 반환합니다.
    }  // 미등록 핀 처리 블록을 종료합니다.
    HAL_GPIO_TogglePin(pMap->pPort, pMap->usPin);  // 대상 핀의 출력을 반전합니다.
}  // GPIO 토글 함수를 종료합니다.
