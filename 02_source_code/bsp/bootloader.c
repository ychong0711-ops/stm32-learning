// bootloader.c — 애플리케이션 측 부트로더 인터페이스(업데이트 클라이언트) 구현입니다.
//
// 이 파일은 "앱이 실행 중에 할 수 있는 일"만 담당합니다.
//   1) 수신한 새 이미지를 스테이징 영역(섹터 5)에 기록하고 CRC 로 검증합니다.
//   2) 검증에 성공하면 백업 SRAM 의 부트 제어 블록에 설치 요청을 남깁니다.
//   3) 시스템을 리셋합니다.
// 앱 영역(섹터 1~4)에 대한 삭제/기록은 이 파일 어디에도 없습니다.
// 그 작업은 전적으로 섹터 0 의 부트로더(bootloader/boot_main.c)가 수행합니다.
#include "bootloader.h"  // 부트로더 클라이언트 인터페이스를 포함합니다.
#include "bsp_flash.h"   // 플래시 삭제/기록/검증 함수를 사용하기 위해 포함합니다.
#include "bsp_iwdg.h"    // 장시간 작업 중 워치독을 피드하기 위해 포함합니다.

#include "stm32f4xx_hal.h"  // 백업 SRAM 클록 제어와 NVIC_SystemReset 을 사용하기 위해 포함합니다.

// 스테이징 진행 상태입니다. 전원이 꺼지면 사라지지만, 플래시에 남은 헤더/표식이
// 진실의 원천이므로 재부팅 후에도 부트로더가 올바르게 판단할 수 있습니다.
static uint32_t s_ulStagingSize = 0U;  // 현재 스테이징 중인 이미지의 전체 크기입니다.
static uint32_t s_ulStagingWritten = 0U;  // 지금까지 스테이징에 기록한 바이트 수입니다.
static BaseType_t s_xStagingActive = pdFALSE;  // 스테이징이 진행 중인지 여부입니다.

static void prvWatchdogHook(void)  // 플래시 드라이버가 주기적으로 호출할 워치독 피드 훅을 정의합니다.
{  // 워치독 훅 함수 본문을 시작합니다.
    IWDG_ReloadCounter();  // 독립 워치독 카운터를 리프레시합니다.
}  // 워치독 훅 함수를 종료합니다.

static void prvBootCtrlCommit(volatile BootCtrl_t *pxCtrl)  // 부트 제어 블록의 체크섬을 갱신하는 내부 함수를 정의합니다.
{  // 체크섬 갱신 함수 본문을 시작합니다.
    pxCtrl->ulMagic = BOOT_CTRL_MAGIC;  // 매직 값을 항상 유효하게 설정합니다.
    pxCtrl->ulCheck = BootCtrl_CalcCheck(pxCtrl);  // 나머지 필드로부터 체크섬을 다시 계산해 기록합니다.
}  // 체크섬 갱신 함수를 종료합니다.

