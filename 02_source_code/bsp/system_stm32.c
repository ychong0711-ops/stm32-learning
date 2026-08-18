// system_stm32.c — 시스템 설정 래퍼 구현 (클록/NVIC)입니다.
// HSE 8MHz → PLL → 180MHz 시스템 클록을 설정합니다.
#include "system_stm32.h"  // 시스템 설정 헤더를 포함합니다.

#include "stm32f4xx_hal.h"  // STM32F4 HAL 드라이버 헤더를 포함합니다.

void SystemClock_Config(void)  // 시스템 클록 설정 함수를 정의합니다.
{  // 시스템 클록 설정 함수 본문을 시작합니다.
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};  // 오실레이터 설정 구조체를 0으로 선언합니다.
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};  // 클록 설정 구조체를 0으로 선언합니다.

    __HAL_RCC_PWR_CLK_ENABLE();  // PWR 클록을 활성화합니다.
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);  // 180MHz 동작을 위해 전압 스케일 1 로 설정합니다.

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;  // 오실레이터로 HSE 를 사용합니다.
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;  // HSE 를 켭니다.
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;  // PLL 을 활성화합니다.
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;  // PLL 입력으로 HSE 를 선택합니다.
    RCC_OscInitStruct.PLL.PLLM = 8;  // PLLM=8 → VCO 입력 1MHz (8MHz/8) 를 만듭니다.
    RCC_OscInitStruct.PLL.PLLN = 180;  // PLLN=180 → VCO 180MHz 를 만듭니다.
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;  // PLLP=/2 → SYSCLK 180MHz 를 만듭니다.
    RCC_OscInitStruct.PLL.PLLQ = 5;  // PLLQ=5 → 48MHz 클록을 위한 분주입니다. (필요 시 조정)
    (void)HAL_RCC_OscConfig(&RCC_OscInitStruct);  // 오실레이터/PLL 설정을 적용합니다.

    // [필수] 오버드라이브 모드를 활성화합니다.
    // RM0390 에 따르면 전압 스케일 1(VOS=0b11) 에서 보장되는 최대 HCLK 는 168MHz 이며,
    // 180MHz 는 오버드라이브 모드를 켜야만 규격 안에 들어옵니다. 이 호출이 없으면
    // PLL 은 180MHz 를 만들어 내지만 전압 레귤레이터가 그 주파수를 뒷받침하지 못해
    // 데이터시트 규격을 벗어난 상태로 동작합니다. (상온에서는 대체로 돌아가지만
    // 고온/저전압에서 간헐적 오동작이나 플래시 액세스 오류로 나타납니다)
    //
    // 호출 위치가 중요합니다. RM0390 의 진입 절차는 "시스템 클록이 HSI/HSE 인 동안
    // PLL 을 켠 뒤, 오버드라이브를 켜고, 그 다음에 PLL 로 전환"입니다. 따라서
    // HAL_RCC_OscConfig() 뒤, HAL_RCC_ClockConfig() 앞이라는 이 자리여야 합니다.
    (void)HAL_PWREx_EnableOverDrive();  // 180MHz 동작을 위해 오버드라이브 모드를 켭니다.

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |  // 설정할 클록 종류를 선택합니다. (HCLK, SYSCLK)
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;  // 설정할 클록 종류를 선택합니다. (PCLK1, PCLK2)
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;  // 시스템 클록 소스를 PLL 로 설정합니다.
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;  // AHB 분주를 1로 설정합니다. (HCLK=180MHz)
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  // APB1 분주를 4로 설정합니다. (PCLK1=45MHz)
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  // APB2 분주를 2로 설정합니다. (PCLK2=90MHz)
    (void)HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);  // 클록 설정을 적용하고 플래시 대기를 5로 설정합니다.
}  // 시스템 클록 설정 함수를 종료합니다.

void NVIC_PriorityGroupConfig(uint32_t ulPriorityGroup)  // NVIC 우선순위 그룹 설정 함수를 정의합니다.
{  // NVIC 우선순위 그룹 설정 함수 본문을 시작합니다.
    HAL_NVIC_SetPriorityGrouping(ulPriorityGroup);  // HAL API 로 우선순위 그룹을 설정합니다.
}  // NVIC 우선순위 그룹 설정 함수를 종료합니다.
