// boot_main.c — 섹터 0 에 상주하는 부트로더의 본체입니다.
//
// 역할 (앱은 절대 자기 자신을 덮어쓰지 않습니다):
//   1) 백업 SRAM 의 부트 제어 블록에서 설치 요청을 읽습니다.
//   2) 요청이 있고 스테이징 이미지(섹터 5)가 CRC 검증을 통과하면,
//      앱 영역(섹터 1~4)을 삭제하고 스테이징 → 앱으로 복사한 뒤 검증합니다.
//   3) 설치가 끝나면 스테이징에 "설치 완료" 표식을 남깁니다.
//   4) 앱 영역의 벡터 테이블이 정상이면 앱으로 점프합니다.
//   5) 앱이 손상되었는데 스테이징 이미지가 유효하면 자동으로 복구를 시도합니다.
//   6) 둘 다 불가능하면 복구 대기 루프(LED 점멸)에 들어갑니다.
//
// 전원 차단 안전성:
//   - 앱 영역 삭제/복사 도중 전원이 끊겨도 부트로더(섹터 0)와 스테이징 이미지(섹터 5)는
//     그대로 남습니다. 다음 부팅에서 앱 벡터가 깨진 것을 감지하고 복구를 재개합니다.
//   - 같은 이미지로 BOOT_MAX_INSTALL_ATTEMPTS 회 이상 실패하면 무한 재시도를 멈춥니다.
#include <stdint.h>  // 고정 폭 정수 타입을 사용하기 위해 포함합니다.

#include "stm32f4xx_hal.h"  // HAL 초기화와 GPIO/PWR 제어를 사용하기 위해 포함합니다.

#include "flash_map.h"  // 플래시 맵과 부트 제어 블록 정의를 포함합니다.
#include "fw_image.h"   // 이미지 헤더와 CRC 검증 API 를 포함합니다.
#include "bsp_flash.h"  // 플래시 삭제/기록/검증 함수를 포함합니다.
#include "bsp_iwdg.h"   // 워치독 피드와 리셋 원인 확인을 위해 포함합니다.

#define BOOT_LED_PORT       GPIOA  // 상태 표시 LED 가 연결된 포트입니다. (NUCLEO-F446RE 의 LD2)
#define BOOT_LED_PIN        GPIO_PIN_5  // 상태 표시 LED 핀 번호입니다.
#define BOOT_COPY_CHUNK     256U  // 복사 시 한 번에 처리할 바이트 수입니다. (워치독 피드 간격)
#define BOOT_IWDG_TIMEOUT_MS 8000U  // 부트로더 구간에서 사용할 넉넉한 워치독 타임아웃입니다.

typedef void (*pfnAppEntry_t)(void);  // 앱 리셋 핸들러를 호출하기 위한 함수 포인터 타입입니다.

static void prvLedInit(void);  // 상태 LED 를 초기화하는 내부 함수 프로토타입입니다.
static void prvLedBlink(uint32_t ulCount, uint32_t ulDelayMs);  // LED 를 지정 횟수 점멸하는 내부 함수 프로토타입입니다.
static void prvBootCtrlCommit(volatile BootCtrl_t *pxCtrl);  // 부트 제어 블록 체크섬을 갱신하는 내부 함수 프로토타입입니다.
static void prvWatchdogHook(void);  // 플래시 드라이버가 호출할 워치독 피드 훅 프로토타입입니다.
static int prvIsAppValid(void);  // 앱 영역이 실행 가능한 상태인지 확인하는 내부 함수 프로토타입입니다.
static int prvInstallStagedImage(void);  // 스테이징 이미지를 앱 영역에 설치하는 내부 함수 프로토타입입니다.
static void prvMarkStagedInstalled(void);  // 스테이징에 설치 완료 표식을 기록하는 내부 함수 프로토타입입니다.
static void prvJumpToApp(void);  // 앱으로 점프하는 내부 함수 프로토타입입니다. (복귀하지 않음)
static void prvRecoveryLoop(void);  // 복구 대기 무한 루프 프로토타입입니다.

