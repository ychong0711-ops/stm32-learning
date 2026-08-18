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
| `flash_map.h` | 플래시 영역 배치 · 부트 제어 블록 정의 | `BOOT_/APP_/STAGE_REGION_*`, `BootCtrl_t` |
| `fw_image.h/.c` | 이미지 헤더 · CRC-32 검증 | `FwImage_Crc32`, `FwImage_CheckHeader`, `FwImage_Verify` |
| `bootloader.h/.c` | 스테이징 기록 · 설치 요청 (앱 쪽) | `Bootloader_BeginStaging`, `WriteChunk`, `FinishStaging`, `VerifyStaged`, `RequestInstallAndReset` |
| `bsp_baremetal.h` | 부트로더용 최소 타입 정의 (FreeRTOS 대체) | `BaseType_t`, `pdPASS` 등 |
| `sensor_bmp280.h/.c` | BMP280 I2C 온도 | `BMP280_Init`, `BMP280_ReadTemperature` |
| `system_stm32.h/.c` | 180MHz 클록, NVIC 그룹 | `SystemClock_Config`, `NVIC_PriorityGroupConfig` |

---

## 2. main_fixed.c 의 중복 typedef (2026-08 처리 완료)

> **이 절은 이제 기록용입니다.** 아래 두 typedef 는 main_fixed.c 에서 이미
> 삭제했고, 자리에는 "어느 헤더가 정의하는지" 설명 주석이 들어가 있습니다.
> 지금은 `make` 만 하면 그대로 빌드됩니다.

BSP 헤더에서 타입을 정의하므로, main_fixed.c 에 같은 타입이 또 있으면
"redefinition" 컴파일 오류가 났습니다. (익명 struct/enum 의 typedef 재정의는
C11 에서도 허용되지 않습니다)

```c
// (삭제됨 1) main_fixed.c 의 CAN 메시지 구조체 → bsp_can.h 가 유일하게 정의
typedef struct { uint32_t ID; uint8_t DLC; uint8_t data[8]; } CAN_Message_t;

// (삭제됨 2) main_fixed.c 의 펌웨어 상태 열거형 → bootloader.h 가 유일하게 정의
typedef enum { UPDATE_NONE = 0, UPDATE_AVAILABLE } FirmwareState_t;
```

`FirmwareState_t` 는 값 구성은 그대로지만 의미가 정밀해졌습니다.
스테이징 패턴에서 `UPDATE_AVAILABLE` 은 "업데이트 요청이 있다"가 아니라
**"검증을 통과한 스테이징 이미지가 있어 설치할 수 있다"** 를 뜻합니다.

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

> **2026-08 갱신**: 이제 IDE 없이 `make` 로 빌드됩니다. 아래 1~5 단계(수동
> 프로젝트 구성)는 더 이상 필요 없습니다. 자세한 내용은 `../BUILD.md` 참조.

```bash
cd 02_source_code
make deps     # ST HAL + FreeRTOS 커널 내려받기 (최초 1회)
make          # 부트로더 + 앱 + 갱신 이미지
make size     # 영역별 사용량 확인
```

굽고 확인하기:

1. `make combined && make flash-combined` — 부트로더+앱 합본을 `0x08000000` 에 기록
2. 시리얼 터미널(115200)에서 디버그 JSON 확인
3. CAN 버스(USB-CAN)로 `0x200`(스로틀), `0x201`(팬), `0x123`(긴급정지) 전송 테스트

STM32CubeIDE 를 계속 쓰고 싶다면, 프로젝트에 `bsp/` · `app/` · `bootloader/` ·
`startup/` · `system/` 을 추가하고 `linker/` 의 스크립트를 지정하면 됩니다.
`FreeRTOSConfig.h` 는 이제 `app/FreeRTOSConfig.h` 에 실물이 있습니다
(아래 "3절"의 예시가 아니라 그 파일이 빌드에 쓰입니다).

---

## 6. 현재 골격의 한계 (실측 전 반드시 확인)

> **2026-08 해소된 항목**: startup 파일 · 링커 스크립트 · Makefile 부재,
> `HAL_Init()` 미호출, SysTick/IRQ 진입점 없음 — 전부 채워져서 실제로 링크됩니다.
> 애플리케이션이 자기 자신을 덮어쓰던 셀프 플래싱도 스테이징 방식으로 바뀌었습니다.
> (`../docs/docs_bootloader_design.md`)

| 항목 | 상태 | 보완 방법 |
|---|---|---|
| CAN 보레이트 타이밍 | 125k/250k/500k/1M 하드코딩 | 오실로스코프로 비트타임 검증 |
| IWDG LSI 주파수 | 32kHz 가정 (실제 17~47kHz) | 타임아웃 실측(E2 실험) 후 보정 |
| BMP280 보정 계수 | 온도만 구현 | 필요 시 압력 보정 추가 |
| 펌웨어 수신 경로 | 스테이징 API 만 존재 | CAN 분할/재전송/흐름제어 프로토콜 구현 |
| 이미지 서명 | CRC-32 만 (우연한 손상 탐지) | 악의적 변조 방어가 필요하면 ECDSA 서명 추가 |
| A/B 롤백 | 없음 (실패 시 기존 앱 유지) | 섹터 6~7(256KB) 이 비어 있어 확장 가능 |
| printf | 블로킹 전송 | 실측(CPU 부하) 후 인터럽트/DMA 전환 검토 |
| ADC 샘플링 | 480 사이클 (최대치) | 센서 특성에 맞게 조정 |

---

## 7. 실측 계획서와의 연결

- **측정 ③ 주기 지터** → `GPIO_Toggle(BSP_GPIO_PIN_MEAS_JITTER)` (센서 태스크 루프에 추가)
- **측정 ④ E2E 응답** → `bsp_can.c` 의 ISR 에 이미 `GPIO_Toggle(BSP_GPIO_PIN_MEAS_E2E)` 내장
- **측정 ⑤ 워치독 실험** → `IWDG_GetResetFlags()` 로 리셋 원인 기록
