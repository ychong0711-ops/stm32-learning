// app_hooks.c — FreeRTOS 훅 함수와 어서션 처리 구현입니다.
//
// FreeRTOSConfig.h 에서 아래 세 가지를 켜 두었으므로, 대응 함수를 반드시
// 제공해야 링크가 됩니다. 골격 코드에는 이 구현이 없어 링크 단계에서
// undefined reference 가 발생했습니다.
//   configCHECK_FOR_STACK_OVERFLOW = 2  → vApplicationStackOverflowHook()
//   configUSE_MALLOC_FAILED_HOOK   = 1  → vApplicationMallocFailedHook()
//   configASSERT()                      → vApplicationAssertFailed()
//
// 실패 정보는 전역 변수에 남깁니다. 여기서 정지하면 워치독이 시스템을 리셋하고,
// 리셋 후에는 IWDG_GetResetFlags() 로 "워치독 리셋"임을 확인할 수 있습니다.
#include <stdint.h>  // 고정 폭 정수 타입을 사용하기 위해 포함합니다.

#include "stm32f4xx_hal.h"  // __disable_irq() 등 CMSIS 인트린식을 사용하기 위해 포함합니다.

#include "FreeRTOS.h"  // 커널 설정과 자료형을 사용하기 위해 포함합니다.
#include "task.h"      // TaskHandle_t 등 태스크 API 를 사용하기 위해 포함합니다.

// 진단용 전역입니다. 디버거로 정지 지점의 원인을 바로 확인할 수 있습니다.
volatile const char *pcLastFailFile = 0;   // 마지막 실패가 발생한 파일 이름입니다.
volatile unsigned long ulLastFailLine = 0; // 마지막 실패가 발생한 줄 번호입니다.
volatile const char *pcLastFailTask = 0;   // 마지막 실패와 관련된 태스크 이름입니다.
volatile uint32_t ulFailKind = 0;          // 실패 종류입니다. (1=assert, 2=stack, 3=malloc)

#define APP_FAIL_ASSERT   1U  // configASSERT 실패를 나타내는 코드입니다.
#define APP_FAIL_STACK    2U  // 스택 오버플로를 나타내는 코드입니다.
#define APP_FAIL_MALLOC   3U  // 힙 할당 실패를 나타내는 코드입니다.

// --------------------------------------------------------------------------
// configASSERT 실패 처리
// --------------------------------------------------------------------------
void vApplicationAssertFailed(const char *pcFile, unsigned long ulLine)  // 어서션 실패 처리 함수를 정의합니다.
{  // 어서션 실패 처리 함수 본문을 시작합니다.
    pcLastFailFile = pcFile;   // 실패한 파일 이름을 기록합니다.
    ulLastFailLine = ulLine;   // 실패한 줄 번호를 기록합니다.
    ulFailKind = APP_FAIL_ASSERT;  // 실패 종류를 어서션으로 기록합니다.

    taskDISABLE_INTERRUPTS();  // 추가 손상을 막기 위해 인터럽트를 차단합니다.
    for (;;)  // 무한 루프에 진입하여 워치독 리셋을 기다립니다.
    {  // 어서션 정지 루프 본문을 시작합니다.
    }  // 어서션 정지 루프 본문을 종료합니다.
}  // 어서션 실패 처리 함수를 종료합니다.

// --------------------------------------------------------------------------
// 스택 오버플로 훅 (configCHECK_FOR_STACK_OVERFLOW = 2)
//
// 문맥 전환 때마다 커널이 태스크 스택 끝의 패턴이 훼손되었는지 검사하고,
// 훼손되었으면 이 함수를 호출합니다. 03_static_proof 의 정적 스택 분석 결과를
// 런타임에서 교차 검증하는 안전망 역할을 합니다.
// --------------------------------------------------------------------------
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)  // 스택 오버플로 훅을 정의합니다.
{  // 스택 오버플로 훅 본문을 시작합니다.
    (void)xTask;  // 태스크 핸들은 사용하지 않으므로 경고를 방지합니다.

    pcLastFailTask = pcTaskName;  // 오버플로를 낸 태스크 이름을 기록합니다.
    ulFailKind = APP_FAIL_STACK;  // 실패 종류를 스택 오버플로로 기록합니다.

    taskDISABLE_INTERRUPTS();  // 이미 스택이 깨졌으므로 인터럽트를 차단합니다.
    for (;;)  // 무한 루프에 진입하여 워치독 리셋을 기다립니다.
    {  // 스택 오버플로 정지 루프 본문을 시작합니다.
    }  // 스택 오버플로 정지 루프 본문을 종료합니다.
}  // 스택 오버플로 훅을 종료합니다.

// --------------------------------------------------------------------------
// 힙 할당 실패 훅 (configUSE_MALLOC_FAILED_HOOK = 1)
//
// pvPortMalloc() 실패 시 호출됩니다. 태스크/큐/세마포어 생성은 모두 전원 투입
// 직후에만 일어나므로, 이 훅이 불린다면 configTOTAL_HEAP_SIZE 부족입니다.
// --------------------------------------------------------------------------
void vApplicationMallocFailedHook(void)  // 힙 할당 실패 훅을 정의합니다.
{  // 힙 할당 실패 훅 본문을 시작합니다.
    ulFailKind = APP_FAIL_MALLOC;  // 실패 종류를 힙 할당 실패로 기록합니다.

    taskDISABLE_INTERRUPTS();  // 일관된 상태를 유지하기 위해 인터럽트를 차단합니다.
    for (;;)  // 무한 루프에 진입하여 워치독 리셋을 기다립니다.
    {  // 힙 할당 실패 정지 루프 본문을 시작합니다.
    }  // 힙 할당 실패 정지 루프 본문을 종료합니다.
}  // 힙 할당 실패 훅을 종료합니다.

// --------------------------------------------------------------------------
// HAL 어서션 (stm32f4xx_hal_conf.h 에서 USE_FULL_ASSERT 를 켠 경우에만 필요)
// --------------------------------------------------------------------------
#ifdef USE_FULL_ASSERT  // HAL 전체 어서션이 켜져 있는지 확인합니다.
void assert_failed(uint8_t *file, uint32_t line)  // HAL 파라미터 검사 실패 함수를 정의합니다.
{  // HAL 어서션 실패 함수 본문을 시작합니다.
    vApplicationAssertFailed((const char *)file, (unsigned long)line);  // 공통 어서션 처리로 넘깁니다.
}  // HAL 어서션 실패 함수를 종료합니다.
#endif  // USE_FULL_ASSERT 분기를 종료합니다.
