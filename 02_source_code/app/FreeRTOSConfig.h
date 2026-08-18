// FreeRTOSConfig.h — FreeRTOS 커널 설정 (STM32F446RE / Cortex-M4F / ARM_CM4F 포트)
//
// bsp/README.md "3. FreeRTOSConfig.h 필수 설정" 절의 내용을 실제 빌드 가능한 형태로
// 구현한 파일입니다. 여기서 정한 값은 다음과 정합해야 합니다.
//   - configMAX_PRIORITIES(8) ≥ 최고 태스크 우선순위(Watchdog=6) + 1
//   - configMAX_SYSCALL_INTERRUPT_PRIORITY(5) ≥ bsp_can.c 의 CAN1_RX0 우선순위(5)
//   - configCPU_CLOCK_HZ = system_stm32.c 의 SystemClock_Config() 결과(180MHz)
#ifndef FREERTOS_CONFIG_H  // FREERTOS_CONFIG_H 가 아직 정의되지 않았는지 확인합니다.
#define FREERTOS_CONFIG_H  // FREERTOS_CONFIG_H 를 정의하여 중복 포함을 방지합니다.

#ifndef __IASMARM__  // 어셈블러가 이 파일을 전처리할 때는 C 선언을 제외합니다.
    #include <stdint.h>  // uint32_t 등 고정 폭 정수 타입을 사용하기 위해 포함합니다.
    extern uint32_t SystemCoreClock;  // system_stm32f4xx.c 가 갱신하는 현재 코어 클록 변수입니다.
#endif  // 어셈블러 분기를 종료합니다.

// --------------------------------------------------------------------------
// 스케줄링 기본 설정
// --------------------------------------------------------------------------
#define configUSE_PREEMPTION                    1  // 선점형 스케줄링을 사용합니다. (RMS 우선순위 설계의 전제)
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1  // Cortex-M 의 CLZ 명령으로 O(1) 태스크 선택을 사용합니다.
#define configUSE_TIME_SLICING                  1  // 같은 우선순위 태스크 간 라운드로빈을 허용합니다. (Sensor/Actuator 가 동일 우선순위 3)
#define configUSE_IDLE_HOOK                     0  // 아이들 훅을 사용하지 않습니다.
#define configUSE_TICK_HOOK                     0  // 틱 훅을 사용하지 않습니다.
#define configCPU_CLOCK_HZ                      ( SystemCoreClock )  // 코어 클록을 런타임 값(180MHz)으로 참조합니다.
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )  // 틱 주기를 1ms 로 설정합니다.
#define configMAX_PRIORITIES                    ( 8 )  // 우선순위 단계 수입니다. (Watchdog=6 이므로 7 이상 필요)
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 128 )  // 아이들 태스크의 스택 크기(워드)입니다.
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 30 * 1024 ) )  // heap_4 가 관리할 힙 크기입니다.
#define configMAX_TASK_NAME_LEN                 ( 12 )  // 태스크 이름의 최대 길이입니다.
#define configUSE_16_BIT_TICKS                  0  // 32비트 틱 카운터를 사용합니다.
#define configIDLE_SHOULD_YIELD                 1  // 아이들 태스크가 동일 우선순위 태스크에 양보하도록 합니다.

// --------------------------------------------------------------------------
// 동기화 객체
// --------------------------------------------------------------------------
#define configUSE_MUTEXES                       1  // 뮤텍스를 사용합니다. (xSemaphore_Actuator, xMutex_Debug)
#define configUSE_RECURSIVE_MUTEXES             0  // 재귀 뮤텍스는 사용하지 않습니다.
#define configUSE_COUNTING_SEMAPHORES           1  // 카운팅 세마포어를 사용합니다.
#define configUSE_BINARY_SEMAPHORES             1  // 바이너리 세마포어를 사용합니다. (CAN RX ISR → 태스크 통지)
#define configQUEUE_REGISTRY_SIZE               8  // 디버거에 표시할 큐 등록 슬롯 수입니다.
#define configUSE_QUEUE_SETS                    0  // 큐 세트는 사용하지 않습니다.
#define configUSE_TASK_NOTIFICATIONS            1  // 태스크 노티피케이션을 사용합니다.
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   1  // 태스크당 노티피케이션 슬롯 개수입니다.

// --------------------------------------------------------------------------
// 진단 / 안전
// --------------------------------------------------------------------------
#define configCHECK_FOR_STACK_OVERFLOW          2  // 방식 2(패턴 검사) 스택 오버플로 검사를 사용합니다.
#define configUSE_MALLOC_FAILED_HOOK            1  // 힙 할당 실패 훅을 사용합니다.
#define configUSE_TRACE_FACILITY                1  // uxTaskGetSystemState() 등 추적 기능을 활성화합니다.
#define configUSE_STATS_FORMATTING_FUNCTIONS    1  // vTaskList() 등 통계 포맷 함수를 활성화합니다.
#define configGENERATE_RUN_TIME_STATS           0  // 런타임 통계는 별도 타이머가 필요하므로 비활성화합니다.
#define configRECORD_STACK_HIGH_ADDRESS         1  // 스택 상한 주소를 기록해 정적 스택 분석과 대조할 수 있게 합니다.

// --------------------------------------------------------------------------
// 메모리 할당
// --------------------------------------------------------------------------
#define configSUPPORT_DYNAMIC_ALLOCATION        1  // 동적 할당(xTaskCreate 등)을 사용합니다.
#define configSUPPORT_STATIC_ALLOCATION         0  // 정적 할당 API 는 사용하지 않습니다.
#define configAPPLICATION_ALLOCATED_HEAP        0  // 힙 배열을 커널이 직접 정의하도록 합니다.