void Bootloader_Init(void)  // 부트로더 클라이언트 초기화 함수를 정의합니다.
{  // 초기화 함수 본문을 시작합니다.
    volatile BootCtrl_t *pxCtrl = BOOT_CTRL_PTR;  // 백업 SRAM 의 부트 제어 블록 포인터를 가져옵니다.

    __HAL_RCC_PWR_CLK_ENABLE();  // 전원 제어(PWR) 클록을 활성화합니다. (백업 도메인 접근 선행 조건)
    __HAL_RCC_BKPSRAM_CLK_ENABLE();  // 백업 SRAM 클록을 활성화합니다.
    HAL_PWR_EnableBkUpAccess();  // 백업 도메인에 대한 쓰기 접근을 허용합니다.

    Flash_SetProgressHook(prvWatchdogHook);  // 플래시 장시간 작업 중 워치독이 물리지 않도록 훅을 등록합니다.

    if (BootCtrl_IsValid(pxCtrl) == 0)  // 부트 제어 블록이 손상되었거나 초기 상태인지 확인합니다.
    {  // 블록 초기화 처리를 시작합니다.
        pxCtrl->ulRequest = BOOT_REQUEST_NONE;  // 요청 없음으로 초기화합니다.
        pxCtrl->ulAttempts = 0U;  // 설치 시도 횟수를 초기화합니다.
        pxCtrl->ulLastResult = BOOT_RESULT_NONE;  // 마지막 결과를 초기화합니다.
        pxCtrl->ulResetCount = 0U;  // 부팅 횟수를 초기화합니다.
        prvBootCtrlCommit(pxCtrl);  // 매직과 체크섬을 기록해 블록을 유효화합니다.
    }  // 블록 초기화 처리를 종료합니다.

    // 부트로더가 남긴 설치 요청이 아직 남아 있다면(설치가 끝나 앱이 실행 중이므로) 정리합니다.
    if (pxCtrl->ulRequest == BOOT_REQUEST_UPDATE)  // 요청이 여전히 설정되어 있는지 확인합니다.
    {  // 잔여 요청 정리 블록을 시작합니다.
        pxCtrl->ulRequest = BOOT_REQUEST_NONE;  // 요청을 해제합니다.
        pxCtrl->ulAttempts = 0U;  // 시도 횟수를 초기화합니다.
        prvBootCtrlCommit(pxCtrl);  // 체크섬을 갱신합니다.
    }  // 잔여 요청 정리 블록을 종료합니다.

    s_ulStagingSize = 0U;  // 스테이징 크기 상태를 초기화합니다.
    s_ulStagingWritten = 0U;  // 기록량 상태를 초기화합니다.
    s_xStagingActive = pdFALSE;  // 스테이징 비활성 상태로 시작합니다.
}  // 초기화 함수를 종료합니다.

BaseType_t Bootloader_BeginStaging(uint32_t ulImageSize)  // 스테이징 시작 함수를 정의합니다.
{  // 스테이징 시작 함수 본문을 시작합니다.
    if ((ulImageSize == 0U) || (ulImageSize > STAGE_IMAGE_MAX_SIZE) || ((ulImageSize & 0x3U) != 0U))  // 크기 유효성을 확인합니다.
    {  // 크기 오류 처리 블록을 시작합니다.
        return pdFAIL;  // 스테이징 시작 실패를 반환합니다.
    }  // 크기 오류 처리 블록을 종료합니다.

    Flash_Init();  // 플래시 제어 레지스터를 언락합니다.

    // 스테이징 영역 전체(섹터 5)를 삭제합니다. 앱 영역은 건드리지 않으므로
    // 이 작업 도중 전원이 끊겨도 실행 중인 펌웨어는 그대로 남아 있습니다.
    if (Flash_EraseRange(STAGE_REGION_ADDR, STAGE_REGION_SIZE) != pdPASS)  // 스테이징 영역 삭제가 실패했는지 확인합니다.
    {  // 삭제 실패 처리 블록을 시작합니다.
        Flash_Deinit();  // 플래시를 다시 잠급니다.
        return pdFAIL;  // 실패를 반환합니다.
    }  // 삭제 실패 처리 블록을 종료합니다.

    Flash_Deinit();  // 조각 기록 전까지는 플래시를 잠가 두어 오동작 기록을 막습니다.

    s_ulStagingSize = ulImageSize;  // 목표 이미지 크기를 저장합니다.
    s_ulStagingWritten = 0U;  // 기록량을 0 으로 초기화합니다.
    s_xStagingActive = pdTRUE;  // 스테이징 진행 중 상태로 전환합니다.

    return pdPASS;  // 스테이징 시작 성공을 반환합니다.
}  // 스테이징 시작 함수를 종료합니다.

