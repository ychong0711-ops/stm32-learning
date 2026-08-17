// bsp_can.h — CAN 통신 BSP 헤더 (STM32F4 HAL 기반)입니다.
// 사용 CAN 페리퍼럴은 CAN1 이며, 수신은 인터럽트 + 링 버퍼 + 세마포어 구조입니다.
#ifndef BSP_CAN_H  // BSP_CAN_H 가 아직 정의되지 않았는지 확인합니다.
#define BSP_CAN_H  // BSP_CAN_H 를 정의하여 중복 포함을 방지합니다.

#include <stdint.h>    // uint8_t, uint32_t 등 고정 폭 정수 타입을 사용하기 위해 포함합니다.
#include "FreeRTOS.h"  // BaseType_t 등 FreeRTOS 공통 타입을 사용하기 위해 포함합니다.
#include "task.h"      // pdMS_TO_TICKS 등 태스크 관련 매크로를 사용하기 위해 포함합니다.

typedef struct  // CAN 메시지 구조체 정의를 시작합니다.
{  // CAN 메시지 멤버 변수 선언을 시작합니다.
    uint32_t ID;     // CAN 메시지 식별자(표준 11bit 또는 확장 29bit)를 저장합니다.
    uint8_t  DLC;    // CAN 데이터 길이 코드(0~8)를 저장합니다.
    uint8_t  data[8]; // CAN 페이로드 최대 8바이트를 저장합니다.
} CAN_Message_t;  // CAN 메시지 구조체 타입 이름을 CAN_Message_t 로 정의합니다.

BaseType_t CAN_Init(uint32_t ulBaudrate);  // CAN 컨트롤러를 지정 보레이트로 초기화하는 함수 프로토타입입니다.
void CAN_FilterConfig(uint32_t ulFilterId, uint32_t ulFilterMask);  // 수신 필터를 설정하는 함수 프로토타입입니다.
BaseType_t CAN_Receive(CAN_Message_t *pxMsg, TickType_t xTimeout);  // CAN 메시지를 타임아웃 대기로 수신하는 함수 프로토타입입니다.

#endif /* BSP_CAN_H */  // BSP_CAN_H 조건부 컴파일 블록을 종료합니다.
