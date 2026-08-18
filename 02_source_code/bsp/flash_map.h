// flash_map.h — 플래시 메모리 맵과 부트 제어 블록의 단일 정의처(Single Source of Truth)입니다.
// 부트로더(섹터 0)와 애플리케이션(섹터 1~4)이 같은 값을 보도록 이 헤더 하나만 공유합니다.
// 링커 스크립트(linker/*.ld)의 주소/크기와 반드시 일치해야 합니다.
#ifndef FLASH_MAP_H  // FLASH_MAP_H 가 아직 정의되지 않았는지 확인합니다.
#define FLASH_MAP_H  // FLASH_MAP_H 를 정의하여 중복 포함을 방지합니다.

#include <stdint.h>  // uint32_t 등 고정 폭 정수 타입을 사용하기 위해 포함합니다.

// --------------------------------------------------------------------------
// STM32F446RE 내부 플래시 섹터 구조 (총 512KB)
//   섹터 0 : 0x08000000 ~ 0x08003FFF (16KB)   → 부트로더
//   섹터 1 : 0x08004000 ~ 0x08007FFF (16KB)   ┐
//   섹터 2 : 0x08008000 ~ 0x0800BFFF (16KB)   ├ 애플리케이션 (합계 112KB)
//   섹터 3 : 0x0800C000 ~ 0x0800FFFF (16KB)   │
//   섹터 4 : 0x08010000 ~ 0x0801FFFF (64KB)   ┘
//   섹터 5 : 0x08020000 ~ 0x0803FFFF (128KB)  → 스테이징(수신 이미지 보관)
//   섹터 6 : 0x08040000 ~ 0x0805FFFF (128KB)  → 예비 (로그/파라미터)
//   섹터 7 : 0x08060000 ~ 0x0807FFFF (128KB)  → 예비
// --------------------------------------------------------------------------

#define FLASH_MAP_BASE_ADDR        0x08000000U  // 내부 플래시의 시작 주소입니다.
#define FLASH_MAP_TOTAL_SIZE       (512U * 1024U)  // 내부 플래시의 전체 크기(512KB)입니다.
#define FLASH_MAP_END_ADDR         (FLASH_MAP_BASE_ADDR + FLASH_MAP_TOTAL_SIZE)  // 플래시 끝(마지막+1) 주소입니다.

#define BOOT_REGION_ADDR           0x08000000U  // 부트로더 영역의 시작 주소(섹터 0)입니다.
#define BOOT_REGION_SIZE           (16U * 1024U)  // 부트로더 영역의 크기(16KB)입니다.

#define APP_REGION_ADDR            0x08004000U  // 애플리케이션 영역의 시작 주소(섹터 1)입니다.
#define APP_REGION_SIZE            (112U * 1024U)  // 애플리케이션 영역의 크기(섹터 1~4 합계 112KB)입니다.
#define APP_REGION_END_ADDR        (APP_REGION_ADDR + APP_REGION_SIZE)  // 애플리케이션 영역의 끝(마지막+1) 주소입니다.

#define STAGE_REGION_ADDR          0x08020000U  // 스테이징 영역의 시작 주소(섹터 5)입니다.
#define STAGE_REGION_SIZE          (128U * 1024U)  // 스테이징 영역의 크기(128KB)입니다.

// 스테이징 영역의 앞 512바이트는 메타데이터(헤더 + 설치 완료 표식)로 예약합니다.
// 나머지 영역에 애플리케이션 이미지 본문(.isr_vector 부터)이 그대로 저장됩니다.
#define STAGE_META_SIZE            0x200U  // 스테이징 메타데이터 영역의 크기(512바이트)입니다.
#define STAGE_HEADER_ADDR          (STAGE_REGION_ADDR + 0x000U)  // 이미지 헤더가 기록될 주소입니다.
#define STAGE_INSTALLED_ADDR       (STAGE_REGION_ADDR + 0x100U)  // 설치 완료 표식 워드가 기록될 주소입니다.
#define STAGE_IMAGE_ADDR           (STAGE_REGION_ADDR + STAGE_META_SIZE)  // 이미지 본문이 시작될 주소입니다.
#define STAGE_IMAGE_MAX_SIZE       (APP_REGION_SIZE)  // 스테이징에 담을 수 있는 이미지 최대 크기입니다. (앱 영역 크기와 동일하게 제한)

#define STAGE_INSTALLED_MARK       0x494E5354U  // 설치 완료 표식 값('INST')입니다. 소거 상태(0xFFFFFFFF)에서 1회만 기록됩니다.

// --------------------------------------------------------------------------
// 부트 제어 블록 (백업 SRAM, 4KB @ 0x40024000)
// 백업 SRAM 은 시스템 리셋으로 지워지지 않으므로, 앱 → 부트로더로 의사를 전달하는
// 통로로 사용합니다. (VBAT 가 없으면 전원 차단 시 소실되지만, 그 경우 "요청 없음"으로
// 안전하게 해석되므로 페일세이프입니다.)
// --------------------------------------------------------------------------

