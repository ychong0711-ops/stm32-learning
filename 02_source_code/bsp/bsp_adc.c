// bsp_adc.c — ADC BSP 구현 (STM32F4 HAL, 폴링 단일 변환)입니다.
// ADC1 의 PA0(채널0), PA1(채널1) 을 폴링 방식으로 읽습니다.
#include "bsp_adc.h"  // ADC BSP 헤더를 포함합니다.

#include "stm32f4xx_hal.h"  // STM32F4 HAL 드라이버 헤더를 포함합니다.

static ADC_HandleTypeDef s_hadc1;  // ADC1 의 HAL 핸들 구조체입니다.

void ADC_Init(void)  // ADC 초기화 함수를 정의합니다.
{  // ADC 초기화 함수 본문을 시작합니다.
    GPIO_InitTypeDef GPIO_InitStruct = {0};  // GPIO 초기화 구조체를 0으로 선언합니다.

    __HAL_RCC_GPIOA_CLK_ENABLE();  // GPIOA 클록을 활성화합니다.
    __HAL_RCC_ADC1_CLK_ENABLE();  // ADC1 클록을 활성화합니다.

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;  // PA0, PA1 핀을 선택합니다.
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;  // 아날로그 입력 모드로 설정합니다.
    GPIO_InitStruct.Pull = GPIO_NOPULL;  // 내부 풀업/풀다운을 사용하지 않습니다.
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);  // 설정을 GPIOA 에 적용합니다.

    s_hadc1.Instance = ADC1;  // HAL 핸들에 ADC1 인스턴스를 연결합니다.
    s_hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;  // ADC 클록을 PCLK2/4(22.5MHz)로 설정합니다.
    s_hadc1.Init.Resolution = ADC_RESOLUTION_12B;  // 분해능을 12bit 로 설정합니다.
    s_hadc1.Init.ScanConvMode = DISABLE;  // 스캔 변환 모드를 비활성화합니다. (단일 채널)
    s_hadc1.Init.ContinuousConvMode = DISABLE;  // 연속 변환 모드를 비활성화합니다.
    s_hadc1.Init.DiscontinuousConvMode = DISABLE;  // 불연속 변환 모드를 비활성화합니다.
    s_hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;  // 외부 트리거 엣지를 사용하지 않습니다.
    s_hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;  // 소프트웨어 시작 방식으로 설정합니다.
    s_hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;  // 데이터 정렬을 오른쪽 정렬로 설정합니다.
    s_hadc1.Init.NbrOfConversion = 1;  // 변환 개수를 1로 설정합니다.
    s_hadc1.Init.DMAContinuousRequests = DISABLE;  // DMA 연속 요청을 비활성화합니다.
    s_hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;  // 단일 변환 종료를 EOC 기준으로 설정합니다.

    (void)HAL_ADC_Init(&s_hadc1);  // ADC1 을 초기화합니다.
}  // ADC 초기화 함수를 종료합니다.

uint16_t ADC_ReadChannel(uint32_t ulChannel)  // ADC 채널 읽기 함수를 정의합니다.
{  // ADC 채널 읽기 함수 본문을 시작합니다.
    ADC_ChannelConfTypeDef sConfig = {0};  // ADC 채널 설정 구조체를 0으로 선언합니다.
    uint32_t ulHalChannel;  // HAL 채널 번호를 저장할 변수입니다.

    switch (ulChannel)  // 논리 채널 번호에 따라 분기합니다.
    {  // 채널 매핑 switch 블록을 시작합니다.
        case BSP_ADC_CHANNEL_RPM:  // RPM 센서 채널인 경우입니다.
            ulHalChannel = ADC_CHANNEL_0;  // PA0(ADC1_IN0)으로 매핑합니다.
            break;  // RPM 채널 케이스를 종료합니다.
        case BSP_ADC_CHANNEL_THROTTLE:  // 스로틀 센서 채널인 경우입니다.
            ulHalChannel = ADC_CHANNEL_1;  // PA1(ADC1_IN1)으로 매핑합니다.
            break;  // 스로틀 채널 케이스를 종료합니다.
        default:  // 정의되지 않은 채널인 경우입니다.
            ulHalChannel = ADC_CHANNEL_0;  // 기본값으로 채널 0 을 사용합니다.
            break;  // 기본 케이스를 종료합니다.
    }  // 채널 매핑 switch 블록을 종료합니다.

    sConfig.Channel = ulHalChannel;  // 변환할 HAL 채널을 설정합니다.
    sConfig.Rank = 1;  // 변환 순위를 1로 설정합니다.
    sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;  // 샘플링 시간을 480 사이클로 설정합니다. (노이즈 억제에 유리)
    (void)HAL_ADC_ConfigChannel(&s_hadc1, &sConfig);  // 채널 설정을 ADC1 에 적용합니다.

    (void)HAL_ADC_Start(&s_hadc1);  // ADC 변환을 시작합니다.
    if (HAL_ADC_PollForConversion(&s_hadc1, 10U) != HAL_OK)  // 변환 완료를 최대 10ms 동안 폴링 대기합니다.
    {  // 변환 실패 처리 블록을 시작합니다.
        (void)HAL_ADC_Stop(&s_hadc1);  // ADC 를 정지합니다.
        return 0U;  // 변환 실패 시 0 을 반환합니다.
    }  // 변환 실패 처리 블록을 종료합니다.
    uint16_t usValue = (uint16_t)HAL_ADC_GetValue(&s_hadc1);  // 변환 결과 값을 읽어 저장합니다.
    (void)HAL_ADC_Stop(&s_hadc1);  // ADC 를 정지합니다.

    return usValue;  // 읽은 ADC 값을 반환합니다.
}  // ADC 채널 읽기 함수를 종료합니다.