// --------------------------------------------------------------------------
// 부트로더 진입점
// --------------------------------------------------------------------------
int main(void)  // 부트로더의 main 함수를 정의합니다.
{  // 부트로더 main 함수 본문을 시작합니다.
    volatile BootCtrl_t *pxCtrl = BOOT_CTRL_PTR;  // 백업 SRAM 의 부트 제어 블록을 가리킵니다.
    uint32_t ulRequest = BOOT_REQUEST_NONE;  // 앱이 남긴 요청 코드를 담을 변수입니다.
    uint32_t ulAttempts = 0U;  // 설치 시도 횟수를 담을 변수입니다.
    int iAppValid;  // 앱 영역 유효성 결과를 담을 변수입니다.
    int iStagedValid;  // 스테이징 이미지 유효성 결과를 담을 변수입니다.

    HAL_Init();  // HAL 을 초기화합니다. (SysTick 1ms 기동 → HAL_Delay 사용 가능)

    // 부트로더는 클록을 올리지 않고 HSI 16MHz 기본 상태로 동작합니다.
    // 설치 시간은 조금 길어지지만, PLL 설정 실패라는 실패 지점을 없애 신뢰성을 높입니다.

    prvLedInit();  // 상태 표시 LED 를 초기화합니다.

    // 앱에서 IWDG 가 켜진 채 리셋되었을 수 있습니다. IWDG 는 끌 수 없으므로
    // 부트로더 구간 전체를 덮을 만큼 넉넉한 타임아웃으로 다시 설정하고 계속 피드합니다.
    IWDG_Init(BOOT_IWDG_TIMEOUT_MS);  // 부트로더용 워치독 타임아웃을 설정합니다.
    IWDG_ReloadCounter();  // 즉시 한 번 피드합니다.

    Flash_SetProgressHook(prvWatchdogHook);  // 플래시 장시간 작업 중 워치독을 자동으로 피드하도록 등록합니다.

    // 백업 SRAM 을 읽으려면 PWR 클록과 백업 도메인 접근이 필요합니다.
    __HAL_RCC_PWR_CLK_ENABLE();  // 전원 제어 클록을 활성화합니다.
    __HAL_RCC_BKPSRAM_CLK_ENABLE();  // 백업 SRAM 클록을 활성화합니다.
    HAL_PWR_EnableBkUpAccess();  // 백업 도메인 쓰기 접근을 허용합니다.

    if (BootCtrl_IsValid(pxCtrl) != 0)  // 부트 제어 블록이 유효한지 확인합니다.
    {  // 유효한 블록 처리를 시작합니다.
        ulRequest = pxCtrl->ulRequest;  // 앱이 남긴 요청 코드를 읽습니다.
        ulAttempts = pxCtrl->ulAttempts;  // 지금까지의 설치 시도 횟수를 읽습니다.
    }  // 유효한 블록 처리를 종료합니다.
    else  // 블록이 손상되었거나 첫 부팅인 경우입니다.
    {  // 블록 초기화 처리를 시작합니다.
        pxCtrl->ulRequest = BOOT_REQUEST_NONE;  // 요청 없음으로 초기화합니다.
        pxCtrl->ulAttempts = 0U;  // 시도 횟수를 초기화합니다.
        pxCtrl->ulLastResult = BOOT_RESULT_NONE;  // 결과를 초기화합니다.
        pxCtrl->ulResetCount = 0U;  // 부팅 횟수를 초기화합니다.
        prvBootCtrlCommit(pxCtrl);  // 매직과 체크섬을 기록합니다.
    }  // 블록 초기화 처리를 종료합니다.

    pxCtrl->ulResetCount++;  // 부트로더를 통과한 부팅 횟수를 증가시킵니다. (진단용)
    prvBootCtrlCommit(pxCtrl);  // 변경 사항의 체크섬을 갱신합니다.

    iAppValid = prvIsAppValid();  // 현재 앱 영역이 실행 가능한지 확인합니다.
    iStagedValid = (FwImage_CheckHeader((const FwImageHeader_t *)STAGE_HEADER_ADDR) == FW_IMAGE_OK) ? 1 : 0;  // 스테이징 헤더 유효성을 확인합니다.

    // ---- 설치가 필요한 상황인지 판단합니다 ----
    // (a) 앱이 명시적으로 설치를 요청했다.
    // (b) 요청은 없지만 앱이 깨져 있고 스테이징 이미지는 유효하다 → 자동 복구.
    if (((ulRequest == BOOT_REQUEST_UPDATE) || (iAppValid == 0)) && (iStagedValid != 0))  // 설치 조건을 확인합니다.
    {  // 설치 시도 블록을 시작합니다.
        // 아직 설치되지 않은 이미지인지 확인합니다. (같은 이미지 반복 설치 방지)
        int iAlreadyInstalled = (*(const volatile uint32_t *)STAGE_INSTALLED_ADDR == STAGE_INSTALLED_MARK) ? 1 : 0;  // 설치 완료 표식을 확인합니다.

        if ((iAlreadyInstalled != 0) && (iAppValid != 0))  // 이미 설치됐고 앱도 정상인지 확인합니다.
        {  // 재설치 불필요 처리 블록을 시작합니다.
            pxCtrl->ulRequest = BOOT_REQUEST_NONE;  // 요청을 소비합니다.
            pxCtrl->ulLastResult = BOOT_RESULT_KEPT;  // 기존 앱을 유지했음을 기록합니다.
            prvBootCtrlCommit(pxCtrl);  // 체크섬을 갱신합니다.
        }  // 재설치 불필요 처리 블록을 종료합니다.
        else if (ulAttempts >= BOOT_MAX_INSTALL_ATTEMPTS)  // 재시도 한도를 초과했는지 확인합니다.
        {  // 재시도 한도 초과 처리 블록을 시작합니다.
            pxCtrl->ulRequest = BOOT_REQUEST_NONE;  // 무한 재시도를 막기 위해 요청을 지웁니다.
            pxCtrl->ulLastResult = BOOT_RESULT_FAILED;  // 설치 실패로 기록합니다.
            prvBootCtrlCommit(pxCtrl);  // 체크섬을 갱신합니다.
        }  // 재시도 한도 초과 처리 블록을 종료합니다.
        else  // 실제로 설치를 시도할 수 있는 경우입니다.
        {  // 설치 수행 블록을 시작합니다.
            pxCtrl->ulAttempts = ulAttempts + 1U;  // 시도 횟수를 먼저 증가시켜 기록합니다. (도중 정전 대비)
            prvBootCtrlCommit(pxCtrl);  // 체크섬을 갱신합니다.

            prvLedBlink(2U, 100U);  // 설치 시작을 LED 2회 점멸로 알립니다.

            if (prvInstallStagedImage() != 0)  // 스테이징 이미지 설치가 성공했는지 확인합니다.
            {  // 설치 성공 처리 블록을 시작합니다.
                prvMarkStagedInstalled();  // 스테이징에 설치 완료 표식을 남깁니다.

                pxCtrl->ulRequest = BOOT_REQUEST_NONE;  // 요청을 소비합니다.
                pxCtrl->ulAttempts = 0U;  // 성공했으므로 시도 횟수를 초기화합니다.
                pxCtrl->ulLastResult = (iAppValid == 0) ? BOOT_RESULT_RECOVERED : BOOT_RESULT_INSTALLED;  // 복구인지 일반 설치인지 기록합니다.
                prvBootCtrlCommit(pxCtrl);  // 체크섬을 갱신합니다.

                iAppValid = prvIsAppValid();  // 설치 후 앱 유효성을 다시 확인합니다.
                prvLedBlink(3U, 80U);  // 설치 성공을 LED 3회 점멸로 알립니다.
            }  // 설치 성공 처리 블록을 종료합니다.
            else  // 설치가 실패한 경우입니다.
            {  // 설치 실패 처리 블록을 시작합니다.
                pxCtrl->ulLastResult = BOOT_RESULT_FAILED;  // 설치 실패를 기록합니다.
                prvBootCtrlCommit(pxCtrl);  // 체크섬을 갱신합니다.

                iAppValid = prvIsAppValid();  // 앱이 부분적으로 덮여 깨졌을 수 있으므로 다시 확인합니다.
            }  // 설치 실패 처리 블록을 종료합니다.
        }  // 설치 수행 블록을 종료합니다.
    }  // 설치 시도 블록을 종료합니다.
    else if (ulRequest == BOOT_REQUEST_UPDATE)  // 설치 요청은 있었지만 스테이징 이미지가 무효한 경우입니다.
    {  // 무효 요청 처리 블록을 시작합니다.
        pxCtrl->ulRequest = BOOT_REQUEST_NONE;  // 처리할 수 없는 요청을 지웁니다.
        pxCtrl->ulLastResult = BOOT_RESULT_FAILED;  // 실패로 기록합니다.
        prvBootCtrlCommit(pxCtrl);  // 체크섬을 갱신합니다.
    }  // 무효 요청 처리 블록을 종료합니다.
    else  // 설치할 일이 없는 평상시 부팅입니다.
    {  // 평상시 부팅 처리 블록을 시작합니다.
        if (pxCtrl->ulLastResult == BOOT_RESULT_NONE)  // 아직 결과가 기록되지 않았는지 확인합니다.
        {  // 결과 기록 블록을 시작합니다.
            pxCtrl->ulLastResult = BOOT_RESULT_KEPT;  // 기존 앱을 그대로 실행했음을 기록합니다.
            prvBootCtrlCommit(pxCtrl);  // 체크섬을 갱신합니다.
        }  // 결과 기록 블록을 종료합니다.
    }  // 평상시 부팅 처리 블록을 종료합니다.

    IWDG_ReloadCounter();  // 점프 직전에 워치독을 피드합니다.

    if (iAppValid != 0)  // 앱 영역이 실행 가능한지 확인합니다.
    {  // 앱 실행 블록을 시작합니다.
        prvJumpToApp();  // 앱으로 점프합니다. (여기서 복귀하지 않습니다)
    }  // 앱 실행 블록을 종료합니다.

    prvRecoveryLoop();  // 실행할 앱이 없으므로 복구 대기 루프로 진입합니다.

    return 0;  // 도달하지 않지만 형식상 반환합니다.
}  // 부트로더 main 함수를 종료합니다.

