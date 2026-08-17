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
