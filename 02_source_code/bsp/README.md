# BSP 골격 코드 통합 가이드

`main_fixed.c`(FreeRTOS 예제, 수정판)가 호출하는 모든 BSP 함수를
**STM32CubeF4 HAL + FreeRTOS** 기준으로 구현한 골격 코드입니다.

- **대상 보드**: NUCLEO-F446RE (STM32F446RE, 180MHz)
- **필요 라이브러리**: STM32CubeF4 HAL, FreeRTOS (native), CMSIS

---

## 1. 파일 구성

| 파일 | 역할 | 핵심 API |
|---|---|---|
| `bsp.h` | 통합 헤더 | 전부 포함 |
| `bsp_can.h/.c` | CAN1 수신 (인터럽트+링버퍼+세마포어) | `CAN_Init`, `CAN_FilterConfig`, `CAN_Receive` |
| `bsp_adc.h/.c` | ADC1 폴링 읽기 | `ADC_Init`, `ADC_ReadChannel` |
| `bsp_pwm.h/.c` | TIM1 PWM (20kHz 등) | `PWM_Init`, `PWM_SetDuty` |
| `bsp_gpio.h/.c` | 논리핀→물리핀 매핑 GPIO | `GPIO_Init`, `GPIO_Write`, `GPIO_Toggle` |
| `bsp_uart.h/.c` | USART2 + printf 리타겟 | `UART_Init` |
| `bsp_iwdg.h/.c` | 독립 워치독 | `IWDG_Init`, `IWDG_ReloadCounter`, 리셋원인 |
| `bsp_flash.h/.c` | 내부 플래시 삭제/기록 | `Flash_Init`, `Flash_EraseSector`, `Flash_ProgramWord` |
| `bootloader.h/.c` | 업데이트 요청/플래싱 | `Bootloader_Init`, `Bootloader_CheckUpdateRequest`, `Bootloader_FlashNewFirmware` |
| `sensor_bmp280.h/.c` | BMP280 I2C 온도 | `BMP280_Init`, `BMP280_ReadTemperature` |
| `system_stm32.h/.c` | 180MHz 클록, NVIC 그룹 | `SystemClock_Config`, `NVIC_PriorityGroupConfig` |

---

## 2. main_fixed.c 통합 시 수정해야 할 2곳 (중요)

BSP 헤더에서 타입을 정의하므로, main_fixed.c 의 **중복 typedef 는 삭제**해야 합니다.
(삭제하지 않으면 "redefinition" 컴파일 오류 발생)

```c
// (삭제 대상 1) main_fixed.c 의 CAN 메시지 구조체 → bsp_can.h 로 이동됨
typedef struct { uint32_t ID; uint8_t DLC; uint8_t data[8]; } CAN_Message_t;

// (삭제 대상 2) main_fixed.c 의 펌웨어 상태 열거형 → bootloader.h 로 이동됨
typedef enum { UPDATE_NONE = 0, UPDATE_AVAILABLE } FirmwareState_t;
```

그리고 개별 `#include "bsp_xxx.h"` 들은 아래 한 줄로 대체 가능합니다.

```c
#include "bsp.h"
```

### 추가로 필요한 main_fixed.c 수정 (워치독 초기화)

IWDG 는 **시스템 클록 설정 직후, 스케줄러 시작 전에 1회** 시작해야 합니다.

```c
int main(void)
{
    SystemClock_Config();
    IWDG_Init(1000U);                       // ← 추가: 1초 타임아웃
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    ...
}
```

> 타임아웃(1000ms)은 워치독 태스크 검사 주기(100ms)의 10배로,
> 일시적 부하에도 오탐 리셋되지 않으면서 정지를 잡아내는 값입니다.

---

## 3. FreeRTOSConfig.h 필수 설정

```c
#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCPU_CLOCK_HZ                      ( SystemCoreClock )   // 180000000
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    ( 8 )   // Watchdog=6 이므로 7 이상
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 128 )
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) 30 * 1024 )

#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           1
#define configQUEUE_REGISTRY_SIZE               8
#define configUSE_TASK_NOTIFICATIONS            1

#define configCHECK_FOR_STACK_OVERFLOW          2      // 스택 오버플로 검사
#define configUSE_TRACE_FACILITY                1      // vTaskList() 용
#define configUSE_STATS_FORMATTING_FUNCTIONS    1      // 통계 포맷 용

#define configSUPPORT_DYNAMIC_ALLOCATION        1

/* ---- 인터럽트 우선순위 (STM32 4bit 그룹 기준, 반드시 확인) ---- */
#define configPRIO_BITS                         4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
#define configKERNEL_INTERRUPT_PRIORITY         ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

#define configASSERT( x )  if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }
```