// --------------------------------------------------------------------------
// 내부 구현
// --------------------------------------------------------------------------
static void prvWatchdogHook(void)  // 플래시 진행 콜백을 정의합니다.
{  // 워치독 훅 함수 본문을 시작합니다.
    IWDG_ReloadCounter();  // 워치독 카운터를 리프레시합니다.
}  // 워치독 훅 함수를 종료합니다.

static void prvBootCtrlCommit(volatile BootCtrl_t *pxCtrl)  // 부트 제어 블록 체크섬 갱신 함수를 정의합니다.
{  // 체크섬 갱신 함수 본문을 시작합니다.
    pxCtrl->ulMagic = BOOT_CTRL_MAGIC;  // 매직 값을 유효하게 유지합니다.
    pxCtrl->ulCheck = BootCtrl_CalcCheck(pxCtrl);  // 체크섬을 다시 계산해 기록합니다.
    __DSB();  // 백업 SRAM 쓰기가 실제로 반영되도록 데이터 동기화 배리어를 실행합니다.
}  // 체크섬 갱신 함수를 종료합니다.

static void prvLedInit(void)  // 상태 LED 초기화 함수를 정의합니다.
{  // LED 초기화 함수 본문을 시작합니다.
    GPIO_InitTypeDef xGpio = {0};  // GPIO 초기화 구조체를 0 으로 선언합니다.

    __HAL_RCC_GPIOA_CLK_ENABLE();  // GPIOA 클록을 활성화합니다.

    xGpio.Pin = BOOT_LED_PIN;  // LED 핀을 선택합니다.
    xGpio.Mode = GPIO_MODE_OUTPUT_PP;  // 푸시풀 출력 모드로 설정합니다.
    xGpio.Pull = GPIO_NOPULL;  // 내부 풀업/풀다운을 사용하지 않습니다.
    xGpio.Speed = GPIO_SPEED_FREQ_LOW;  // 저속으로 충분하므로 저속을 선택합니다.
    HAL_GPIO_Init(BOOT_LED_PORT, &xGpio);  // 설정을 GPIOA 에 적용합니다.

    HAL_GPIO_WritePin(BOOT_LED_PORT, BOOT_LED_PIN, GPIO_PIN_RESET);  // LED 를 소등 상태로 시작합니다.
}  // LED 초기화 함수를 종료합니다.

