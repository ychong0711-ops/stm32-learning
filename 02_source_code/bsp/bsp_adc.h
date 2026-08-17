// bsp_adc.h — ADC BSP 헤더 (STM32F4 HAL 기반)입니다.
// 사용 ADC 는 ADC1 이며, 폴링 방식의 단일 변환으로 값을 읽습니다.
#ifndef BSP_ADC_H  // BSP_ADC_H 가 아직 정의되지 않았는지 확인합니다.
#define BSP_ADC_H  // BSP_ADC_H 를 정의하여 중복 포함을 방지합니다.

#include <stdint.h>  // uint16_t, uint32_t 등 고정 폭 정수 타입을 사용하기 위해 포함합니다.

#define BSP_ADC_CHANNEL_RPM       0U  // RPM 센서의 논리 채널 번호를 0으로 정의합니다.
#define BSP_ADC_CHANNEL_THROTTLE  1U  // 스로틀 센서의 논리 채널 번호를 1로 정의합니다.

void ADC_Init(void);  // ADC 주변장치를 초기화하는 함수 프로토타입입니다.
uint16_t ADC_ReadChannel(uint32_t ulChannel);  // 지정 채널을 읽어 12bit 값을 반환하는 함수 프로토타입입니다.

#endif /* BSP_ADC_H */  // BSP_ADC_H 조건부 컴파일 블록을 종료합니다.
