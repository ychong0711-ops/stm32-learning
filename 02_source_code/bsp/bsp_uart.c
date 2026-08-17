// bsp_uart.c — UART BSP 구현 (USART2, printf 리타겟)입니다.
// USART2(PA2/PA3)로 newlib 의 _write() 를 연결하여 printf 출력을 보냅니다.
#include "bsp_uart.h"  // UART BSP 헤더를 포함합니다.

#include "stm32f4xx_hal.h"  // STM32F4 HAL 드라이버 헤더를 포함합니다.

static UART_HandleTypeDef s_huart2;  // USART2 의 HAL 핸들 구조체입니다.

void UART_Init(uint32_t ulBaudrate)  // UART 초기화 함수를 정의합니다.
{  // UART 초기화 함수 본문을 시작합니다.
    GPIO_InitTypeDef GPIO_InitStruct = {0};  // GPIO 초기화 구조체를 0으로 선언합니다.

    __HAL_RCC_GPIOA_CLK_ENABLE();  // GPIOA 클록을 활성화합니다.
    __HAL_RCC_USART2_CLK_ENABLE();  // USART2 클록을 활성화합니다.

    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;  // PA2(TX), PA3(RX) 핀을 선택합니다.
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;  // 대체 기능 푸시풀 모드로 설정합니다.
    GPIO_InitStruct.Pull = GPIO_NOPULL;  // 내부 풀업/풀다운을 사용하지 않습니다.
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;  // 핀 속도를 최고속으로 설정합니다.
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;  // 대체 기능을 AF7(USART2)로 설정합니다.
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);  // 설정을 GPIOA 에 적용합니다.

    s_huart2.Instance = USART2;  // HAL 핸들에 USART2 인스턴스를 연결합니다.
    s_huart2.Init.BaudRate = ulBaudrate;  // 보레이트를 인자 값으로 설정합니다.
    s_huart2.Init.WordLength = UART_WORDLENGTH_8B;  // 데이터 길이를 8bit 로 설정합니다.
    s_huart2.Init.StopBits = UART_STOPBITS_1;  // 스톱 비트를 1bit 로 설정합니다.
    s_huart2.Init.Parity = UART_PARITY_NONE;  // 패리티를 사용하지 않습니다.
    s_huart2.Init.Mode = UART_MODE_TX_RX;  // 송수신 겸용 모드로 설정합니다.
    s_huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;  // 하드웨어 흐름 제어를 사용하지 않습니다.
    s_huart2.Init.OverSampling = UART_OVERSAMPLING_16;  // 오버샘플링을 16배로 설정합니다.
    (void)HAL_UART_Init(&s_huart2);  // USART2 를 초기화합니다.
}  // UART 초기화 함수를 종료합니다.

#if defined(__GNUC__)  // GCC 계열 컴파일러에서만 아래 _write 재정의를 활성화합니다.
int _write(int file, char *ptr, int len)  // newlib 의 _write 를 재정의하는 함수를 정의합니다.
{  // _write 함수 본문을 시작합니다.
    (void)file;  // 표준 출력 파일 디스크립터는 사용하지 않으므로 무시합니다.
    HAL_UART_Transmit(&s_huart2, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY);  // 데이터를 USART2 로 블로킹 전송합니다.
    return len;  // 전송한 바이트 수를 반환합니다.
}  // _write 함수를 종료합니다.
#endif /* __GNUC__ */  // GCC 조건부 컴파일 블록을 종료합니다.