BaseType_t Bootloader_WriteChunk(uint32_t ulOffset, const void *pvData, uint32_t ulLength)  // 이미지 조각 기록 함수를 정의합니다.
{  // 조각 기록 함수 본문을 시작합니다.
    BaseType_t xResult;  // 기록 결과를 담을 변수입니다.

    if (s_xStagingActive != pdTRUE)  // 스테이징이 시작되지 않았는지 확인합니다.
    {  // 미시작 처리 블록을 시작합니다.
        return pdFAIL;  // 실패를 반환합니다.
    }  // 미시작 처리 블록을 종료합니다.

    if ((pvData == 0) || (ulLength == 0U) || ((ulLength & 0x3U) != 0U) || ((ulOffset & 0x3U) != 0U))  // 인자 유효성을 확인합니다.
    {  // 인자 오류 처리 블록을 시작합니다.
        return pdFAIL;  // 실패를 반환합니다.
    }  // 인자 오류 처리 블록을 종료합니다.

    if ((ulOffset + ulLength) > s_ulStagingSize)  // 기록 범위가 선언된 이미지 크기를 넘는지 확인합니다.
    {  // 범위 초과 처리 블록을 시작합니다.
        return pdFAIL;  // 실패를 반환합니다.
    }  // 범위 초과 처리 블록을 종료합니다.

    Flash_Init();  // 기록을 위해 플래시를 언락합니다.
    xResult = Flash_ProgramBuffer(STAGE_IMAGE_ADDR + ulOffset, pvData, ulLength);  // 스테이징 본문 영역에 조각을 기록합니다.
    Flash_Deinit();  // 기록이 끝나면 즉시 플래시를 잠급니다.

    if (xResult != pdPASS)  // 기록이 실패했는지 확인합니다.
    {  // 기록 실패 처리 블록을 시작합니다.
        return pdFAIL;  // 실패를 반환합니다.
    }  // 기록 실패 처리 블록을 종료합니다.

    if ((ulOffset + ulLength) > s_ulStagingWritten)  // 지금까지의 최대 기록 위치가 갱신되었는지 확인합니다.
    {  // 기록량 갱신 블록을 시작합니다.
        s_ulStagingWritten = ulOffset + ulLength;  // 누적 기록량을 갱신합니다.
    }  // 기록량 갱신 블록을 종료합니다.

    return pdPASS;  // 조각 기록 성공을 반환합니다.
}  // 조각 기록 함수를 종료합니다.

BaseType_t Bootloader_FinishStaging(const FwImageHeader_t *pxHeader)  // 스테이징 마무리 함수를 정의합니다.
{  // 스테이징 마무리 함수 본문을 시작합니다.
    BaseType_t xResult;  // 헤더 기록 결과를 담을 변수입니다.

    if ((s_xStagingActive != pdTRUE) || (pxHeader == 0))  // 스테이징 상태와 헤더 포인터를 확인합니다.
    {  // 상태 오류 처리 블록을 시작합니다.
        return pdFAIL;  // 실패를 반환합니다.
    }  // 상태 오류 처리 블록을 종료합니다.

    if (pxHeader->ulImageSize != s_ulStagingSize)  // 헤더의 크기가 스테이징 선언 크기와 다른지 확인합니다.
    {  // 크기 불일치 처리 블록을 시작합니다.
        return pdFAIL;  // 실패를 반환합니다.
    }  // 크기 불일치 처리 블록을 종료합니다.

    if (s_ulStagingWritten != s_ulStagingSize)  // 이미지 전체가 기록되지 않았는지 확인합니다.
    {  // 미완성 처리 블록을 시작합니다.
        return pdFAIL;  // 실패를 반환합니다.
    }  // 미완성 처리 블록을 종료합니다.

    // 본문이 모두 기록된 뒤에야 헤더를 씁니다. 헤더가 있다는 것은 "본문이 완성되었다"는
    // 뜻이 되므로, 헤더 기록 이전에 전원이 끊기면 부트로더는 이 이미지를 무시합니다.
    Flash_Init();  // 헤더 기록을 위해 플래시를 언락합니다.
    xResult = Flash_ProgramBuffer(STAGE_HEADER_ADDR, pxHeader, FW_IMAGE_HEADER_SIZE);  // 헤더 32바이트를 기록합니다.
    Flash_Deinit();  // 플래시를 다시 잠급니다.

    if (xResult != pdPASS)  // 헤더 기록이 실패했는지 확인합니다.
    {  // 헤더 기록 실패 처리 블록을 시작합니다.
        return pdFAIL;  // 실패를 반환합니다.
    }  // 헤더 기록 실패 처리 블록을 종료합니다.

    s_xStagingActive = pdFALSE;  // 스테이징을 완료 상태로 전환합니다.

    // 플래시에 실제로 기록된 내용으로 최종 검증합니다. (기록 오류를 여기서 걸러냅니다)
    return (Bootloader_VerifyStaged() == FW_IMAGE_OK) ? pdPASS : pdFAIL;  // 검증 결과를 반환합니다.
}  // 스테이징 마무리 함수를 종료합니다.

