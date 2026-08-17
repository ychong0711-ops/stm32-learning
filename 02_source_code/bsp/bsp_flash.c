// bsp_flash.c — 내부 플래시 BSP 구현 (STM32F4 HAL)입니다.
// 내부 플래시의 섹터 삭제와 32bit 워드 기록을 담당합니다.
#include "bsp_flash.h"  // 플래시 BSP 헤더를 포함합니다.

#include "stm32f4xx_hal.h"  // STM32F4 HAL 드라이버 헤더를 포함합니다.

void Flash_Init(void)  // 플래시 초기화 함수를 정의합니다.
{  // 플래시 초기화 함수 본문을 시작합니다.
    HAL_FLASH_Unlock();  // 플래시 제어 레지스터를 언락합니다.
}  // 플래시 초기화 함수를 종료합니다.

BaseType_t Flash_EraseSector(uint32_t ulSector)  // 섹터 삭제 함수를 정의합니다.
{  // 섹터 삭제 함수 본문을 시작합니다.
    FLASH_EraseInitTypeDef sEraseInit;  // HAL 삭제 설정 구조체를 선언합니다.
    uint32_t ulSectorError = 0U;  // 삭제 오류 섹터 번호를 저장할 변수입니다.

    sEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;  // 섹터 단위 삭제로 설정합니다.
    sEraseInit.Sector = ulSector;  // 삭제할 시작 섹터를 지정합니다.
    sEraseInit.NbSectors = 1;  // 삭제할 섹터 개수를 1로 설정합니다.
    sEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;  // 동작 전압 범위를 2.7~3.6V 로 설정합니다.

    __HAL_FLASH_DATA_CACHE_DISABLE();  // 데이터 캐시를 비활성화합니다. (F4 삭제 전 요구사항)
    __HAL_FLASH_INSTRUCTION_CACHE_DISABLE();  // 명령 캐시를 비활성화합니다.

    __HAL_FLASH_DATA_CACHE_RESET();  // 데이터 캐시를 리셋합니다.
    __HAL_FLASH_INSTRUCTION_CACHE_RESET();  // 명령 캐시를 리셋합니다.

    HAL_StatusTypeDef eStatus = HAL_FLASHEx_Erase(&sEraseInit, &ulSectorError);  // 섹터 삭제를 수행합니다.

    __HAL_FLASH_DATA_CACHE_ENABLE();  // 데이터 캐시를 다시 활성화합니다.
    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();  // 명령 캐시를 다시 활성화합니다.

    return (eStatus == HAL_OK) ? pdPASS : pdFAIL;  // 삭제 성공 여부를 반환합니다.
}  // 섹터 삭제 함수를 종료합니다.

BaseType_t Flash_ProgramWord(uint32_t ulAddress, uint32_t ulData)  // 워드 기록 함수를 정의합니다.
{  // 워드 기록 함수 본문을 시작합니다.
    HAL_StatusTypeDef eStatus =  // HAL 기록 상태를 저장할 변수를 선언합니다.
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, ulAddress, ulData);  // 지정 주소에 32bit 워드를 기록합니다.

    return (eStatus == HAL_OK) ? pdPASS : pdFAIL;  // 기록 성공 여부를 반환합니다.
}  // 워드 기록 함수를 종료합니다.
