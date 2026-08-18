// stm32f4xx_it.c — 애플리케이션의 예외/인터럽트 진입점 모음입니다.
//
// startup_stm32f446xx.s 의 벡터 테이블은 모든 핸들러를 .weak + .thumb_set 으로
// Default_Handler 에 묶어 둡니다. 따라서 여기에 같은 이름의 강한(strong) 심볼을
// 정의하면 링커가 자동으로 이 함수들로 대체합니다. (startup 파일 수정 불필요)
//
// [기존 골격의 결함과 수정]
//   - bsp_can.c 는 HAL_CAN_RxFifo0MsgPendingCallback() 만 구현하고
//     CAN1_RX0_IRQHandler() 를 정의하지 않았습니다. 그 결과 CAN 수신 인터럽트는
//     Default_Handler 의 무한 루프로 떨어져 시스템이 그대로 멈춥니다.
//     아래에서 진입점을 정의해 HAL_CAN_IRQHandler() 로 연결합니다.
//   - SysTick_Handler() 가 어디에도 없어 HAL_Delay()/HAL_GetTick() 이 영원히
//     블로킹되고 FreeRTOS 틱도 발생하지 않았습니다. 아래에서 정의합니다.
#include "stm32f4xx_hal.h"  // HAL 틱/IRQ 처리 함수를 사용하기 위해 포함합니다.

#include "FreeRTOS.h"  // 커널 설정과 자료형을 사용하기 위해 포함합니다.
#include "task.h"      // 스케줄러 상태 조회 API 를 사용하기 위해 포함합니다.

#include "bsp_can.h"  // CAN1 HAL 핸들 접근자 CAN_GetHandle() 을 사용하기 위해 포함합니다.

// FreeRTOS 포트(port.c)가 제공하는 틱 핸들러입니다. 헤더에 노출되지 않아 직접 선언합니다.
extern void xPortSysTickHandler(void);  // 커널 틱 처리 함수를 외부 선언합니다.

// --------------------------------------------------------------------------
// 코어 예외 핸들러
//
// SVC_Handler / PendSV_Handler 는 FreeRTOSConfig.h 의 이름 매핑을 통해
// port.c 의 vPortSVCHandler / xPortPendSVHandler 가 그대로 제공합니다.
// 여기서 정의하면 중복 정의로 링크 오류가 나므로 정의하지 않습니다.
// --------------------------------------------------------------------------

void NMI_Handler(void)  // NMI 예외 핸들러를 정의합니다.
{  // NMI 핸들러 본문을 시작합니다.
    // HSE 클록 감시(CSS)가 동작하면 NMI 로 진입합니다. 여기서 멈추면 워치독이
    // 시스템을 리셋하므로, 안전측 동작으로 그대로 정지합니다.
    for (;;)  // 무한 루프에 진입합니다.
    {  // NMI 무한 루프 본문을 시작합니다.
    }  // NMI 무한 루프 본문을 종료합니다.
}  // NMI 핸들러를 종료합니다.

// 하드폴트 발생 시 예외 스택 프레임을 전역에 남겨 디버거로 원인을 추적합니다.
volatile uint32_t ulHardFaultR0;   // 폴트 시점의 R0 값을 보관합니다.
volatile uint32_t ulHardFaultR1;   // 폴트 시점의 R1 값을 보관합니다.
volatile uint32_t ulHardFaultR2;   // 폴트 시점의 R2 값을 보관합니다.
volatile uint32_t ulHardFaultR3;   // 폴트 시점의 R3 값을 보관합니다.
volatile uint32_t ulHardFaultR12;  // 폴트 시점의 R12 값을 보관합니다.
volatile uint32_t ulHardFaultLR;   // 폴트 시점의 LR(복귀 주소) 값을 보관합니다.
volatile uint32_t ulHardFaultPC;   // 폴트를 일으킨 명령어 주소를 보관합니다.
volatile uint32_t ulHardFaultPSR;  // 폴트 시점의 xPSR 값을 보관합니다.
volatile uint32_t ulHardFaultCFSR; // 설정 가능 폴트 상태 레지스터 값을 보관합니다.
volatile uint32_t ulHardFaultHFSR; // 하드폴트 상태 레지스터 값을 보관합니다.
volatile uint32_t ulHardFaultMMAR; // 메모리 관리 폴트 주소를 보관합니다.
volatile uint32_t ulHardFaultBFAR; // 버스 폴트 주소를 보관합니다.

