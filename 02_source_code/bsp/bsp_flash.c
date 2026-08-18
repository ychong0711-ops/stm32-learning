// bsp_flash.c — 내부 플래시 BSP 구현 (STM32F4 HAL)입니다.
// 섹터 삭제 / 워드·버퍼 기록 / 소거 확인 / 검증 / 주소→섹터 매핑을 제공합니다.
// 애플리케이션(스테이징 기록)과 부트로더(앱 영역 설치)가 함께 사용합니다.
#include "bsp_flash.h"  // 플래시 BSP 헤더를 포함합니다.
#include "flash_map.h"  // 플래시 전체 맵 상수를 사용하기 위해 포함합니다.

#include "stm32f4xx_hal.h"  // STM32F4 HAL 드라이버 헤더를 포함합니다.

// 장시간 삭제/기록 중 워치독을 먹이기 위한 훅입니다. 등록되지 않으면 호출하지 않습니다.
static void (*s_pfnProgressHook)(void) = 0;  // 진행 콜백 함수 포인터를 NULL 로 초기화합니다.

#define FLASH_PROGRESS_INTERVAL_WORDS 256U  // 몇 워드마다 진행 콜백을 호출할지 정의합니다.

// STM32F446RE(512KB) 섹터 구성: 0~3 = 16KB, 4 = 64KB, 5~7 = 128KB 입니다.
#define FLASH_SECTOR_COUNT 8U  // 이 디바이스의 총 섹터 개수입니다.

static const uint32_t s_ulSectorStart[FLASH_SECTOR_COUNT + 1U] =  // 각 섹터의 시작 주소 표를 정의합니다. (마지막은 끝 주소)
{  // 섹터 시작 주소 표 초기화를 시작합니다.
    0x08000000U,  // 섹터 0 의 시작 주소입니다. (16KB)
    0x08004000U,  // 섹터 1 의 시작 주소입니다. (16KB)
    0x08008000U,  // 섹터 2 의 시작 주소입니다. (16KB)
    0x0800C000U,  // 섹터 3 의 시작 주소입니다. (16KB)
    0x08010000U,  // 섹터 4 의 시작 주소입니다. (64KB)
    0x08020000U,  // 섹터 5 의 시작 주소입니다. (128KB)
    0x08040000U,  // 섹터 6 의 시작 주소입니다. (128KB)
    0x08060000U,  // 섹터 7 의 시작 주소입니다. (128KB)
    0x08080000U   // 플래시 끝(마지막 섹터의 끝 + 1) 주소입니다.
};  // 섹터 시작 주소 표 초기화를 종료합니다.

static void prvProgress(uint32_t ulCounter)  // 일정 주기로 진행 콜백을 호출하는 내부 함수를 정의합니다.
{  // 진행 콜백 호출 함수 본문을 시작합니다.
    if ((s_pfnProgressHook != 0) && ((ulCounter % FLASH_PROGRESS_INTERVAL_WORDS) == 0U))  // 콜백이 있고 주기에 도달했는지 확인합니다.
    {  // 콜백 호출 블록을 시작합니다.
        s_pfnProgressHook();  // 등록된 콜백(예: 워치독 피드)을 호출합니다.
    }  // 콜백 호출 블록을 종료합니다.
}  // 진행 콜백 호출 함수를 종료합니다.

void Flash_SetProgressHook(void (*pfnHook)(void))  // 진행 콜백 등록 함수를 정의합니다.
{  // 진행 콜백 등록 함수 본문을 시작합니다.
    s_pfnProgressHook = pfnHook;  // 전달받은 함수 포인터를 저장합니다.
}  // 진행 콜백 등록 함수를 종료합니다.

