// bsp_pwm.h — PWM BSP 헤더 (STM32F4 HAL 기반)입니다.
// 사용 타이머는 TIM1 이며, 듀티는 0~1000 퍼밀(0.1% 단위)로 표현합니다.
#ifndef BSP_PWM_H  // BSP_PWM_H 가 아직 정의되지 않았는지 확인합니다.
#define BSP_PWM_H  // BSP_PWM_H 를 정의하여 중복 포함을 방지합니다.

#include <stdint.h>  // uint32_t 등 고정 폭 정수 타입을 사용하기 위해 포함합니다.

#define BSP_PWM_CHANNEL_THROTTLE  0U  // 스로틀 PWM 의 논리 채널 번호를 0으로 정의합니다.

void PWM_Init(uint32_t ulChannel, uint32_t ulFrequencyHz);  // PWM 채널을 지정 주파수로 초기화하는 함수 프로토타입입니다.
void PWM_SetDuty(uint32_t ulChannel, uint32_t ulDutyPermille);  // 지정 채널의 듀티를 설정하는 함수 프로토타입입니다.

#endif /* BSP_PWM_H */  // BSP_PWM_H 조건부 컴파일 블록을 종료합니다.
