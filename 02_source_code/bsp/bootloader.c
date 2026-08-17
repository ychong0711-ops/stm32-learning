// bootloader.c — 부트로더 인터페이스 구현 (골격)입니다.
// 업데이트 요청 플래그를 백업 SRAM 에 저장하고, 플래싱을 수행합니다.
#include "bootloader.h"  // 부트로더 인터페이스 헤더를 포함합니다.

#include "stm32f4xx_hal.h"  // STM32F4 HAL 드라이버 헤더를 포함합니다.
#include "bsp_flash.h"  // 내부 플래시 API 를 사용하기 위해 포함합니다.
#include "bsp_iwdg.h"  // 플래싱 중 워치독 피드를 위해 포함합니다.

#define BKPSRAM_BASE_ADDR   0x40024000U  // 백업 SRAM 의 베이스 주소를 정의합니다.
#define UPDATE_FLAG_ADDR    (BKPSRAM_BASE_ADDR + 0x00U)  // 업데이트 플래그가 저장될 주소를 정의합니다.

static const uint32_t s_ulAppSectors[] = { FLASH_SECTOR_1, FLASH_SECTOR_2,  // 앱 영역 섹터 1, 2 를 정의합니다.
                                           FLASH_SECTOR_3, FLASH_SECTOR_4 };  // 앱 영역 섹터 3, 4 를 정의합니다.
#define APP_SECTOR_COUNT  (sizeof(s_ulAppSectors) / sizeof(s_ulAppSectors[0]))  // 앱 섹터 개수를 계산하는 매크로입니다.

static const uint32_t s_ulFirmwareImage[] = { 0xDEADBEEFU };  // 새 펌웨어 이미지(골격용 더미)를 정의합니다.
#define FIRMWARE_IMAGE_SIZE  (sizeof(s_ulFirmwareImage) / sizeof(s_ulFirmwareImage[0]))  // 이미지 워드 수를 계산하는 매크로입니다.

void Bootloader_Init(void)  // 부트로더 초기화 함수를 정의합니다.
{  // 부트로더 초기화 함수 본문을 시작합니다.
    __HAL_RCC_BKPSRAM_CLK_ENABLE();  // 백업 SRAM 클록을 활성화합니다.
    HAL_PWR_EnableBkUpAccess();  // 백업 도메인 접근을 허용합니다.
}  // 부트로더 초기화 함수를 종료합니다.

FirmwareState_t Bootloader_CheckUpdateRequest(void)  // 업데이트 요청 확인 함수를 정의합니다.
{  // 업데이트 요청 확인 함수 본문을 시작합니다.
    volatile uint32_t *pFlag = (volatile uint32_t *)UPDATE_FLAG_ADDR;  // 플래그 주소를 포인터로 변환합니다.
    return (*pFlag == BOOTLOADER_UPDATE_MAGIC) ? UPDATE_AVAILABLE : UPDATE_NONE;  // 매직 값 일치 여부로 상태를 반환합니다.
}  // 업데이트 요청 확인 함수를 종료합니다.

void Bootloader_SetUpdateRequest(void)  // 업데이트 요청 설정 함수를 정의합니다.
{  // 업데이트 요청 설정 함수 본문을 시작합니다.
    volatile uint32_t *pFlag = (volatile uint32_t *)UPDATE_FLAG_ADDR;  // 플래그 주소를 포인터로 변환합니다.
    *pFlag = BOOTLOADER_UPDATE_MAGIC;  // 매직 값을 기록하여 요청을 설정합니다.
}  // 업데이트 요청 설정 함수를 종료합니다.

void Bootloader_ClearUpdateRequest(void)  // 업데이트 요청 해제 함수를 정의합니다.
{  // 업데이트 요청 해제 함수 본문을 시작합니다.
    volatile uint32_t *pFlag = (volatile uint32_t *)UPDATE_FLAG_ADDR;  // 플래그 주소를 포인터로 변환합니다.
    *pFlag = 0U;  // 0을 기록하여 요청을 해제합니다.
}  // 업데이트 요청 해제 함수를 종료합니다.

void Bootloader_FlashNewFirmware(void)  // 새 펌웨어 플래싱 함수를 정의합니다.
{  // 플래싱 함수 본문을 시작합니다.
    for (uint32_t i = 0U; i < APP_SECTOR_COUNT; i++)  // 앱 영역의 모든 섹터를 순회합니다.
    {  // 섹터 삭제 반복문 본문을 시작합니다.
        IWDG_ReloadCounter();  // 삭제 전에 워치독을 피드합니다. (16KB 삭제는 수십~수백 ms 소요)
        (void)Flash_EraseSector(s_ulAppSectors[i]);  // 현재 섹터를 삭제합니다.
    }  // 섹터 삭제 반복문을 종료합니다.

    for (uint32_t i = 0U; i < FIRMWARE_IMAGE_SIZE; i++)  // 이미지의 모든 워드를 순회합니다.
    {  // 이미지 기록 반복문 본문을 시작합니다.
        uint32_t ulAddr = BOOTLOADER_APP_START_ADDR + (i * 4U);  // 기록할 플래시 주소를 계산합니다.
        (void)Flash_ProgramWord(ulAddr, s_ulFirmwareImage[i]);  // 현재 워드를 기록합니다.

        if ((i % 256U) == 0U)  // 256워드(1KB)마다인지 확인합니다.
        {  // 주기적 피드 블록을 시작합니다.
            IWDG_ReloadCounter();  // 1KB 단위로 워치독을 피드합니다.
        }  // 주기적 피드 블록을 종료합니다.
    }  // 이미지 기록 반복문을 종료합니다.

    IWDG_ReloadCounter();  // 기록 완료 후 마지막으로 워치독을 피드합니다.
}  // 플래싱 함수를 종료합니다.