void Flash_Init(void)  // 플래시 초기화 함수를 정의합니다.
{  // 플래시 초기화 함수 본문을 시작합니다.
    HAL_FLASH_Unlock();  // 플래시 제어 레지스터를 언락합니다.
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |  // 이전에 남아 있을 수 있는 오류 플래그를 지웁니다.
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);  // 정렬/병렬/시퀀스 오류 플래그까지 모두 지웁니다.
}  // 플래시 초기화 함수를 종료합니다.

void Flash_Deinit(void)  // 플래시 잠금 함수를 정의합니다.
{  // 플래시 잠금 함수 본문을 시작합니다.
    HAL_FLASH_Lock();  // 플래시 제어 레지스터를 다시 잠가 오동작에 의한 기록을 막습니다.
}  // 플래시 잠금 함수를 종료합니다.

uint32_t Flash_GetSector(uint32_t ulAddress)  // 주소가 속한 섹터 번호를 반환하는 함수를 정의합니다.
{  // 주소→섹터 변환 함수 본문을 시작합니다.
    if ((ulAddress < s_ulSectorStart[0]) || (ulAddress >= s_ulSectorStart[FLASH_SECTOR_COUNT]))  // 주소가 플래시 범위 밖인지 확인합니다.
    {  // 범위 밖 처리 블록을 시작합니다.
        return FLASH_INVALID_SECTOR;  // 변환 실패 값을 반환합니다.
    }  // 범위 밖 처리 블록을 종료합니다.

    for (uint32_t ulSector = 0U; ulSector < FLASH_SECTOR_COUNT; ulSector++)  // 모든 섹터를 순회합니다.
    {  // 섹터 탐색 반복문 본문을 시작합니다.
        if (ulAddress < s_ulSectorStart[ulSector + 1U])  // 주소가 다음 섹터 시작보다 작은지 확인합니다.
        {  // 해당 섹터를 찾은 경우 처리 블록을 시작합니다.
            return ulSector;  // 찾은 섹터 번호를 반환합니다.
        }  // 해당 섹터를 찾은 경우 처리 블록을 종료합니다.
    }  // 섹터 탐색 반복문을 종료합니다.

    return FLASH_INVALID_SECTOR;  // 이론상 도달하지 않지만 안전을 위해 실패 값을 반환합니다.
}  // 주소→섹터 변환 함수를 종료합니다.

uint32_t Flash_GetSectorSize(uint32_t ulSector)  // 섹터 크기를 반환하는 함수를 정의합니다.
{  // 섹터 크기 조회 함수 본문을 시작합니다.
    if (ulSector >= FLASH_SECTOR_COUNT)  // 섹터 번호가 범위를 벗어나는지 확인합니다.
    {  // 범위 초과 처리 블록을 시작합니다.
        return 0U;  // 크기 0 을 반환하여 오류를 알립니다.
    }  // 범위 초과 처리 블록을 종료합니다.
    return s_ulSectorStart[ulSector + 1U] - s_ulSectorStart[ulSector];  // 다음 섹터 시작과의 차이를 크기로 반환합니다.
}  // 섹터 크기 조회 함수를 종료합니다.

uint32_t Flash_GetSectorStart(uint32_t ulSector)  // 섹터 시작 주소를 반환하는 함수를 정의합니다.
{  // 섹터 시작 주소 조회 함수 본문을 시작합니다.
    if (ulSector >= FLASH_SECTOR_COUNT)  // 섹터 번호가 범위를 벗어나는지 확인합니다.
    {  // 범위 초과 처리 블록을 시작합니다.
        return 0U;  // 주소 0 을 반환하여 오류를 알립니다.
    }  // 범위 초과 처리 블록을 종료합니다.
    return s_ulSectorStart[ulSector];  // 표에서 시작 주소를 반환합니다.
}  // 섹터 시작 주소 조회 함수를 종료합니다.