void Bootloader_AbortStaging(void)  // 스테이징 취소 함수를 정의합니다.
{  // 스테이징 취소 함수 본문을 시작합니다.
    // 헤더를 아직 쓰지 않았다면 부트로더는 이 이미지를 인식하지 않습니다.
    // 이미 헤더가 있다면 스테이징 영역을 삭제해 확실히 무효화합니다.
    Flash_Init();  // 플래시를 언락합니다.
    (void)Flash_EraseSector(Flash_GetSector(STAGE_REGION_ADDR));  // 헤더가 있는 첫 섹터를 삭제해 이미지를 무효화합니다.
    Flash_Deinit();  // 플래시를 다시 잠급니다.

    s_ulStagingSize = 0U;  // 스테이징 크기를 초기화합니다.
    s_ulStagingWritten = 0U;  // 기록량을 초기화합니다.
    s_xStagingActive = pdFALSE;  // 스테이징 비활성 상태로 되돌립니다.
}  // 스테이징 취소 함수를 종료합니다.

FwImageStatus_t Bootloader_VerifyStaged(void)  // 스테이징 이미지 검증 함수를 정의합니다.
{  // 검증 함수 본문을 시작합니다.
    const FwImageHeader_t *pxHeader = (const FwImageHeader_t *)STAGE_HEADER_ADDR;  // 스테이징 헤더를 직접 가리킵니다.
    const void *pvImage = (const void *)STAGE_IMAGE_ADDR;  // 스테이징 본문을 직접 가리킵니다.

    IWDG_ReloadCounter();  // 최대 112KB 에 대한 CRC 계산 전에 워치독을 피드합니다.
    return FwImage_Verify(pxHeader, pvImage);  // 헤더와 본문 CRC 를 모두 검증한 결과를 반환합니다.
}  // 검증 함수를 종료합니다.

FirmwareState_t Bootloader_CheckUpdateRequest(void)  // 업데이트 가능 여부 확인 함수를 정의합니다.
{  // 업데이트 확인 함수 본문을 시작합니다.
    const FwImageHeader_t *pxHeader = (const FwImageHeader_t *)STAGE_HEADER_ADDR;  // 스테이징 헤더를 가리킵니다.

    if (s_xStagingActive == pdTRUE)  // 아직 수신이 진행 중인지 확인합니다.
    {  // 진행 중 처리 블록을 시작합니다.
        return UPDATE_NONE;  // 수신이 끝나지 않았으므로 설치 대상이 아닙니다.
    }  // 진행 중 처리 블록을 종료합니다.

    if (*(const volatile uint32_t *)STAGE_INSTALLED_ADDR == STAGE_INSTALLED_MARK)  // 이미 설치 완료 표식이 찍혔는지 확인합니다.
    {  // 설치 완료 처리 블록을 시작합니다.
        return UPDATE_NONE;  // 이미 반영된 이미지이므로 다시 설치하지 않습니다.
    }  // 설치 완료 처리 블록을 종료합니다.

    // 헤더만 가볍게 확인합니다. 전체 CRC 검증은 비용이 크므로 설치 직전에 수행합니다.
    if (FwImage_CheckHeader(pxHeader) != FW_IMAGE_OK)  // 헤더가 유효하지 않은지 확인합니다.
    {  // 헤더 무효 처리 블록을 시작합니다.
        return UPDATE_NONE;  // 설치 가능한 이미지가 없음을 반환합니다.
    }  // 헤더 무효 처리 블록을 종료합니다.

    return UPDATE_AVAILABLE;  // 설치 가능한 스테이징 이미지가 있음을 반환합니다.
}  // 업데이트 확인 함수를 종료합니다.