#define BOOT_CTRL_ADDR             0x40024000U  // 백업 SRAM 내 부트 제어 블록의 주소입니다.
#define BOOT_CTRL_MAGIC            0x5555AAAAU  // 부트 제어 블록이 유효함을 나타내는 매직 값입니다.

#define BOOT_REQUEST_NONE          0x00000000U  // 업데이트 요청이 없음을 의미하는 값입니다.
#define BOOT_REQUEST_UPDATE        0x55504454U  // 스테이징 이미지 설치를 요청하는 값('UPDT')입니다.

#define BOOT_RESULT_NONE           0x00000000U  // 부트로더가 아직 아무 작업도 하지 않았음을 의미합니다.
#define BOOT_RESULT_INSTALLED      0x0000A5A5U  // 스테이징 이미지 설치에 성공했음을 의미합니다.
#define BOOT_RESULT_RECOVERED      0x0000B6B6U  // 손상된 앱을 스테이징 이미지로 복구했음을 의미합니다.
#define BOOT_RESULT_KEPT           0x0000C7C7U  // 설치하지 않고 기존 앱을 그대로 실행했음을 의미합니다.
#define BOOT_RESULT_FAILED         0x0000DEADU  // 설치를 시도했으나 실패했음을 의미합니다.

#define BOOT_MAX_INSTALL_ATTEMPTS  3U  // 같은 이미지에 대해 허용하는 최대 설치 재시도 횟수입니다.

typedef struct  // 부트 제어 블록 구조체 정의를 시작합니다.
{  // 부트 제어 블록 멤버 선언을 시작합니다.
    uint32_t ulMagic;      // 블록 유효성 매직 값(BOOT_CTRL_MAGIC)을 저장합니다.
    uint32_t ulRequest;    // 앱이 부트로더에게 전달하는 요청 코드를 저장합니다.
    uint32_t ulAttempts;   // 부트로더가 설치를 시도한 횟수를 저장합니다. (무한 재시도 방지)
    uint32_t ulLastResult; // 부트로더가 마지막으로 수행한 작업의 결과를 저장합니다.
    uint32_t ulResetCount; // 부트로더를 통과한 부팅 횟수를 저장합니다. (진단용)
    uint32_t ulCheck;      // 위 5개 필드의 합 보수 체크섬을 저장합니다. (SRAM 손상 검출)
} BootCtrl_t;  // 부트 제어 블록 타입 이름을 BootCtrl_t 로 정의합니다.

#define BOOT_CTRL_PTR  ((volatile BootCtrl_t *)BOOT_CTRL_ADDR)  // 부트 제어 블록에 접근하는 포인터 매크로입니다.

// 부트 제어 블록의 체크섬을 계산합니다. (매직/요청/시도/결과/부팅횟수의 합의 2의 보수)
static inline uint32_t BootCtrl_CalcCheck(const volatile BootCtrl_t *pxCtrl)  // 체크섬 계산 인라인 함수를 정의합니다.
{  // 체크섬 계산 함수 본문을 시작합니다.
    uint32_t ulSum = pxCtrl->ulMagic;  // 매직 값을 합계에 더합니다.
    ulSum += pxCtrl->ulRequest;        // 요청 코드를 합계에 더합니다.
    ulSum += pxCtrl->ulAttempts;       // 시도 횟수를 합계에 더합니다.
    ulSum += pxCtrl->ulLastResult;     // 마지막 결과를 합계에 더합니다.
    ulSum += pxCtrl->ulResetCount;     // 부팅 횟수를 합계에 더합니다.
    return (uint32_t)(~ulSum + 1U);    // 합의 2의 보수를 체크섬으로 반환합니다.
}  // 체크섬 계산 함수를 종료합니다.

// 부트 제어 블록이 유효한지(매직 + 체크섬) 확인합니다.
static inline int BootCtrl_IsValid(const volatile BootCtrl_t *pxCtrl)  // 유효성 확인 인라인 함수를 정의합니다.
{  // 유효성 확인 함수 본문을 시작합니다.
    if (pxCtrl->ulMagic != BOOT_CTRL_MAGIC)  // 매직 값이 일치하지 않는지 확인합니다.
    {  // 매직 불일치 처리 블록을 시작합니다.
        return 0;  // 유효하지 않음(0)을 반환합니다.
    }  // 매직 불일치 처리 블록을 종료합니다.
    return (pxCtrl->ulCheck == BootCtrl_CalcCheck(pxCtrl)) ? 1 : 0;  // 체크섬 일치 여부를 반환합니다.
}  // 유효성 확인 함수를 종료합니다.

#endif /* FLASH_MAP_H */  // FLASH_MAP_H 조건부 컴파일 블록을 종료합니다.