BaseType_t Flash_EraseSector(uint32_t ulSector)  // 섹터 삭제 함수를 정의합니다.
{  // 섹터 삭제 함수 본문을 시작합니다.
    FLASH_EraseInitTypeDef sEraseInit;  // HAL 삭제 설정 구조체를 선언합니다.
    uint32_t ulSectorError = 0U;  // 삭제 오류 섹터 번호를 저장할 변수입니다.
    HAL_StatusTypeDef eStatus;  // HAL 반환 상태를 저장할 변수입니다.

    if (ulSector >= FLASH_SECTOR_COUNT)  // 섹터 번호가 유효 범위를 벗어나는지 확인합니다.
    {  // 범위 초과 처리 블록을 시작합니다.
        return pdFAIL;  // 삭제 실패를 반환합니다.
    }  // 범위 초과 처리 블록을 종료합니다.

    if (s_pfnProgressHook != 0)  // 진행 콜백이 등록되어 있는지 확인합니다.
    {  // 삭제 전 콜백 블록을 시작합니다.
        s_pfnProgressHook();  // 128KB 섹터 삭제는 최대 수 초가 걸리므로 직전에 워치독을 먹입니다.
    }  // 삭제 전 콜백 블록을 종료합니다.

    sEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;  // 섹터 단위 삭제로 설정합니다.
    sEraseInit.Banks = FLASH_BANK_1;  // F446 은 단일 뱅크이므로 뱅크 1을 지정합니다.
    sEraseInit.Sector = ulSector;  // 삭제할 시작 섹터를 지정합니다.
    sEraseInit.NbSectors = 1U;  // 삭제할 섹터 개수를 1로 설정합니다.
    sEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;  // 동작 전압 범위를 2.7~3.6V 로 설정합니다. (워드 단위 병렬성)

    __HAL_FLASH_DATA_CACHE_DISABLE();  // 데이터 캐시를 비활성화합니다. (F4 삭제 전 요구사항)
    __HAL_FLASH_INSTRUCTION_CACHE_DISABLE();  // 명령 캐시를 비활성화합니다.

    __HAL_FLASH_DATA_CACHE_RESET();  // 데이터 캐시를 리셋합니다.
    __HAL_FLASH_INSTRUCTION_CACHE_RESET();  // 명령 캐시를 리셋합니다.

    eStatus = HAL_FLASHEx_Erase(&sEraseInit, &ulSectorError);  // 섹터 삭제를 수행합니다. (완료까지 블로킹)

    __HAL_FLASH_DATA_CACHE_ENABLE();  // 데이터 캐시를 다시 활성화합니다.
    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();  // 명령 캐시를 다시 활성화합니다.

    if (s_pfnProgressHook != 0)  // 진행 콜백이 등록되어 있는지 확인합니다.
    {  // 삭제 후 콜백 블록을 시작합니다.
        s_pfnProgressHook();  // 삭제 직후에도 워치독을 먹여 다음 단계까지 여유를 확보합니다.
    }  // 삭제 후 콜백 블록을 종료합니다.

    return ((eStatus == HAL_OK) && (ulSectorError == 0xFFFFFFFFU)) ? pdPASS : pdFAIL;  // HAL 성공이고 오류 섹터가 없으면 성공을 반환합니다.
}  // 섹터 삭제 함수를 종료합니다.

