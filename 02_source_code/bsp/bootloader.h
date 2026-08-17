// bootloader.h — 부트로더 인터페이스 헤더 (골격)입니다.
// 펌웨어 업데이트 요청 확인/설정/플래싱을 담당합니다.
#ifndef BOOTLOADER_H  // BOOTLOADER_H 가 아직 정의되지 않았는지 확인합니다.
#define BOOTLOADER_H  // BOOTLOADER_H 를 정의하여 중복 포함을 방지합니다.

#include <stdint.h>  // uint32_t 등 고정 폭 정수 타입을 사용하기 위해 포함합니다.

typedef enum  // 펌웨어 상태 열거형 정의를 시작합니다.
{  // 펌웨어 상태 값 선언을 시작합니다.
    UPDATE_NONE = 0,     // 펌웨어 업데이트 요청이 없는 상태를 의미합니다.
    UPDATE_AVAILABLE     // 펌웨어 업데이트 요청이 존재하는 상태를 의미합니다.
} FirmwareState_t;  // 펌웨어 상태 타입 이름을 FirmwareState_t 로 정의합니다.

#define BOOTLOADER_APP_START_ADDR   0x08004000U  // 앱 영역 시작 주소(섹터 1)를 정의합니다.
#define BOOTLOADER_UPDATE_MAGIC     0x5555AAAAU  // 업데이트 요청 매직 값을 정의합니다.

void Bootloader_Init(void);  // 부트로더 인터페이스를 초기화하는 함수 프로토타입입니다.
FirmwareState_t Bootloader_CheckUpdateRequest(void);  // 업데이트 요청 여부를 확인하는 함수 프로토타입입니다.
void Bootloader_SetUpdateRequest(void);  // 업데이트 요청 플래그를 설정하는 함수 프로토타입입니다.
void Bootloader_ClearUpdateRequest(void);  // 업데이트 요청 플래그를 해제하는 함수 프로토타입입니다.
void Bootloader_FlashNewFirmware(void);  // 새 펌웨어를 플래시에 기록하는 함수 프로토타입입니다.

#endif /* BOOTLOADER_H */  // BOOTLOADER_H 조건부 컴파일 블록을 종료합니다.