static void prvLedBlink(uint32_t ulCount, uint32_t ulDelayMs)  // LED 점멸 함수를 정의합니다.
{  // LED 점멸 함수 본문을 시작합니다.
    for (uint32_t i = 0U; i < ulCount; i++)  // 지정 횟수만큼 반복합니다.
    {  // 점멸 반복문 본문을 시작합니다.
        HAL_GPIO_WritePin(BOOT_LED_PORT, BOOT_LED_PIN, GPIO_PIN_SET);  // LED 를 켭니다.
        HAL_Delay(ulDelayMs);  // 지정 시간 동안 유지합니다.
        HAL_GPIO_WritePin(BOOT_LED_PORT, BOOT_LED_PIN, GPIO_PIN_RESET);  // LED 를 끕니다.
        HAL_Delay(ulDelayMs);  // 지정 시간 동안 유지합니다.
        IWDG_ReloadCounter();  // 점멸 중에도 워치독을 피드합니다.
    }  // 점멸 반복문을 종료합니다.
}  // LED 점멸 함수를 종료합니다.

static int prvIsAppValid(void)  // 앱 영역 유효성 확인 함수를 정의합니다.
{  // 앱 유효성 확인 함수 본문을 시작합니다.
    // 앱 영역에는 별도 헤더를 두지 않습니다. 벡터 테이블의 초기 SP/PC 가
    // 합리적인 값인지만 확인하여 "삭제됨/절반만 기록됨" 상태를 걸러냅니다.
    return FwImage_IsVectorTableSane((const void *)APP_REGION_ADDR, APP_REGION_ADDR, APP_REGION_SIZE);  // 벡터 테이블 정상 여부를 반환합니다.
}  // 앱 유효성 확인 함수를 종료합니다.