> **핵심 규칙**: CAN RX 인터럽트 우선순위는 `bsp_can.c` 에서 **5** 로 설정되어 있습니다.
> `configMAX_SYSCALL_INTERRUPT_PRIORITY`(여기서는 5)보다 **작거나 같아야**
> 인터럽트 핸들러에서 `xSemaphoreGiveFromISR()` 등 FreeRTOS API 호출이 안전합니다.
> 우선순위를 바꿀 경우 두 곳을 함께 맞춰야 합니다.

---

## 4. 핀맵 요약 (NUCLEO-F446RE)

| 기능 | 페리퍼럴 | 핀 | AF |
|---|---|---|---|
| CAN1_RX | CAN1 | PB8 | AF9 |
| CAN1_TX | CAN1 | PB9 | AF9 |
| RPM 센서 ADC | ADC1_IN0 | PA0 | (아날로그) |
| 스로틀 ADC | ADC1_IN1 | PA1 | (아날로그) |
| 스로틀 PWM | TIM1_CH1 | PA8 | AF1 |
| 팬 GPIO | GPIO | PB13 | — |
| UART (디버그) | USART2 | PA2/PA3 | AF7 |
| BMP280 I2C | I2C1 | PB6/PB7 | AF4 |
| 측정: 지터 토글 | GPIO | PA4 | — |
| 측정: E2E 토글 | GPIO | PA7 | — |
| 오류 LED | GPIO | PA6 | — |

> 보드를 바꾸거나 회로를 직접 구성한다면 위 표와 각 BSP 소스의 핀 설정을
> 반드시 함께 수정하세요. (특히 CAN 트랜시버 3.3V 로직 호환 여부 확인)

---

## 5. 빌드 및 동작 확인 순서

1. STM32CubeIDE 프로젝트 생성 (보드: NUCLEO-F446RE, HAL 사용)
2. FreeRTOS 를 CMSIS-OS 없이 **native 소스**로 추가 (혹은 CubeMX 의 FreeRTOS 미들웨어 사용)
3. `bsp/` 폴더와 `main_fixed.c` 를 프로젝트에 추가
4. 위 "2절"의 typedef 2개 삭제 + `IWDG_Init` 호출 추가
5. `FreeRTOSConfig.h` 를 "3절" 기준으로 설정
6. 빌드 → 플래시 → 시리얼 터미널(115200)에서 디버그 JSON 확인
7. CAN 버스(USB-CAN)로 `0x200`(스로틀), `0x201`(팬), `0x123`(긴급정지) 전송 테스트

---

## 6. 현재 골격의 한계 (실측 전 반드시 확인)

| 항목 | 상태 | 보완 방법 |
|---|---|---|
| CAN 보레이트 타이밍 | 125k/250k/500k/1M 하드코딩 | 오실로스코프로 비트타임 검증 |
| IWDG LSI 주파수 | 32kHz 가정 (실제 17~47kHz) | 타임아웃 실측(E2 실험) 후 보정 |
| BMP280 보정 계수 | 온도만 구현 | 필요 시 압력 보정 추가 |
| 부트로더 이미지 소스 | 더미 배열 | UART/외부 플래시 전송 경로 구현 |
| printf | 블로킹 전송 | 실측(CPU 부하) 후 인터럽트/DMA 전환 검토 |
| ADC 샘플링 | 480 사이클 (최대치) | 센서 특성에 맞게 조정 |

---

## 7. 실측 계획서와의 연결

- **측정 ③ 주기 지터** → `GPIO_Toggle(BSP_GPIO_PIN_MEAS_JITTER)` (센서 태스크 루프에 추가)
- **측정 ④ E2E 응답** → `bsp_can.c` 의 ISR 에 이미 `GPIO_Toggle(BSP_GPIO_PIN_MEAS_E2E)` 내장
- **측정 ⑤ 워치독 실험** → `IWDG_GetResetFlags()` 로 리셋 원인 기록
