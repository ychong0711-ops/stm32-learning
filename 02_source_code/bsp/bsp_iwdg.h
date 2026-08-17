// bsp_iwdg.h — 독립 워치독(IWDG) BSP 헤더 (STM32F4 HAL 기반)입니다.
// IWDG 는 LSI(약 32kHz)로 동작하며 한번 시작하면 멈출 수 없습니다.
#ifndef BSP_IWDG_H  // BSP_IWDG_H 가 아직 정의되지 않았는지 확인합니다.
#define BSP_IWDG_H  // BSP_IWDG_H 를 정의하여 중복 포함을 방지합니다.

#include <stdint.h>  // uint32_t 등 고정 폭 정수 타입을 사용하기 위해 포함합니다.

void IWDG_Init(uint32_t ulTimeoutMs);  // IWDG 를 지정 타임아웃으로 초기화하고 시작하는 함수 프로토타입입니다.
void IWDG_ReloadCounter(void);  // IWDG 카운터를 리프레시(피드)하는 함수 프로토타입입니다.
uint32_t IWDG_GetResetFlags(void);  // 마지막 리셋의 원인 플래그를 반환하는 함수 프로토타입입니다.
void IWDG_ClearResetFlags(void);  // 리셋 원인 플래그를 클리어하는 함수 프로토타입입니다.

#endif /* BSP_IWDG_H */  // BSP_IWDG_H 조건부 컴파일 블록을 종료합니다.