static int prvInstallStagedImage(void)  // 스테이징 이미지 설치 함수를 정의합니다.
{  // 설치 함수 본문을 시작합니다.
    const FwImageHeader_t *pxHeader = (const FwImageHeader_t *)STAGE_HEADER_ADDR;  // 스테이징 헤더를 가리킵니다.
    const uint8_t *pucImage = (const uint8_t *)STAGE_IMAGE_ADDR;  // 스테이징 본문을 가리킵니다.
    uint32_t ulSize;  // 이미지 크기를 담을 변수입니다.

    IWDG_ReloadCounter();  // 전체 CRC 검증 전에 워치독을 피드합니다.

    // 1) 설치 전에 스테이징 이미지 전체를 CRC 로 검증합니다.
    //    여기서 실패하면 앱 영역은 손대지 않으므로 현재 펌웨어가 그대로 보존됩니다.
    if (FwImage_Verify(pxHeader, pucImage) != FW_IMAGE_OK)  // 이미지 검증이 실패했는지 확인합니다.
    {  // 검증 실패 처리 블록을 시작합니다.
        return 0;  // 설치 실패를 반환합니다.
    }  // 검증 실패 처리 블록을 종료합니다.

    ulSize = pxHeader->ulImageSize;  // 검증된 이미지 크기를 읽습니다.

    Flash_Init();  // 플래시를 언락합니다.

    // 2) 앱 영역(섹터 1~4)을 삭제합니다. 이 순간부터 앱은 일시적으로 무효 상태가 되지만,
    //    부트로더와 스테이징 이미지가 살아 있으므로 정전되어도 다음 부팅에서 복구됩니다.
    if (Flash_EraseRange(APP_REGION_ADDR, APP_REGION_SIZE) != pdPASS)  // 앱 영역 삭제가 실패했는지 확인합니다.
    {  // 삭제 실패 처리 블록을 시작합니다.
        Flash_Deinit();  // 플래시를 다시 잠급니다.
        return 0;  // 설치 실패를 반환합니다.
    }  // 삭제 실패 처리 블록을 종료합니다.

    // 3) 스테이징 → 앱 영역으로 복사합니다. 조각 단위로 나누어 워치독을 피드합니다.
    for (uint32_t ulOffset = 0U; ulOffset < ulSize; ulOffset += BOOT_COPY_CHUNK)  // 이미지 전체를 조각 단위로 순회합니다.
    {  // 복사 반복문 본문을 시작합니다.
        uint32_t ulChunk = ulSize - ulOffset;  // 남은 바이트 수를 계산합니다.

        if (ulChunk > BOOT_COPY_CHUNK)  // 남은 양이 조각 크기보다 큰지 확인합니다.
        {  // 조각 크기 제한 블록을 시작합니다.
            ulChunk = BOOT_COPY_CHUNK;  // 한 번에 처리할 크기를 조각 크기로 제한합니다.
        }  // 조각 크기 제한 블록을 종료합니다.

        if (Flash_ProgramBuffer(APP_REGION_ADDR + ulOffset, &pucImage[ulOffset], ulChunk) != pdPASS)  // 조각 기록이 실패했는지 확인합니다.
        {  // 기록 실패 처리 블록을 시작합니다.
            Flash_Deinit();  // 플래시를 다시 잠급니다.
            return 0;  // 설치 실패를 반환합니다.
        }  // 기록 실패 처리 블록을 종료합니다.

        IWDG_ReloadCounter();  // 조각마다 워치독을 피드합니다.
    }  // 복사 반복문을 종료합니다.

    Flash_Deinit();  // 복사가 끝났으므로 플래시를 잠급니다.

    // 4) 앱 영역에 실제로 기록된 내용을 스테이징 원본과 바이트 단위로 비교합니다.
    if (Flash_Verify(APP_REGION_ADDR, pucImage, ulSize) != pdPASS)  // 기록 검증이 실패했는지 확인합니다.
    {  // 검증 실패 처리 블록을 시작합니다.
        return 0;  // 설치 실패를 반환합니다.
    }  // 검증 실패 처리 블록을 종료합니다.

    IWDG_ReloadCounter();  // 최종 CRC 검증 전에 워치독을 피드합니다.

    // 5) 앱 영역 자체를 CRC 로 한 번 더 검증하여 설치 완료를 확정합니다.
    if (FwImage_Crc32((const void *)APP_REGION_ADDR, ulSize) != pxHeader->ulImageCrc32)  // 앱 영역 CRC 가 헤더 값과 다른지 확인합니다.
    {  // CRC 불일치 처리 블록을 시작합니다.
        return 0;  // 설치 실패를 반환합니다.
    }  // CRC 불일치 처리 블록을 종료합니다.

    return 1;  // 설치 성공을 반환합니다.
}  // 설치 함수를 종료합니다.

