// bsp_iwdg.c — 독립 워치독(IWDG) BSP 구현 (STM32F4 HAL)입니다.
// LSI(약 32kHz) 기반이며, 한번 시작하면 멈출 수 없습니다.
#include "bsp_iwdg.h"  // IWDG BSP 헤더를 포함합니다.

#include "stm32f4xx_hal.h"  // STM32F4 HAL 드라이버 헤더를 포함합니다.

static IWDG_HandleTypeDef s_hiwdg;  // IWDG 의 HAL 핸들 구조체입니다.

void IWDG_Init(uint32_t ulTimeoutMs)  // IWDG 초기화 함수를 정의합니다. (1회만 호출)
{  // IWDG 초기화 함수 본문을 시작합니다.
    uint32_t ulLsiHz = 32000U;  // LSI 공칭 주파수를 32kHz 로 가정합니다. (실제 17~47kHz 편차)
    uint32_t ulPrescalerDiv = 4U;  // 분주값을 4부터 시작합니다.
    uint32_t ulReload;  // 계산된 리로드 값을 저장할 변수입니다.

    // 분주를 4 → 8 → ... → 256 으로 키우며 리로드가 12bit 에 들어오는 조합을 찾습니다.
    // 최대 설정 가능 타임아웃은 분주 256, 리로드 4095 일 때
    // 4095 * 256 / 32000 ≈ 32.7초입니다.
    for (;;)  // 적절한 분주를 찾을 때까지 반복합니다.
    {  // 리로드 계산 반복문 본문을 시작합니다.
        ulReload = (ulLsiHz / 1000U) * ulTimeoutMs / ulPrescalerDiv;  // 타임아웃과 분주로 리로드 값을 계산합니다.
        if (ulReload <= 0xFFFU)  // 계산된 리로드가 12bit(0xFFF) 이하인지 확인합니다.
        {  // 범위 충족 확인 블록을 시작합니다.
            break;  // 범위 안이면 반복을 종료합니다.
        }  // 범위 충족 확인 블록을 종료합니다.

        if (ulPrescalerDiv >= 256U)  // 더 이상 분주를 키울 수 없는지 확인합니다.
        {  // 최대 분주 도달 처리 블록을 시작합니다.
            // 요청한 타임아웃이 하드웨어 한계(약 32.7초)를 넘습니다.
            // 이전 구현은 여기서 분주를 512 로 키운 채 루프를 빠져나가,
            // 512 로 나눈 리로드 값을 분주 256 과 함께 설정했습니다. 그 결과
            // 실제 타임아웃이 요청값의 절반이 되는데도 아무 신호가 없었습니다.
            // 워치독이 의도보다 일찍 물어 정상 동작 중 리셋을 유발하므로,
            // 조용히 틀린 값을 쓰는 대신 표현 가능한 최댓값으로 포화시킵니다.
            ulReload = 0xFFFU;  // 리로드를 12bit 최댓값으로 포화시킵니다.
            break;  // 최대 분주 상태로 반복을 종료합니다.
        }  // 최대 분주 도달 처리 블록을 종료합니다.

        ulPrescalerDiv <<= 1U;  // 분주값을 2배로 증가시킵니다. (4 → 8 → 16 ...)
    }  // 리로드 계산 반복문을 종료합니다.

    if (ulReload == 0U)  // 타임아웃이 너무 짧아 리로드가 0 이 되었는지 확인합니다.
    {  // 리로드 하한 처리 블록을 시작합니다.
        ulReload = 1U;  // 리로드 0 은 즉시 리셋을 뜻하므로 최소 1 로 올립니다.
    }  // 리로드 하한 처리 블록을 종료합니다.

    uint32_t ulHALPrescaler;  // HAL 프리스케일러 열거값을 저장할 변수입니다.
    switch (ulPrescalerDiv)  // 계산된 분주값을 HAL 열거값으로 매핑합니다.
    {  // 프리스케일러 매핑 switch 블록을 시작합니다.
        case 4U:  // 분주 4인 경우입니다.
            ulHALPrescaler = IWDG_PRESCALER_4;  // HAL 의 /4 프리스케일러로 매핑합니다.
            break;  // 분주 4 케이스를 종료합니다.
        case 8U:  // 분주 8인 경우입니다.
            ulHALPrescaler = IWDG_PRESCALER_8;  // HAL 의 /8 프리스케일러로 매핑합니다.
            break;  // 분주 8 케이스를 종료합니다.
        case 16U:  // 분주 16인 경우입니다.
            ulHALPrescaler = IWDG_PRESCALER_16;  // HAL 의 /16 프리스케일러로 매핑합니다.
            break;  // 분주 16 케이스를 종료합니다.
        case 32U:  // 분주 32인 경우입니다.
            ulHALPrescaler = IWDG_PRESCALER_32;  // HAL 의 /32 프리스케일러로 매핑합니다.
            break;  // 분주 32 케이스를 종료합니다.
        case 64U:  // 분주 64인 경우입니다.
            ulHALPrescaler = IWDG_PRESCALER_64;  // HAL 의 /64 프리스케일러로 매핑합니다.
            break;  // 분주 64 케이스를 종료합니다.
        case 128U:  // 분주 128인 경우입니다.
            ulHALPrescaler = IWDG_PRESCALER_128;  // HAL 의 /128 프리스케일러로 매핑합니다.
            break;  // 분주 128 케이스를 종료합니다.
        case 256U:  // 분주 256인 경우입니다.
            ulHALPrescaler = IWDG_PRESCALER_256;  // HAL 의 /256 프리스케일러로 매핑합니다.
            break;  // 분주 256 케이스를 종료합니다.
        default:  // 예상 밖의 분주값인 경우입니다.
            ulHALPrescaler = IWDG_PRESCALER_256;  // 안전하게 /256 으로 매핑합니다.
            break;  // 기본 케이스를 종료합니다.
    }  // 프리스케일러 매핑 switch 블록을 종료합니다.

    s_hiwdg.Instance = IWDG;  // HAL 핸들에 IWDG 인스턴스를 연결합니다.
    s_hiwdg.Init.Prescaler = ulHALPrescaler;  // 매핑한 프리스케일러를 설정합니다.
    s_hiwdg.Init.Reload = ulReload;  // 계산한 리로드 값을 설정합니다.
    (void)HAL_IWDG_Init(&s_hiwdg);  // IWDG 를 초기화하고 동작을 시작합니다.
}  // IWDG 초기화 함수를 종료합니다.

void IWDG_ReloadCounter(void)  // IWDG 피드 함수를 정의합니다.
{  // IWDG 피드 함수 본문을 시작합니다.
    (void)HAL_IWDG_Refresh(&s_hiwdg);  // IWDG 카운터를 리프레시합니다.
}  // IWDG 피드 함수를 종료합니다.

uint32_t IWDG_GetResetFlags(void)  // 리셋 원인 플래그 조회 함수를 정의합니다.
{  // 리셋 원인 조회 함수 본문을 시작합니다.
    return RCC->CSR & 0xFC000000U;  // RCC CSR 레지스터에서 리셋 플래그만 마스킹하여 반환합니다.
}  // 리셋 원인 조회 함수를 종료합니다.

void IWDG_ClearResetFlags(void)  // 리셋 원인 플래그 클리어 함수를 정의합니다.
{  // 리셋 원인 클리어 함수 본문을 시작합니다.
    __HAL_RCC_CLEAR_RESET_FLAGS();  // HAL 매크로로 모든 리셋 플래그를 클리어합니다.
}  // 리셋 원인 클리어 함수를 종료합니다.