BaseType_t Flash_EraseRange(uint32_t ulAddress, uint32_t ulLength)  // 주소 범위를 덮는 섹터들을 삭제하는 함수를 정의합니다.
{  // 범위 삭제 함수 본문을 시작합니다.
    uint32_t ulFirst;  // 범위의 첫 섹터 번호를 담을 변수입니다.
    uint32_t ulLast;  // 범위의 마지막 섹터 번호를 담을 변수입니다.

    if (ulLength == 0U)  // 삭제 길이가 0 인지 확인합니다.
    {  // 길이 0 처리 블록을 시작합니다.
        return pdPASS;  // 할 일이 없으므로 성공을 반환합니다.
    }  // 길이 0 처리 블록을 종료합니다.

    ulFirst = Flash_GetSector(ulAddress);  // 시작 주소가 속한 섹터를 구합니다.
    ulLast = Flash_GetSector(ulAddress + ulLength - 1U);  // 마지막 바이트가 속한 섹터를 구합니다.

    if ((ulFirst == FLASH_INVALID_SECTOR) || (ulLast == FLASH_INVALID_SECTOR))  // 두 주소 중 하나라도 플래시 밖인지 확인합니다.
    {  // 잘못된 범위 처리 블록을 시작합니다.
        return pdFAIL;  // 삭제 실패를 반환합니다.
    }  // 잘못된 범위 처리 블록을 종료합니다.

    for (uint32_t ulSector = ulFirst; ulSector <= ulLast; ulSector++)  // 범위에 걸친 모든 섹터를 순회합니다.
    {  // 섹터 삭제 반복문 본문을 시작합니다.
        if (Flash_EraseSector(ulSector) != pdPASS)  // 개별 섹터 삭제가 실패했는지 확인합니다.
        {  // 삭제 실패 처리 블록을 시작합니다.
            return pdFAIL;  // 즉시 실패를 반환합니다.
        }  // 삭제 실패 처리 블록을 종료합니다.
    }  // 섹터 삭제 반복문을 종료합니다.

    return pdPASS;  // 모든 섹터 삭제 성공을 반환합니다.
}  // 범위 삭제 함수를 종료합니다.

BaseType_t Flash_ProgramWord(uint32_t ulAddress, uint32_t ulData)  // 워드 기록 함수를 정의합니다.
{  // 워드 기록 함수 본문을 시작합니다.
    HAL_StatusTypeDef eStatus;  // HAL 기록 상태를 저장할 변수입니다.

    if ((ulAddress & 0x3U) != 0U)  // 주소가 4바이트 정렬되지 않았는지 확인합니다.
    {  // 정렬 오류 처리 블록을 시작합니다.
        return pdFAIL;  // 워드 기록은 정렬이 필수이므로 실패를 반환합니다.
    }  // 정렬 오류 처리 블록을 종료합니다.

    eStatus = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, ulAddress, (uint64_t)ulData);  // 지정 주소에 32bit 워드를 기록합니다.

    if (eStatus != HAL_OK)  // 기록이 실패했는지 확인합니다.
    {  // 기록 실패 처리 블록을 시작합니다.
        return pdFAIL;  // 실패를 반환합니다.
    }  // 기록 실패 처리 블록을 종료합니다.

    return (*(volatile uint32_t *)ulAddress == ulData) ? pdPASS : pdFAIL;  // 즉시 되읽어 기록 결과를 확인합니다.
}  // 워드 기록 함수를 종료합니다.

BaseType_t Flash_ProgramBuffer(uint32_t ulAddress, const void *pvData, uint32_t ulLength)  // 버퍼 기록 함수를 정의합니다.
{  // 버퍼 기록 함수 본문을 시작합니다.
    const uint8_t *pucSrc = (const uint8_t *)pvData;  // 바이트 단위 접근을 위해 원본 포인터를 변환합니다.
    uint32_t ulWords;  // 기록할 워드 개수를 담을 변수입니다.

    if ((pvData == 0) || ((ulAddress & 0x3U) != 0U) || ((ulLength & 0x3U) != 0U))  // 포인터/정렬/길이 조건을 확인합니다.
    {  // 인자 오류 처리 블록을 시작합니다.
        return pdFAIL;  // 실패를 반환합니다.
    }  // 인자 오류 처리 블록을 종료합니다.

    ulWords = ulLength / 4U;  // 전체 길이를 워드 개수로 변환합니다.

    for (uint32_t i = 0U; i < ulWords; i++)  // 모든 워드를 순회합니다.
    {  // 워드 기록 반복문 본문을 시작합니다.
        uint32_t ulWord;  // 기록할 32비트 값을 담을 변수입니다.

        // 원본이 4바이트 정렬이 아닐 수 있으므로 바이트를 모아 워드를 구성합니다. (리틀엔디언)
        ulWord = (uint32_t)pucSrc[(i * 4U) + 0U];  // 최하위 바이트를 채웁니다.
        ulWord |= (uint32_t)pucSrc[(i * 4U) + 1U] << 8;  // 두 번째 바이트를 채웁니다.
        ulWord |= (uint32_t)pucSrc[(i * 4U) + 2U] << 16;  // 세 번째 바이트를 채웁니다.
        ulWord |= (uint32_t)pucSrc[(i * 4U) + 3U] << 24;  // 최상위 바이트를 채웁니다.

        if (Flash_ProgramWord(ulAddress + (i * 4U), ulWord) != pdPASS)  // 워드 기록이 실패했는지 확인합니다.
        {  // 기록 실패 처리 블록을 시작합니다.
            return pdFAIL;  // 즉시 실패를 반환합니다.
        }  // 기록 실패 처리 블록을 종료합니다.

        prvProgress(i + 1U);  // 일정 워드마다 진행 콜백(워치독 피드)을 호출합니다.
    }  // 워드 기록 반복문을 종료합니다.

    return pdPASS;  // 전체 기록 성공을 반환합니다.
}  // 버퍼 기록 함수를 종료합니다.