uint32_t Bootloader_GetStagedVersion(void)  // 스테이징 이미지 버전 조회 함수를 정의합니다.
{  // 버전 조회 함수 본문을 시작합니다.
    const FwImageHeader_t *pxHeader = (const FwImageHeader_t *)STAGE_HEADER_ADDR;  // 스테이징 헤더를 가리킵니다.

    if (FwImage_CheckHeader(pxHeader) != FW_IMAGE_OK)  // 헤더가 유효하지 않은지 확인합니다.
    {  // 헤더 무효 처리 블록을 시작합니다.
        return 0U;  // 버전을 알 수 없으므로 0 을 반환합니다.
    }  // 헤더 무효 처리 블록을 종료합니다.

    return pxHeader->ulFwVersion;  // 헤더에 기록된 펌웨어 버전을 반환합니다.
}  // 버전 조회 함수를 종료합니다.

uint32_t Bootloader_GetLastBootResult(void)  // 마지막 부트 결과 조회 함수를 정의합니다.
{  // 부트 결과 조회 함수 본문을 시작합니다.
    volatile BootCtrl_t *pxCtrl = BOOT_CTRL_PTR;  // 부트 제어 블록을 가리킵니다.

    if (BootCtrl_IsValid(pxCtrl) == 0)  // 블록이 유효하지 않은지 확인합니다.
    {  // 블록 무효 처리 블록을 시작합니다.
        return BOOT_RESULT_NONE;  // 결과 없음을 반환합니다.
    }  // 블록 무효 처리 블록을 종료합니다.

    return pxCtrl->ulLastResult;  // 부트로더가 남긴 마지막 결과 코드를 반환합니다.
}  // 부트 결과 조회 함수를 종료합니다.

void Bootloader_SetUpdateRequest(void)  // 설치 요청 설정 함수를 정의합니다.
{  // 요청 설정 함수 본문을 시작합니다.
    volatile BootCtrl_t *pxCtrl = BOOT_CTRL_PTR;  // 부트 제어 블록을 가리킵니다.

    pxCtrl->ulRequest = BOOT_REQUEST_UPDATE;  // 부트로더에게 설치를 요청합니다.
    pxCtrl->ulAttempts = 0U;  // 새 요청이므로 시도 횟수를 초기화합니다.
    prvBootCtrlCommit(pxCtrl);  // 매직과 체크섬을 갱신해 요청을 확정합니다.
}  // 요청 설정 함수를 종료합니다.

void Bootloader_ClearUpdateRequest(void)  // 설치 요청 해제 함수를 정의합니다.
{  // 요청 해제 함수 본문을 시작합니다.
    volatile BootCtrl_t *pxCtrl = BOOT_CTRL_PTR;  // 부트 제어 블록을 가리킵니다.

    pxCtrl->ulRequest = BOOT_REQUEST_NONE;  // 요청을 해제합니다.
    pxCtrl->ulAttempts = 0U;  // 시도 횟수를 초기화합니다.
    prvBootCtrlCommit(pxCtrl);  // 체크섬을 갱신해 변경을 확정합니다.
}  // 요청 해제 함수를 종료합니다.

BaseType_t Bootloader_RequestInstallAndReset(void)  // 설치 요청 후 리셋하는 함수를 정의합니다.
{  // 설치 요청 함수 본문을 시작합니다.
    if (Bootloader_VerifyStaged() != FW_IMAGE_OK)  // 설치 직전에 전체 CRC 로 최종 검증합니다.
    {  // 검증 실패 처리 블록을 시작합니다.
        return pdFAIL;  // 손상된 이미지로 리셋하지 않고 실패를 반환합니다.
    }  // 검증 실패 처리 블록을 종료합니다.

    Bootloader_SetUpdateRequest();  // 부트 제어 블록에 설치 요청을 남깁니다.

    IWDG_ReloadCounter();  // 리셋 직전에 워치독을 피드해 여유를 확보합니다.
    __DSB();  // 백업 SRAM 쓰기가 실제로 완료되도록 데이터 동기화 배리어를 실행합니다.

    NVIC_SystemReset();  // 시스템을 리셋합니다. 부트로더가 설치를 수행합니다.

    return pdPASS;  // 실제로는 도달하지 않지만 형식상 성공을 반환합니다.
}  // 설치 요청 함수를 종료합니다.
