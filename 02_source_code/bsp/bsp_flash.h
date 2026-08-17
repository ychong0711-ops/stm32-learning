// bsp_flash.h — 내부 플래시 BSP 헤더 (STM32F4 HAL 기반)입니다.
// 부트로더 자체 업데이트 시나리오에서 앱 영역 삭제/기록에 사용합니다.
#ifndef BSP_FLASH_H  // BSP_FLASH_H 가 아직 정의되지 않았는지 확인합니다.
#define BSP_FLASH_H  // BSP_FLASH_H 를 정의하여 중복 포함을 방지합니다.

#include <stdint.h>    // uint32_t 등 고정 폭 정수 타입을 사용하기 위해 포함합니다.
#include "FreeRTOS.h"  // BaseType_t 를 사용하기 위해 포함합니다.

void Flash_Init(void);  // 플래시 모듈을 초기화(언락)하는 함수 프로토타입입니다.
BaseType_t Flash_EraseSector(uint32_t ulSector);  // 지정 섹터를 삭제하는 함수 프로토타입입니다.
BaseType_t Flash_ProgramWord(uint32_t ulAddress, uint32_t ulData);  // 지정 주소에 32bit 워드를 기록하는 함수 프로토타입입니다.

#endif /* BSP_FLASH_H */  // BSP_FLASH_H 조건부 컴파일 블록을 종료합니다.