// --------------------------------------------------------------------------
// 소프트웨어 타이머 / 코루틴
// --------------------------------------------------------------------------
#define configUSE_TIMERS                        0  // 소프트웨어 타이머를 사용하지 않습니다. (태스크만으로 주기 제어)
#define configUSE_CO_ROUTINES                   0  // 코루틴을 사용하지 않습니다.
#define configMAX_CO_ROUTINE_PRIORITIES         ( 2 )  // 코루틴 우선순위 단계 수입니다. (미사용)

// --------------------------------------------------------------------------
// 선택적 API 포함 여부
// --------------------------------------------------------------------------
#define INCLUDE_vTaskPrioritySet                1  // vTaskPrioritySet() 을 포함합니다.
#define INCLUDE_uxTaskPriorityGet               1  // uxTaskPriorityGet() 을 포함합니다.
#define INCLUDE_vTaskDelete                     1  // vTaskDelete() 를 포함합니다.
#define INCLUDE_vTaskSuspend                    1  // vTaskSuspend()/vTaskResume() 를 포함합니다. (펌웨어 업데이트에서 사용)
// 주기 태스크의 지터를 억제하는 vTaskDelayUntil() 을 포함합니다.
// 최신 커널에서는 xTaskDelayUntil() 이 정식 이름이고, INCLUDE_xTaskDelayUntil 만
// 켜면 vTaskDelayUntil() 은 매크로로 자동 제공됩니다. 두 매크로를 동시에 켜면
// 커널이 #error 로 거부하므로(FreeRTOS.h) 신규 이름 하나만 켭니다.
#define INCLUDE_xTaskDelayUntil                 1  // xTaskDelayUntil() 및 vTaskDelayUntil() 호환 매크로를 포함합니다.
#define INCLUDE_vTaskDelay                      1  // vTaskDelay() 를 포함합니다.
#define INCLUDE_xTaskGetSchedulerState          1  // xTaskGetSchedulerState() 를 포함합니다.
#define INCLUDE_uxTaskGetStackHighWaterMark     1  // 스택 여유 측정 API 를 포함합니다. (정적 분석 결과 검증용)
#define INCLUDE_xTaskGetCurrentTaskHandle       1  // 현재 태스크 핸들 조회 API 를 포함합니다.
#define INCLUDE_eTaskGetState                   1  // 태스크 상태 조회 API 를 포함합니다.

// --------------------------------------------------------------------------
// 인터럽트 우선순위 (Cortex-M4, STM32 는 상위 4비트만 구현)
//
// [매우 중요] FromISR API 를 호출하는 ISR 의 우선순위 숫자는
//             configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY(5) 이상이어야 합니다.
//             (Cortex-M 에서는 숫자가 클수록 우선순위가 낮습니다)
//             bsp_can.c 의 HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 5, 0) 과 정합합니다.
// --------------------------------------------------------------------------
#define configPRIO_BITS                         4  // STM32F4 가 구현한 우선순위 비트 수입니다.
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15  // 가장 낮은 우선순위 값입니다. (PendSV/SysTick 용)
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5  // FromISR 호출이 허용되는 가장 높은 우선순위 값입니다.
#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )  // 커널 인터럽트 우선순위를 8비트 필드로 변환합니다.
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8 - configPRIO_BITS ) )  // 최대 시스콜 우선순위를 8비트 필드로 변환합니다.

// --------------------------------------------------------------------------
// 예외 핸들러 이름 매핑
//
// startup_stm32f446xx.s 의 벡터 테이블은 SVC_Handler / PendSV_Handler /
// SysTick_Handler 이름을 참조합니다. FreeRTOS 포트는 vPortSVCHandler /
// xPortPendSVHandler / xPortSysTickHandler 로 정의하므로 여기서 이름을 맞춥니다.
// (port.c 가 실행 시 벡터 테이블에 실제로 이 함수들이 등록되었는지 configASSERT 로 검증합니다)
// --------------------------------------------------------------------------
#define vPortSVCHandler                         SVC_Handler  // SVC 예외를 FreeRTOS 핸들러로 연결합니다.
#define xPortPendSVHandler                      PendSV_Handler  // PendSV 예외를 FreeRTOS 문맥 전환 핸들러로 연결합니다.
// SysTick 은 이름을 매핑하지 않습니다. HAL 과 커널이 같은 타이머를 공유해야 하므로
// app/stm32f4xx_it.c 의 SysTick_Handler() 가 HAL_IncTick() 과 xPortSysTickHandler() 를
// 순서대로 호출합니다. (자세한 이유는 그 파일의 주석 참조)

// --------------------------------------------------------------------------
// configASSERT
// 실패 시 인터럽트를 막고 정지시켜 디버거로 원인을 추적할 수 있게 합니다.
// --------------------------------------------------------------------------
#ifndef __IASMARM__  // C 코드에서만 configASSERT 를 정의합니다.
    extern void vApplicationAssertFailed( const char *pcFile, unsigned long ulLine );  // 어서션 실패 처리 함수를 선언합니다.
    #define configASSERT( x ) \
        if( ( x ) == 0 ) { vApplicationAssertFailed( __FILE__, __LINE__ ); }  // 조건이 거짓이면 어서션 처리 함수를 호출합니다.
#endif  // configASSERT 정의 분기를 종료합니다.

#endif /* FREERTOS_CONFIG_H */  // FREERTOS_CONFIG_H 조건부 컴파일 블록을 종료합니다.