BaseType_t Flash_IsErased(uint32_t ulAddress, uint32_t ulLength)  // 소거 상태 확인 함수를 정의합니다.
{  // 소거 확인 함수 본문을 시작합니다.
    const uint8_t *pucFlash = (const uint8_t *)ulAddress;  // 플래시를 바이트 배열로 해석합니다.

    for (uint32_t i = 0U; i < ulLength; i++)  // 지정 길이만큼 반복합니다.
    {  // 소거 확인 반복문 본문을 시작합니다.
        if (pucFlash[i] != 0xFFU)  // 소거 상태가 아닌 바이트를 찾았는지 확인합니다.
        {  // 미소거 바이트 처리 블록을 시작합니다.
            return pdFAIL;  // 소거 상태가 아님을 반환합니다.
        }  // 미소거 바이트 처리 블록을 종료합니다.

        prvProgress(i + 1U);  // 긴 영역 확인 중에도 주기적으로 워치독을 먹입니다.
    }  // 소거 확인 반복문을 종료합니다.

    return pdPASS;  // 전체가 소거 상태임을 반환합니다.
}  // 소거 확인 함수를 종료합니다.

BaseType_t Flash_Verify(uint32_t ulAddress, const void *pvData, uint32_t ulLength)  // 기록 검증 함수를 정의합니다.
{  // 기록 검증 함수 본문을 시작합니다.
    const uint8_t *pucFlash = (const uint8_t *)ulAddress;  // 플래시 측을 바이트 배열로 해석합니다.
    const uint8_t *pucSrc = (const uint8_t *)pvData;  // 원본 측을 바이트 배열로 해석합니다.

    if (pvData == 0)  // 원본 포인터가 NULL 인지 확인합니다.
    {  // NULL 처리 블록을 시작합니다.
        return pdFAIL;  // 검증 실패를 반환합니다.
    }  // NULL 처리 블록을 종료합니다.

    for (uint32_t i = 0U; i < ulLength; i++)  // 지정 길이만큼 반복합니다.
    {  // 검증 반복문 본문을 시작합니다.
        if (pucFlash[i] != pucSrc[i])  // 플래시 내용과 원본이 다른지 확인합니다.
        {  // 불일치 처리 블록을 시작합니다.
            return pdFAIL;  // 검증 실패를 반환합니다.
        }  // 불일치 처리 블록을 종료합니다.

        prvProgress(i + 1U);  // 긴 검증 중에도 주기적으로 워치독을 먹입니다.
    }  // 검증 반복문을 종료합니다.

    return pdPASS;  // 전체 일치를 반환합니다.
}  // 기록 검증 함수를 종료합니다.
