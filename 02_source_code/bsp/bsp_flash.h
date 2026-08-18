// bsp_flash.h — 내부 플래시 BSP 헤더 (STM32F4 HAL 기반)입니다.
// 애플리케이션은 스테이징 영역 기록에, 부트로더는 앱 영역 삭제/기록에 사용합니다.
// 부트로더는 FreeRTOS 없이 동작하므로 BSP_BAREMETAL 정의 시 커널 헤더 없이 컴파일됩니다.
#ifndef BSP_FLASH_H  // BSP_FLASH_H 가 아직 정의되지 않았는지 확인합니다.
#define BSP_FLASH_H  // BSP_FLASH_H 를 정의하여 중복 포함을 방지합니다.

#include <stdint.h>    // uint32_t 등 고정 폭 정수 타입을 사용하기 위해 포함합니다.

#ifdef BSP_BAREMETAL  // 부트로더처럼 FreeRTOS 가 없는 빌드인지 확인합니다.
    #include "bsp_baremetal.h"  // BaseType_t/pdPASS/pdFAIL 의 최소 정의를 포함합니다.
#else  // FreeRTOS 가 있는 애플리케이션 빌드인 경우입니다.
    #include "FreeRTOS.h"  // BaseType_t 와 pdPASS/pdFAIL 을 사용하기 위해 포함합니다.
#endif  // 빌드 종류 분기를 종료합니다.

#define FLASH_INVALID_SECTOR 0xFFFFFFFFU  // 주소→섹터 변환 실패를 나타내는 값입니다.

void Flash_Init(void);  // 플래시 모듈을 초기화(언락)하는 함수 프로토타입입니다.
void Flash_Deinit(void);  // 플래시 제어 레지스터를 다시 잠그는 함수 프로토타입입니다.

uint32_t Flash_GetSector(uint32_t ulAddress);  // 주소가 속한 섹터 번호를 반환하는 함수 프로토타입입니다.
uint32_t Flash_GetSectorSize(uint32_t ulSector);  // 섹터의 바이트 크기를 반환하는 함수 프로토타입입니다.
uint32_t Flash_GetSectorStart(uint32_t ulSector);  // 섹터의 시작 주소를 반환하는 함수 프로토타입입니다.

BaseType_t Flash_EraseSector(uint32_t ulSector);  // 지정 섹터를 삭제하는 함수 프로토타입입니다.
BaseType_t Flash_EraseRange(uint32_t ulAddress, uint32_t ulLength);  // 주소 범위를 덮는 섹터들을 삭제하는 함수 프로토타입입니다.
BaseType_t Flash_ProgramWord(uint32_t ulAddress, uint32_t ulData);  // 지정 주소에 32bit 워드를 기록하는 함수 프로토타입입니다.
BaseType_t Flash_ProgramBuffer(uint32_t ulAddress, const void *pvData, uint32_t ulLength);  // 버퍼를 워드 단위로 기록하는 함수 프로토타입입니다.
BaseType_t Flash_IsErased(uint32_t ulAddress, uint32_t ulLength);  // 범위가 소거 상태(0xFF)인지 확인하는 함수 프로토타입입니다.
BaseType_t Flash_Verify(uint32_t ulAddress, const void *pvData, uint32_t ulLength);  // 기록 결과를 원본과 비교하는 함수 프로토타입입니다.

void Flash_SetProgressHook(void (*pfnHook)(void));  // 장시간 작업 중 주기적으로 호출할 콜백(워치독 피드 등)을 등록합니다.

#endif /* BSP_FLASH_H */  // BSP_FLASH_H 조건부 컴파일 블록을 종료합니다.
