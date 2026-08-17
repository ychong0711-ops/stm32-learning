// bsp.h — BSP(Board Support Package) 통합 헤더 파일입니다.
// main_fixed.c 에서 사용하는 모든 BSP 함수를 한 번에 포함할 수 있게 해줍니다.
#ifndef BSP_H  // BSP_H 가 아직 정의되지 않았는지 확인합니다.
#define BSP_H  // BSP_H 를 정의하여 이 헤더의 중복 포함을 방지합니다.

#include "bsp_can.h"        // CAN 통신 BSP 함수 선언 헤더를 포함합니다.
#include "bsp_adc.h"        // ADC 변환 BSP 함수 선언 헤더를 포함합니다.
#include "bsp_pwm.h"        // PWM 출력 BSP 함수 선언 헤더를 포함합니다.
#include "bsp_gpio.h"       // GPIO 제어 BSP 함수 선언 헤더를 포함합니다.
#include "bsp_uart.h"       // UART 통신 BSP 함수 선언 헤더를 포함합니다.
#include "bsp_iwdg.h"       // 독립 워치독(IWDG) BSP 함수 선언 헤더를 포함합니다.
#include "bsp_flash.h"      // 내부 플래시 BSP 함수 선언 헤더를 포함합니다.
#include "bootloader.h"     // 부트로더 인터페이스 선언 헤더를 포함합니다.
#include "sensor_bmp280.h"  // BMP280 센서 함수 선언 헤더를 포함합니다.
#include "system_stm32.h"   // 시스템 클록 및 NVIC 설정 함수 선언 헤더를 포함합니다.

#endif /* BSP_H */  // BSP_H 조건부 컴파일 블록을 종료합니다.
