// bsp_uart.h — UART BSP 헤더 (STM32F4 HAL 기반)입니다.
// printf() 리타겟을 통해 USART2 로 디버그 출력을 보냅니다.
#ifndef BSP_UART_H  // BSP_UART_H 가 아직 정의되지 않았는지 확인합니다.
#define BSP_UART_H  // BSP_UART_H 를 정의하여 중복 포함을 방지합니다.

#include <stdint.h>  // uint32_t 등 고정 폭 정수 타입을 사용하기 위해 포함합니다.

#define BSP_UART_BAUD_115200  115200U  // UART 보레이트 115200 을 정의합니다. (main_fixed.c 의 UART_BAUD_115200 과 동일)

void UART_Init(uint32_t ulBaudrate);  // 디버깅용 UART 를 지정 보레이트로 초기화하는 함수 프로토타입입니다.

#endif /* BSP_UART_H */  // BSP_UART_H 조건부 컴파일 블록을 종료합니다.