static void prvMarkStagedInstalled(void)  // 설치 완료 표식 기록 함수를 정의합니다.
{  // 표식 기록 함수 본문을 시작합니다.
    // 스테이징 영역은 삭제하지 않고 표식만 남깁니다. 앱이 다시 깨졌을 때
    // 이 이미지를 그대로 재설치하여 복구할 수 있게 하기 위함입니다.
    Flash_Init();  // 플래시를 언락합니다.
    (void)Flash_ProgramWord(STAGE_INSTALLED_ADDR, STAGE_INSTALLED_MARK);  // 설치 완료 표식 워드를 기록합니다.
    Flash_Deinit();  // 플래시를 다시 잠급니다.
}  // 표식 기록 함수를 종료합니다.

static void prvJumpToApp(void)  // 앱 점프 함수를 정의합니다.
{  // 앱 점프 함수 본문을 시작합니다.
    uint32_t ulAppSp = *(const volatile uint32_t *)(APP_REGION_ADDR + 0U);  // 앱 벡터 테이블에서 초기 스택 포인터를 읽습니다.
    uint32_t ulAppPc = *(const volatile uint32_t *)(APP_REGION_ADDR + 4U);  // 앱 벡터 테이블에서 리셋 핸들러 주소를 읽습니다.
    pfnAppEntry_t pfnApp = (pfnAppEntry_t)ulAppPc;  // 리셋 핸들러를 함수 포인터로 변환합니다.

    // 앱이 깨끗한 상태에서 시작하도록 부트로더가 켠 주변장치를 되돌립니다.
    HAL_GPIO_WritePin(BOOT_LED_PORT, BOOT_LED_PIN, GPIO_PIN_RESET);  // 상태 LED 를 끕니다.
    HAL_RCC_DeInit();  // RCC 를 리셋 기본값(HSI)으로 되돌립니다.
    HAL_DeInit();  // HAL 이 설정한 주변장치와 SysTick 을 해제합니다.

    __disable_irq();  // 점프 준비 중 인터럽트가 끼어들지 못하게 전역 인터럽트를 막습니다.

    SysTick->CTRL = 0U;  // SysTick 타이머를 정지합니다.
    SysTick->LOAD = 0U;  // SysTick 리로드 값을 지웁니다.
    SysTick->VAL = 0U;  // SysTick 현재 값을 지웁니다.

    for (uint32_t i = 0U; i < 8U; i++)  // NVIC 의 8개 레지스터 그룹을 순회합니다.
    {  // NVIC 정리 반복문 본문을 시작합니다.
        NVIC->ICER[i] = 0xFFFFFFFFU;  // 해당 그룹의 모든 인터럽트를 비활성화합니다.
        NVIC->ICPR[i] = 0xFFFFFFFFU;  // 해당 그룹의 대기 중 인터럽트를 모두 지웁니다.
    }  // NVIC 정리 반복문을 종료합니다.

    // 앱의 벡터 테이블을 사용하도록 VTOR 을 옮깁니다.
    // (앱도 SystemInit 에서 USER_VECT_TAB_ADDRESS 로 같은 값을 설정하지만,
    //  점프 직후 발생할 수 있는 예외에 대비해 여기서 먼저 설정합니다.)
    SCB->VTOR = APP_REGION_ADDR;  // 벡터 테이블 오프셋 레지스터를 앱 주소로 설정합니다.

    __DSB();  // 이전의 모든 메모리 접근이 완료되도록 보장합니다.
    __ISB();  // 파이프라인을 비워 새 벡터 테이블 설정이 즉시 반영되게 합니다.

    __set_MSP(ulAppSp);  // 메인 스택 포인터를 앱의 초기 스택 값으로 교체합니다.
    __set_CONTROL(0U);  // 특권 모드 + MSP 사용 상태로 되돌립니다. (앱 startup 의 전제 조건)
    __ISB();  // CONTROL 변경이 즉시 반영되도록 파이프라인을 비웁니다.

    __enable_irq();  // 앱이 인터럽트를 사용할 수 있도록 전역 인터럽트를 다시 허용합니다.

    pfnApp();  // 앱의 리셋 핸들러로 점프합니다. 여기서 복귀하지 않습니다.

    for (;;)  // 만에 하나 복귀한 경우를 대비한 무한 루프입니다.
    {  // 안전 루프 본문을 시작합니다.
    }  // 안전 루프 본문을 종료합니다.
}  // 앱 점프 함수를 종료합니다.

static void prvRecoveryLoop(void)  // 복구 대기 루프 함수를 정의합니다.
{  // 복구 대기 루프 함수 본문을 시작합니다.
    // 실행 가능한 앱도, 설치 가능한 이미지도 없는 상태입니다.
    // LED 를 빠르게 점멸하며 디버거/ST-LINK 를 통한 재기록을 기다립니다.
    // 워치독은 계속 피드하여 리셋 루프에 빠지지 않게 합니다.
    for (;;)  // 무한 루프를 시작합니다.
    {  // 복구 루프 본문을 시작합니다.
        HAL_GPIO_TogglePin(BOOT_LED_PORT, BOOT_LED_PIN);  // 상태 LED 를 반전시킵니다.
        HAL_Delay(100U);  // 100ms 대기하여 빠른 점멸을 만듭니다.
        IWDG_ReloadCounter();  // 워치독을 피드하여 리셋을 방지합니다.
    }  // 복구 루프 본문을 종료합니다.
}  // 복구 대기 루프 함수를 종료합니다.
