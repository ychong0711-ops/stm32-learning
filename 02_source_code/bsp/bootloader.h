// bootloader.h — 애플리케이션 측 부트로더 인터페이스(업데이트 클라이언트) 헤더입니다.
//
// [설계 변경: 셀프 플래싱 → 스테이징 패턴]
//   (이전) 앱이 실행 중에 자기 자신이 있는 섹터 1~4 를 직접 삭제/기록 → 정전·리셋 시
//          실행 코드가 사라져 복구 불가(벽돌). 삭제 도중에는 코드 페치도 불가능.
//   (현재) 앱은 수신 이미지를 "스테이징 영역(섹터 5)"에만 기록합니다. 자기 자신은
//          절대 건드리지 않습니다. 기록·검증이 끝나면 백업 SRAM 의 부트 제어 블록에
//          설치 요청만 남기고 리셋합니다. 실제 앱 영역 설치는 섹터 0 에 상주하는
//          부트로더(bootloader/boot_main.c)가 수행합니다.
//   → 어느 시점에 전원이 끊겨도 부트로더는 살아 있고, 스테이징 이미지가 온전하면
//     다음 부팅에서 설치를 이어서 완료할 수 있습니다.
//
// 이 헤더는 앱 전용입니다. 부트로더 자체는 boot_flash.h / flash_map.h 를 직접 씁니다.
#ifndef BOOTLOADER_H  // BOOTLOADER_H 가 아직 정의되지 않았는지 확인합니다.
#define BOOTLOADER_H  // BOOTLOADER_H 를 정의하여 중복 포함을 방지합니다.

#include <stdint.h>  // uint32_t 등 고정 폭 정수 타입을 사용하기 위해 포함합니다.

#include "flash_map.h"  // 플래시 맵과 부트 제어 블록 정의를 사용하기 위해 포함합니다.
#include "fw_image.h"   // 이미지 헤더 구조체와 CRC 검증 API 를 사용하기 위해 포함합니다.

#ifdef BSP_BAREMETAL  // FreeRTOS 가 없는 빌드인지 확인합니다.
    #include "bsp_baremetal.h"  // BaseType_t 의 최소 정의를 포함합니다.
#else  // FreeRTOS 애플리케이션 빌드인 경우입니다.
    #include "FreeRTOS.h"  // BaseType_t 와 pdPASS/pdFAIL 을 사용하기 위해 포함합니다.
#endif  // 빌드 종류 분기를 종료합니다.

typedef enum  // 펌웨어 상태 열거형 정의를 시작합니다.
{  // 펌웨어 상태 값 선언을 시작합니다.
    UPDATE_NONE = 0,     // 설치 대기 중인 유효한 스테이징 이미지가 없는 상태를 의미합니다.
    UPDATE_AVAILABLE     // 검증을 통과한 스테이징 이미지가 있어 설치 가능한 상태를 의미합니다.
} FirmwareState_t;  // 펌웨어 상태 타입 이름을 FirmwareState_t 로 정의합니다.

// 하위 호환용 별칭입니다. 새 코드는 flash_map.h 의 이름을 직접 사용하십시오.
#define BOOTLOADER_APP_START_ADDR   APP_REGION_ADDR   // 앱 영역 시작 주소(섹터 1)입니다.
#define BOOTLOADER_UPDATE_MAGIC     BOOT_CTRL_MAGIC   // 부트 제어 블록 매직 값입니다.

void Bootloader_Init(void);  // 백업 SRAM 접근을 열고 부트 제어 블록을 준비하는 함수 프로토타입입니다.

// ---- 스테이징 (앱 → 섹터 5) ----------------------------------------------
// 사용 순서: BeginStaging() → WriteChunk() 반복 → FinishStaging() → RequestInstall()
BaseType_t Bootloader_BeginStaging(uint32_t ulImageSize);  // 스테이징 영역을 삭제하고 수신을 시작하는 함수 프로토타입입니다.
BaseType_t Bootloader_WriteChunk(uint32_t ulOffset, const void *pvData, uint32_t ulLength);  // 이미지 조각을 스테이징에 기록하는 함수 프로토타입입니다.
BaseType_t Bootloader_FinishStaging(const FwImageHeader_t *pxHeader);  // 헤더를 기록하고 전체 이미지를 검증하는 함수 프로토타입입니다.
void Bootloader_AbortStaging(void);  // 진행 중인 스테이징을 취소하고 내부 상태를 되돌리는 함수 프로토타입입니다.

// ---- 검증 / 조회 ----------------------------------------------------------
FwImageStatus_t Bootloader_VerifyStaged(void);  // 스테이징 이미지를 헤더+CRC 로 검증하는 함수 프로토타입입니다.
FirmwareState_t Bootloader_CheckUpdateRequest(void);  // 설치 가능한 스테이징 이미지가 있는지 확인하는 함수 프로토타입입니다.
uint32_t Bootloader_GetStagedVersion(void);  // 스테이징 이미지의 펌웨어 버전을 반환하는 함수 프로토타입입니다.
uint32_t Bootloader_GetLastBootResult(void);  // 부트로더가 마지막으로 남긴 결과 코드를 반환하는 함수 프로토타입입니다.

// ---- 설치 요청 (앱 → 부트로더) --------------------------------------------
void Bootloader_SetUpdateRequest(void);  // 부트 제어 블록에 설치 요청을 기록하는 함수 프로토타입입니다.
void Bootloader_ClearUpdateRequest(void);  // 설치 요청을 취소하는 함수 프로토타입입니다.
BaseType_t Bootloader_RequestInstallAndReset(void);  // 검증 후 설치를 요청하고 시스템을 리셋하는 함수 프로토타입입니다.

#endif /* BOOTLOADER_H */  // BOOTLOADER_H 조건부 컴파일 블록을 종료합니다.
