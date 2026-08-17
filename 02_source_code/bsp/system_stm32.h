// system_stm32.h — 시스템 설정 래퍼 헤더 (클록/NVIC)입니다.
// 180MHz 시스템 클록 설정과 NVIC 우선순위 그룹 설정을 담당합니다.
#ifndef SYSTEM_STM32_H  // SYSTEM_STM32_H 가 아직 정의되지 않았는지 확인합니다.
#define SYSTEM_STM32_H  // SYSTEM_STM32_H 를 정의하여 중복 포함을 방지합니다.

#include <stdint.h>  // uint32_t 등 고정 폭 정수 타입을 사용하기 위해 포함합니다.

void SystemClock_Config(void);  // 시스템 클록을 180MHz 로 설정하는 함수 프로토타입입니다.
void NVIC_PriorityGroupConfig(uint32_t ulPriorityGroup);  // NVIC 우선순위 그룹을 설정하는 함수 프로토타입입니다.

#endif /* SYSTEM_STM32_H */  // SYSTEM_STM32_H 조건부 컴파일 블록을 종료합니다.
