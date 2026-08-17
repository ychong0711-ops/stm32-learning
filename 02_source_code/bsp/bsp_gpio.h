// bsp_gpio.h — GPIO BSP 헤더 (STM32F4 HAL 기반)입니다.
// 논리 핀 ID 를 물리 핀(포트+핀)에 매핑하는 테이블 기반 구현을 사용합니다.
#ifndef BSP_GPIO_H  // BSP_GPIO_H 가 아직 정의되지 않았는지 확인합니다.
#define BSP_GPIO_H  // BSP_GPIO_H 를 정의하여 중복 포함을 방지합니다.

#include <stdint.h>  // uint8_t, uint32_t 등 고정 폭 정수 타입을 사용하기 위해 포함합니다.

#define BSP_GPIO_PIN_FAN          13U  // 팬 제어 핀의 논리 ID 를 13으로 정의합니다. (main_fixed.c 의 GPIO_PIN_FAN 과 동일)
#define BSP_GPIO_PIN_MEAS_JITTER  20U  // 주기 지터 측정용 토글 핀의 논리 ID 를 20으로 정의합니다.
#define BSP_GPIO_PIN_MEAS_E2E     21U  // 긴급정지 E2E 응답 측정용 토글 핀의 논리 ID 를 21로 정의합니다.
#define BSP_GPIO_PIN_LED_ERR      22U  // 오류 표시 LED 핀의 논리 ID 를 22로 정의합니다.

void GPIO_Init(uint32_t ulPin);  // 지정 논리 핀을 출력으로 초기화하는 함수 프로토타입입니다.
void GPIO_Write(uint32_t ulPin, uint8_t ucState);  // 지정 논리 핀에 상태를 출력하는 함수 프로토타입입니다.
void GPIO_Toggle(uint32_t ulPin);  // 지정 논리 핀의 출력을 토글하는 함수 프로토타입입니다.

#endif /* BSP_GPIO_H */  // BSP_GPIO_H 조건부 컴파일 블록을 종료합니다.