// 예외 스택 프레임을 읽어 전역에 복사한 뒤 정지하는 C 레벨 핸들러입니다.
void vHardFaultReport(uint32_t *pulStackFrame)  // 하드폴트 상세 기록 함수를 정의합니다.
{  // 하드폴트 상세 기록 함수 본문을 시작합니다.
    ulHardFaultR0 = pulStackFrame[0];   // 스택 프레임에서 R0 를 읽어 저장합니다.
    ulHardFaultR1 = pulStackFrame[1];   // 스택 프레임에서 R1 을 읽어 저장합니다.
    ulHardFaultR2 = pulStackFrame[2];   // 스택 프레임에서 R2 를 읽어 저장합니다.
    ulHardFaultR3 = pulStackFrame[3];   // 스택 프레임에서 R3 을 읽어 저장합니다.
    ulHardFaultR12 = pulStackFrame[4];  // 스택 프레임에서 R12 를 읽어 저장합니다.
    ulHardFaultLR = pulStackFrame[5];   // 스택 프레임에서 LR 을 읽어 저장합니다.
    ulHardFaultPC = pulStackFrame[6];   // 스택 프레임에서 PC 를 읽어 저장합니다.
    ulHardFaultPSR = pulStackFrame[7];  // 스택 프레임에서 xPSR 을 읽어 저장합니다.

    ulHardFaultCFSR = SCB->CFSR;  // 설정 가능 폴트 상태 레지스터를 저장합니다.
    ulHardFaultHFSR = SCB->HFSR;  // 하드폴트 상태 레지스터를 저장합니다.
    ulHardFaultMMAR = SCB->MMFAR; // 메모리 관리 폴트 주소를 저장합니다.
    ulHardFaultBFAR = SCB->BFAR;  // 버스 폴트 주소를 저장합니다.

    // 여기서 정지합니다. IWDG 를 피드하지 않으므로 설정된 타임아웃 후 자동 리셋되고,
    // 앱이 반복해서 폴트를 내면 부트로더가 재시도 한계를 보고 복구 루프로 넘어갑니다.
    for (;;)  // 무한 루프에 진입합니다.
    {  // 하드폴트 무한 루프 본문을 시작합니다.
    }  // 하드폴트 무한 루프 본문을 종료합니다.
}  // 하드폴트 상세 기록 함수를 종료합니다.

// 예외 진입 시 사용된 스택(MSP/PSP)을 골라 vHardFaultReport 에 넘기는 어셈블리 트램폴린입니다.
__attribute__((naked)) void HardFault_Handler(void)  // 하드폴트 핸들러를 네이키드 함수로 정의합니다.
{  // 하드폴트 핸들러 본문을 시작합니다.
    __asm volatile  // 인라인 어셈블리 블록을 시작합니다.
    (  // 어셈블리 문자열 목록을 시작합니다.
        " tst lr, #4            \n"  // EXC_RETURN 의 bit2 로 사용된 스택을 판별합니다.
        " ite eq                \n"  // 조건 실행 블록을 준비합니다.
        " mrseq r0, msp         \n"  // bit2 가 0 이면 MSP 를 인자로 넘깁니다.
        " mrsne r0, psp         \n"  // bit2 가 1 이면 PSP 를 인자로 넘깁니다.
        " ldr r1, =vHardFaultReport \n"  // C 핸들러 주소를 로드합니다.
        " bx r1                 \n"  // C 핸들러로 분기합니다. (복귀하지 않음)
    );  // 어셈블리 문자열 목록을 종료합니다.
}  // 하드폴트 핸들러를 종료합니다.

void MemManage_Handler(void)  // 메모리 관리 폴트 핸들러를 정의합니다.
{  // 메모리 관리 폴트 핸들러 본문을 시작합니다.
    for (;;)  // 무한 루프에 진입합니다.
    {  // 메모리 관리 폴트 루프 본문을 시작합니다.
    }  // 메모리 관리 폴트 루프 본문을 종료합니다.
}  // 메모리 관리 폴트 핸들러를 종료합니다.

