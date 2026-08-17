// bsp_pwm.c — PWM BSP 구현 (STM32F4 HAL, TIM1 어드밴스드 타이머)입니다.
// TIM1_CH1(PA8)을 사용하며, 듀티는 0~1000 퍼밀 단위로 표현합니다.
#include "bsp_pwm.h"  // PWM BSP 헤더를 포함합니다.

#include "stm32f4xx_hal.h"  // STM32F4 HAL 드라이버 헤더를 포함합니다.

static TIM_HandleTypeDef s_htim1;  // TIM1 의 HAL 핸들 구조체입니다.
static uint32_t s_ulTimerClockHz = 0U;  // 타이머 클록 주파수(Hz)를 저장하는 변수입니다.

void PWM_Init(uint32_t ulChannel, uint32_t ulFrequencyHz)  // PWM 초기화 함수를 정의합니다.
{  // PWM 초기화 함수 본문을 시작합니다.
    GPIO_InitTypeDef GPIO_InitStruct = {0};  // GPIO 초기화 구조체를 0으로 선언합니다.
    TIM_OC_InitTypeDef sConfigOC = {0};  // 타이머 출력 비교 설정 구조체를 0으로 선언합니다.
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTime = {0};  // 브레이크/데드타임 설정 구조체를 0으로 선언합니다.

    (void)ulChannel;  // 현재는 채널 0(스로틀)만 지원하므로 사용하지 않는 인자를 무시합니다.

    __HAL_RCC_GPIOA_CLK_ENABLE();  // GPIOA 클록을 활성화합니다.
    __HAL_RCC_TIM1_CLK_ENABLE();  // TIM1 클록을 활성화합니다.

    GPIO_InitStruct.Pin = GPIO_PIN_8;  // PA8(TIM1_CH1) 핀을 선택합니다.
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;  // 대체 기능 푸시풀 모드로 설정합니다.
    GPIO_InitStruct.Pull = GPIO_NOPULL;  // 내부 풀업/풀다운을 사용하지 않습니다.
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;  // 핀 속도를 고속으로 설정합니다.
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;  // 대체 기능을 AF1(TIM1)로 설정합니다.
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);  // 설정을 GPIOA 에 적용합니다.

    s_ulTimerClockHz = HAL_RCC_GetPCLK2Freq();  // APB2 타이머 클록 주파수를 읽어 저장합니다. (180MHz)

    uint32_t ulARR = (s_ulTimerClockHz / ulFrequencyHz) - 1U;  // 목표 주파수에서 ARR 값을 계산합니다.
    if (ulARR > 0xFFFFU)  // 계산된 ARR 이 16bit 범위를 넘는지 확인합니다.
    {  // ARR 초과 처리 블록을 시작합니다.
        ulARR = 0xFFFFU;  // ARR 을 16bit 최대값으로 제한합니다. (필요 시 프리스케일러 조정)
    }  // ARR 초과 처리 블록을 종료합니다.

    s_htim1.Instance = TIM1;  // HAL 핸들에 TIM1 인스턴스를 연결합니다.
    s_htim1.Init.Prescaler = 0;  // 프리스케일러를 0(분주 없음)으로 설정합니다.
    s_htim1.Init.CounterMode = TIM_COUNTERMODE_UP;  // 카운터를 상향 카운트 모드로 설정합니다.
    s_htim1.Init.Period = ulARR;  // 자동 재로드 값(주기)을 계산한 ARR 로 설정합니다.
    s_htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;  // 클록 분주를 1로 설정합니다.
    s_htim1.Init.RepetitionCounter = 0;  // 반복 카운터를 0으로 설정합니다.
    s_htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;  // 자동 재로드 프리로드를 비활성화합니다.
    (void)HAL_TIM_PWM_Init(&s_htim1);  // TIM1 을 PWM 모드로 초기화합니다.

    sConfigOC.OCMode = TIM_OCMODE_PWM1;  // 출력 비교 모드를 PWM 모드 1로 설정합니다.
    sConfigOC.Pulse = 0U;  // 초기 펄스(듀티)를 0으로 설정합니다.
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;  // 출력 극성을 액티브 하이로 설정합니다.
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;  // 보완 출력 극성을 액티브 하이로 설정합니다.
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;  // 고속 모드를 비활성화합니다.
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;  // 유휴 상태 출력을 리셋으로 설정합니다.
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;  // 보완 출력 유휴 상태를 리셋으로 설정합니다.
    (void)HAL_TIM_PWM_ConfigChannel(&s_htim1, &sConfigOC, TIM_CHANNEL_1);  // 채널1 에 설정을 적용합니다.

    sBreakDeadTime.OffStateRunMode = TIM_OSSR_DISABLE;  // 오프 상태 런 모드를 비활성화합니다.
    sBreakDeadTime.OffStateIDLEMode = TIM_OSSI_DISABLE;  // 오프 상태 유휴 모드를 비활성화합니다.
    sBreakDeadTime.LockLevel = TIM_LOCKLEVEL_OFF;  // 레지스터 잠금을 해제 상태로 설정합니다.
    sBreakDeadTime.DeadTime = 0;  // 데드타임을 0으로 설정합니다.
    sBreakDeadTime.BreakState = TIM_BREAK_DISABLE;  // 브레이크 기능을 비활성화합니다.
    sBreakDeadTime.BreakPolarity = TIM_BREAKPOLARITY_HIGH;  // 브레이크 극성을 하이로 설정합니다.
    sBreakDeadTime.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;  // 자동 출력 활성화를 비활성화합니다.
    (void)HAL_TIMEx_ConfigBreakDeadTime(&s_htim1, &sBreakDeadTime);  // 브레이크/데드타임 설정을 적용합니다.

    (void)HAL_TIM_PWM_Start(&s_htim1, TIM_CHANNEL_1);  // 채널1 PWM 출력을 시작합니다.
    __HAL_TIM_MOE_ENABLE(&s_htim1);  // 메인 출력(MOE)을 활성화합니다. (어드밴스드 타이머 필수)
}  // PWM 초기화 함수를 종료합니다.

void PWM_SetDuty(uint32_t ulChannel, uint32_t ulDutyPermille)  // PWM 듀티 설정 함수를 정의합니다.
{  // PWM 듀티 설정 함수 본문을 시작합니다.
    (void)ulChannel;  // 현재는 채널 0만 지원하므로 사용하지 않는 인자를 무시합니다.

    if (ulDutyPermille > 1000U)  // 듀티가 1000 퍼밀(100%)을 넘는지 확인합니다.
    {  // 듀티 클램프 블록을 시작합니다.
        ulDutyPermille = 1000U;  // 최대 1000 퍼밀로 제한합니다.
    }  // 듀티 클램프 블록을 종료합니다.

    uint32_t ulARR = __HAL_TIM_GET_AUTORELOAD(&s_htim1);  // 현재 ARR 값을 읽어옵니다.
    uint32_t ulCCR = (ulARR * ulDutyPermille) / 1000U;  // 듀티 비율로 CCR 값을 계산합니다.

    __HAL_TIM_SET_COMPARE(&s_htim1, TIM_CHANNEL_1, ulCCR);  // 계산한 CCR 을 채널1 에 적용합니다.
}  // PWM 듀티 설정 함수를 종료합니다.