void BusFault_Handler(void)  // 버스 폴트 핸들러를 정의합니다.
{  // 버스 폴트 핸들러 본문을 시작합니다.
    for (;;)  // 무한 루프에 진입합니다.
    {  // 버스 폴트 루프 본문을 시작합니다.
    }  // 버스 폴트 루프 본문을 종료합니다.
}  // 버스 폴트 핸들러를 종료합니다.

void UsageFault_Handler(void)  // 사용 폴트 핸들러를 정의합니다.
{  // 사용 폴트 핸들러 본문을 시작합니다.
    for (;;)  // 무한 루프에 진입합니다.
    {  // 사용 폴트 루프 본문을 시작합니다.
    }  // 사용 폴트 루프 본문을 종료합니다.
}  // 사용 폴트 핸들러를 종료합니다.

void DebugMon_Handler(void)  // 디버그 모니터 핸들러를 정의합니다.
{  // 디버그 모니터 핸들러 본문을 시작합니다.
}  // 디버그 모니터 핸들러를 종료합니다.

// --------------------------------------------------------------------------
// SysTick — HAL 타임베이스와 FreeRTOS 틱의 공유
//
// [왜 FreeRTOSConfig.h 에서 xPortSysTickHandler 로 이름 매핑을 하지 않는가]
//   흔한 STM32+FreeRTOS 실수는 SysTick_Handler 를 xPortSysTickHandler 로 직접
//   매핑해 버리는 것입니다. 그러면 HAL_IncTick() 이 호출되지 않아 uwTick 이 멈추고,
//   HAL_Delay()/HAL 드라이버의 타임아웃(HAL_UART_Transmit, HAL_RCC_OscConfig,
//   HAL_FLASHEx_Erase 등)이 전부 무한 대기에 빠집니다.
//   sensor_bmp280.c 의 HAL_Delay(10) 두 곳이 정확히 이 함정에 걸립니다.
//   따라서 진입점을 여기에 두고 HAL → 커널 순서로 둘 다 호출합니다.
//
// [스케줄러 시작 전 보호]
//   HAL_Init() 이 SysTick 을 먼저 켜므로 vTaskStartScheduler() 이전에도 틱이
//   발생합니다. 그 시점에 xPortSysTickHandler() 를 부르면 아직 초기화되지 않은
//   커널 자료구조를 건드립니다. 스케줄러 상태를 확인해 그 전에는 HAL 만 갱신합니다.
// --------------------------------------------------------------------------
void SysTick_Handler(void)  // SysTick 예외 핸들러를 정의합니다.
{  // SysTick 핸들러 본문을 시작합니다.
    HAL_IncTick();  // HAL 의 1ms 틱 카운터(uwTick)를 증가시킵니다.

    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)  // 스케줄러가 시작되었는지 확인합니다.
    {  // 커널 틱 처리 블록을 시작합니다.
        xPortSysTickHandler();  // FreeRTOS 틱을 처리하고 필요하면 문맥 전환을 요청합니다.
    }  // 커널 틱 처리 블록을 종료합니다.
}  // SysTick 핸들러를 종료합니다.

// --------------------------------------------------------------------------
// 주변장치 인터럽트
// --------------------------------------------------------------------------

void CAN1_RX0_IRQHandler(void)  // CAN1 수신 FIFO0 인터럽트 진입점을 정의합니다.
{  // CAN1 RX0 인터럽트 핸들러 본문을 시작합니다.
    // HAL_CAN_IRQHandler() 가 인터럽트 플래그를 해석해
    // HAL_CAN_RxFifo0MsgPendingCallback()(bsp_can.c 에 구현) 을 호출합니다.
    // CAN_GetHandle() 은 HAL 헤더 의존을 피하려고 void* 를 반환하므로 캐스팅합니다.
    HAL_CAN_IRQHandler((CAN_HandleTypeDef *)CAN_GetHandle());  // CAN1 인터럽트를 HAL 에 위임합니다.
}  // CAN1 RX0 인터럽트 핸들러를 종료합니다.
