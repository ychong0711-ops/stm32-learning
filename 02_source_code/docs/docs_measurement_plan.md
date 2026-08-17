# FreeRTOS 임베디드 프로젝트 실측 계획서 (Measurement Plan)

> 대상: `main_fixed.c` — CAN + FreeRTOS + STM32 임베디드 애플리케이션 (수정판)
> 목적: 코드의 품질을 **수치로 증명**하여 "동작한다"가 아니라 "측정되고 검증되었다"를 문서화
> 작성일: 2026-08-17 | 버전: v1.0

---

## 0. 왜 실측이 필요한가 (이 프로젝트의 경우)

현재 코드는 BSP 함수들이 "존재한다고 가정"한 **시뮬레이션 수준**입니다. 독일 자동차 임베디드(및 대학원 지원) 관점에서 이 코드가 가치를 가지려면 다음 질문에 **숫자로 답할 수 있어야** 합니다.

| 질문 | 답을 주는 측정 항목 |
|---|---|
| 스택 크기를 감으로 잡았는데 충분한가? | ① 스택 사용량 (High Water Mark) |
| 우선순위 배정이 CPU를 낭비하거나 태스크를 굶기지 않는가? | ② CPU 부하 (Run-Time Stats) |
| "10ms 주기"가 실제로 얼마나 정확한가? | ③ 주기 지터 (Jitter) |
| 긴급 정지가 "즉시"라는 말의 실체는 몇 ms인가? | ④ End-to-End 응답시간 |
| 워치독이 정말 고장을 잡아내는가? | ⑤ 워치독 유효성 실험 |

이 5가지를 측정·기록하면, 이 프로젝트는 **"ISO 26262 검증 프로세스의 축소판"**이 됩니다. 그 자체가 독일 자동차 업계와 대학원이 원하는 신호입니다.

---

## 1. 측정 환경 및 장비

### 1.1 하드웨어 구성 (BOM)

| 항목 | 추천 부품 | 선정 근거 | 대략 비용 |
|---|---|---|---|
| MCU 보드 | NUCLEO-F446RE (STM32F446RE) | 180MHz, CAN1/CAN2 내장, 저렴, J-Link 전환 가능 | ~3만원 |
| (대안) MCU 보드 | NUCLEO-F767ZI | 216MHz, RAM 512KB — SystemView/대형 버퍼에 유리 | ~6만원 |
| CAN 트랜시버 | SN65HVD230 모듈 | 3.3V 로직 호환, Nucleo와 직결 | ~5천원 |
| USB-CAN 어댑터 | PCAN-USB (PEAK) 또는 Canable(CandleLight FW) | PCAN-View는 산업 표준 분석 SW | ~3천~30만원 |
| 로직 애널라이저 | 8ch 24MHz (DSLogic/저가형) | 지터·응답시간 측정의 핵심 | ~2만원 |
| 오실로스코프 | (선택) 필수 아님 | 로직 애널라이저로 대부분 대체 가능 | — |
| 전원/기타 | USB + 점퍼선 | — | — |

> 💡 **J-Link 준비**: SEGGER SystemView(CPU 부하 시각화)는 J-Link가 필요합니다. Nucleo의 ST-Link는 SEGGER의 **STLinkReflash** 유틸리티로 J-Link OB 펌웨어로 전환할 수 있어서 추가 비용 없이 사용 가능합니다.

### 1.2 툴체인 / 소프트웨어

| 구분 | 항목 | 버전 기록 (템플릿에 명시) |
|---|---|---|
| IDE | STM32CubeIDE | 예: 1.15.0 |
| 컴파일러 | arm-none-eabi-gcc (CubeIDE 내장) | 예: 12.3.1 |
| RTOS | FreeRTOS (native, CMSIS-OS 아님) | 예: 10.6.2 |
| HAL | STM32CubeF4 | 예: 1.28.0 |
| 분석 | SEGGER SystemView | 예: 3.52 |
| CAN 분석 | PCAN-View (또는 cangaroo) | 예: 4.x |
| 로직 분석 | DSView / Sigrok-PulseView | — |

> 모든 버전을 기록해 두세요. 독일식 문서에서 "재현 가능성(Reproducibility)"은 품질의 기본 요건입니다.

---

## 2. 측정 항목 요약 및 합격 기준

| # | 측정 항목 | 도구 | 합격 기준 (제안) |
|---|---|---|---|
| ① | 태스크별 스택 사용량 | `uxTaskGetStackHighWaterMark()` + `-fstack-usage` | 여유 ≥ 30% (최대 사용 ≤ 스택의 70%) |
| ② | CPU 부하 (태스크별/전체) | Run-Time Stats + SystemView | 평균 ≤ 70%, 피크 ≤ 85% |
| ③ | 주기 지터 | GPIO 토글 + 로직 애널라이저 / DWT | 10ms 주기 기준 ±100µs 이내 (±1%) |
| ④ | 긴급정지 End-to-End 응답 | 2채널 로직 애널라이저 | ≤ 10ms (사양에 따라 조정) |
| ⑤ | 워치독 유효성 | 고장 주입 실험 | 태스크 정지 후 IWDG 타임아웃 시간에 리셋 |

---

## 3. 측정 ① — 태스크별 스택 사용량

### 3.1 원리

FreeRTOS는 태스크 생성 시 스택에 알려진 패턴(`0xA5`)을 채워두고, `uxTaskGetStackHighWaterMark()`가 **"아직 지워지지 않은 최소 잔여 워드 수"**를 반환합니다. 이 값이 낮을수록 스택을 많이 쓴 것입니다.

```
스택 상단 ┌──────────────┐
         │  사용된 영역  │  ← 스택 포인터가 여기까지 내려온 적 있음
         │  0xA5 패턴 유지 │  ← High Water Mark = 이 영역의 크기
스택 하단 └──────────────┘
```

### 3.2 필수 설정 (FreeRTOSConfig.h)

```c
#define configCHECK_FOR_STACK_OVERFLOW       2   /* 런타임 스택 오버플로 검사 */
#define configUSE_TRACE_FACILITY             1   /* vTaskList() 사용에 필요 */
#define configUSE_STATS_FORMATTING_FUNCTIONS 1   /* 통계 포맷 함수 사용에 필요 */
```

스택 오버플로 발생 시 호출되는 훅을 구현합니다.

```c
/* 스택 오버플로 발생 시 호출됨 — 여기서 알람 GPIO/LED 표시 후 정지 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    HAL_GPIO_WritePin(LED_ERR_GPIO_Port, LED_ERR_Pin, GPIO_PIN_SET); /* 알람 */
    taskDISABLE_INTERRUPTS();
    for (;;) { /* 원인 분석을 위해 여기서 멈춤 */ }
}
```

### 3.3 측정 코드

**방법 A — 런타임 실측 (주 측정)**

```c
/* 디버그 태스크 등에서 주기적으로 모든 태스크의 스택 잔여량을 출력 */
void vPrintStackUsage(void)
{
    static char pcBuffer[1024];
    vTaskList(pcBuffer);          /* 태스크별 상태+스택 잔여(워드) 테이블 */
    printf("Task            State  Prio  Stack  Num\r\n");
    printf("%s\r\n", pcBuffer);
}
```

개별 태스크를 직접 조회할 때는:

```c
UBaseType_t uxFreeWords = uxTaskGetStackHighWaterMark(xTaskHandle_Sensor);
printf("Sensor stack free: %u words\r\n", (unsigned int)uxFreeWords);
```

**방법 B — 정적 분석 (보조)**

GCC의 `-fstack-usage` 플래그는 함수별 최악 스택 사용량(`.su` 파일)을 생성합니다. CubeIDE의 빌드 플래그에 추가하고, 콜그래프(호출 관계)와 결합하면 각 태스크의 **최악 이론 스택**을 추정할 수 있습니다.

```
# .su 파일 예시 (main.c.su)
main.c:24:6:vTask_Sensor  24  static,dynamic_bounded
```

### 3.4 측정 절차 (중요)

스택 사용량은 **최악 시나리오에서만 의미**가 있습니다. 아래 조건에서 측정하세요.

1. CAN 버스에 최대 부하 트래픽을 주입 (예: 1ms 주기 메시지 연속)
2. `printf` 디버그 출력 ON (포맷팅 함수가 스택을 가장 많이 먹는 경로)
3. 긴급 정지(`prvEmergencyStop`)를 여러 번 트리거 (깊은 호출 스택 경로)
4. 펌웨어 업데이트 시나리오 1회 수행
5. 위 조건으로 최소 30분 운용 후 High Water Mark 기록

### 3.5 결과 표 템플릿

| 태스크 | 할당 스택 | 최대 사용(워드) | 잔여(워드) | 사용률 | 판정(≥30% 여유) |
|---|---|---|---|---|---|
| Watchdog | 128 | `<측정>` | `<측정>` | `<계산>` | ✅/❌ |
| CAN_Rx | 256 | | | | |
| Sensor | 128 | | | | |
| Actuator | 128 | | | | |
| Debug | 256 | | | | |
| Firmware | 512 | | | | |

> 판정 기준: 잔여 워드 ≥ 할당 스택의 30%이면 통과. 미달 시 스택 증량 후 재측정.

---

## 4. 측정 ② — CPU 부하 (Run-Time Stats)

### 4.1 원리

`vTaskGetRunTimeStats()`는 각 태스크가 **스케줄러에 의해 실행된 총 시간**을 자유 실행 카운터 기준으로 집계합니다. 전체 경과 시간 대비 비율이 곧 CPU 점유율입니다.

```
CPU 점유율(%) = 태스크 실행시간 합계 / 벽시계 경과시간 × 100
```

### 4.2 설정 — DWT 사이클 카운터를 시계로 사용 (가장 간단)

DWT(Debug Watchpoint and Trace)의 `CYCCNT`는 별도 타이머 없이 CPU 사이클을 세는 32비트 카운터입니다.

```c
/* FreeRTOSConfig.h */
#define configGENERATE_RUN_TIME_STATS   1
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()  vConfigureTimerForRunTimeStats()
#define portGET_RUN_TIME_COUNTER_VALUE()          DWT->CYCCNT

/* main.c */
static void vConfigureTimerForRunTimeStats(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  /* DWT 활성화 */
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;            /* 카운터 시작 */
}
```

> ⚠️ 주의: 180MHz에서 32비트 CYCCNT는 약 **23.8초**마다 오버플로합니다. 짧은 구간(수 초)만 측정하면 무시 가능하고, 장시간 측정이 필요하면 오버플로를 보정하는 TIM 기반 구현을 사용하세요.

### 4.3 측정 코드

```c
void vPrintRunTimeStats(void)
{
    static char pcBuffer[1024];
    vTaskGetRunTimeStats(pcBuffer);
    printf("Task            AbsTime(us)   Pct\r\n");
    printf("%s\r\n", pcBuffer);
}
```

출력 예시 (DWT 1틱 = 1/180MHz = 5.56ns):

```
Task            AbsTime(us)   Pct
Watchdog              5200     3%
CAN_Rx              160000    89%
Sensor               32000    18%
...
```

### 4.4 SEGGER SystemView (권장)

Run-Time Stats가 "평균 부하"를 준다면, **SystemView는 시간축 위에서 스케줄링을 시각화**합니다. 태스크 전환, ISR, 뮤텍스 획득/대기까지 타임라인으로 보여주므로, "CAN 태스크가 언제 실행을 독점하는지", "긴급 정지가 어느 지점에서 지연되는지"가 직접 보입니다. 결과 캡처 화면은 보고서의 가장 설득력 있는 증거물이 됩니다.

### 4.5 측정 시나리오 매트릭스

| 시나리오 | 조건 | 기록 대상 |
|---|---|---|
| S0 유휴 | CAN 무트래픽, printf OFF | 기준 부하 |
| S1 정상 | CAN 1k msg/s, printf ON | 운용 부하 |
| S2 피크 | CAN 최대 수신 + 긴급정지 연속 트리거 | 최악 부하 |
| S3 업데이트 | 펌웨어 업데이트 수행 | 전이 구간 확인 |

각 시나리오에서 **printf ON/OFF를 각각 측정**하여 디버그 출력의 부하 기여도를 분리하세요. (대부분의 경우 printf가 부하의 상당 부분을 차지합니다 — 이것 자체가 좋은 분석 포인트입니다.)

### 4.6 결과 표 템플릿

| 시나리오 | CAN_Rx | Sensor | Actuator | Debug | Watchdog | Firmware | Idle(여유) | 전체 부하 |
|---|---|---|---|---|---|---|---|---|
| S0 | | | | | | | | |
| S1 | | | | | | | | |
| S2 | | | | | | | | |

> 판정 기준: S2(피크)에서 전체 부하 ≤ 85%, Idle(여유 시간) ≥ 15%면 통과.

---

## 5. 측정 ③ — 주기 지터 (Task Period Jitter)

### 5.1 정의

지터는 **"실제 주기 − 목표 주기"**의 편차입니다. `vTaskDelayUntil()`을 쓰더라도 우선순위 역전·선점·인터럽트 때문에 실제 주기는 흔들립니다.

```
목표: 10.000ms 주기
실측: 9.997 / 10.003 / 10.012 / 9.998 ...
지터: -3µs / +3µs / +12µs / -2µs ...
```

### 5.2 방법 A — GPIO 토글 + 로직 애널라이저 (지상 기준, 권장)

센서 태스크 루프의 시작과 끝에서 GPIO를 토글합니다.

```c
void vTask_Sensor(void *pvParameters)
{
    for (;;)
    {
        HAL_GPIO_WritePin(MEAS_GPIO_Port, MEAS_Pin, GPIO_PIN_SET);  /* 루프 진입 */
        /* ... 센서 읽기 ... */
        HAL_GPIO_WritePin(MEAS_GPIO_Port, MEAS_Pin, GPIO_PIN_RESET); /* 루프 종료 */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}
```

로직 애널라이저에서:
- **상승 에지 간 간격** = 실제 주기 → 지터 산출
- **HIGH 펄스 폭** = 태스크 본문 실행 시간 (CPU 부하와 교차 검증)

샘플은 최소 10,000개(100Hz 기준 약 100초)를 수집합니다.

### 5.3 방법 B — DWT 온보드 측정 (오실로스코프 없이)

펌웨어 안에서 주기를 샘플링해 UART로 덤프합니다.

```c
/* 센서 태스크 루프 */
static uint32_t ulPrevCycle = 0U;
uint32_t ulNow = DWT->CYCCNT;
uint32_t ulDelta = ulNow - ulPrevCycle;   /* 실제 주기(CPU 사이클) */
ulPrevCycle = ulNow;
/* 링버퍼에 저장 후, 디버그 태스크에서 주기적으로 덤프 */
```

덤프된 데이터는 파이썬으로 분석합니다.

```python
# jitter_analysis.py — UART 덤프 데이터(주기, µs 단위) 분석
import statistics

deltas_us = [ ... ]          # 측정값 리스트 (예: 10000개)
nominal = 10000.0            # 목표 주기 10ms = 10000µs

mean   = statistics.mean(deltas_us)
stdev  = statistics.stdev(deltas_us)
j_max  = max(deltas_us) - nominal   # +최대 지터
j_min  = min(deltas_us) - nominal   # -최대 지터 (음수)

print(f"평균 주기   : {mean:8.2f} us")
print(f"표준편차     : {stdev:8.2f} us")
print(f"최대 지터    : +{j_max:.2f} / {j_min:.2f} us")
print(f"P-P 지터     : {max(deltas_us)-min(deltas_us):.2f} us")
```

### 5.4 결과 표 템플릿

| 태스크 | 목표 주기 | 평균 주기 | 표준편차 | 최대 지터(+/-) | P-P | 판정(±1%) |
|---|---|---|---|---|---|---|
| Sensor | 10ms | | | | | ✅/❌ |
| Watchdog | 100ms | | | | | |
| Firmware | 5000ms | | | | | |

---

## 6. 측정 ④ — 긴급정지 End-to-End 응답시간

**이 프로젝트에서 가장 중요한 안전 메트릭입니다.** "큐를 거치지 않고 즉시 적용"했다는 수정이 실제로 몇 ms인지 증명합니다.

### 6.1 측정 경로

```
PC(PCAN-USB) ──CAN──▶ STM32 CAN RX 완료 ──▶ prvEmergencyStop() ──▶ PWM 핀 LOW
                            [지점 A]               [지점 B]
```

### 6.2 방법 — 2채널 로직 애널라이저 (지상 기준)

1. **채널 1**: CAN RX 완료 인터럽트에서 GPIO 토글 (지점 A)
2. **채널 2**: 스로틀 PWM 핀 (지점 B — 듀티 0으로 떨어지는 순간)
3. PCAN-View로 `CAN_ID_EMERGENCY` 전송
4. **A→B 시간 = End-to-End 응답시간**

```c
/* CAN RX 완료 콜백(ISR)에서 지점 A 표시 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    HAL_GPIO_TogglePin(MEAS2_GPIO_Port, MEAS2_Pin);  /* 측정용 토글 */
    /* ... 원래 처리 ... */
}
```

### 6.3 방법 (보조) — DWT 온보드

ISR 진입 시각을 저장해 두고, `prvEmergencyStop()` 종료 시점에서 차이를 계산합니다.

```c
volatile uint32_t ulCanRxCycleStamp = 0;
/* ISR에서 */  ulCanRxCycleStamp = DWT->CYCCNT;
/* prvEmergencyStop() 끝에서 */
uint32_t ulLatency = DWT->CYCCNT - ulCanRxCycleStamp; /* 사이클 */
```

> ⚠️ 온보드 측정은 ISR 진입 전 지연(인터럽트 지연)이 빠지므로, **지상 기준(스코프/LA)이 공식 값**이 되어야 합니다. 온보드 값은 상대 비교용으로만 사용합니다.

### 6.4 결과 표 템플릿

| 측정 회차 | CAN TX→RX 지연(A구간) | 처리 지연(B구간) | End-to-End 합계 | 판정 |
|---|---|---|---|---|
| 1 | | | | ✅/❌ |
| 2 | | | | |
| ... (최소 50회) | | | | |
| **평균/최대** | | | **`<최대값>`** | |

> 판정 기준: 최악 응답시간이 요구사양(예: 10ms) 이내. 안전 관련 시스템이라면 최악값(WCET) 기준으로 판정합니다.

---

## 7. 측정 ⑤ — 워치독 유효성 실험 (고장 주입)

수정 1(전용 워치독 태스크)이 실제로 고장을 잡는지 **의도적으로 태스크를 죽여서** 검증합니다. 문서로 남기면 "단위 테스트를 넘어선 시스템 검증"으로 매우 높은 평가를 받습니다.

| 실험 | 조작 | 기대 결과 | 판정 |
|---|---|---|---|
| E1 정상 운용 | 없음 (30분) | 리셋 없음 | ✅ |
| E2 단일 태스크 정지 | 디버그 태스크 `vTaskSuspend` | IWDG 타임아웃(≈설정값) 후 리셋 발생 | ✅ |
| E3 긴급정지 정상동작 | E2 중 CAN 긴급 메시지 전송 | 정지와 무관하게 즉시 안전 상태 적용 | ✅ |
| E4 업데이트 중 리셋 없음 | 펌웨어 업데이트 수행 | 플래싱 완료까지 리셋 없음 (수정 1 검증) | ✅ |

리셋 원인은 리셋 후 부팅 시 RCC 레지스터로 확인합니다.

```c
/* main() 초반부 — 리셋 원인 기록 */
if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
    printf("Reset cause: IWDG (watchdog)\r\n");
}
__HAL_RCC_CLEAR_RESET_FLAGS();
```

> E2 실험에서 **리셋까지 걸린 시간이 IWDG 설정 타임아웃과 일치하는지**(±오차) 기록하세요. 이 일치가 "워치독이 정확히 동작한다"의 증거입니다. E4는 이번 코드 수정의 핵심(업데이트 중 벽돌 방지)을 직접 증명하는 실험입니다.

---

## 8. 측정 수행 시 유의사항 (독일식 문서 관점)

1. **측정 전 워밍업**: 부팅 직후 측정 금지. PLL 안정화·캐시 워밍 후(예: 30초)부터 기록.
2. **재현 가능성**: 하드웨어 버전, 툴체인 버전, FreeRTOS 버전, 측정 스크립트를 전부 버전 관리(Git)에 포함.
3. **원시 데이터 보존**: 평균값만 남기지 말고 원시 덤프(CSV)를 부록으로 첨부.
4. **최악값 기준 판정**: 평균이 아니라 **최악값(worst-case)**으로 합격/불합격을 판정하는 것이 안전 시스템의 원칙.
5. **단위 명시**: 워드/바이트, µs/ms, % 등 단위를 표마다 명확히.

---

## 9. 대학원 지원에서의 활용 포인트

이 실측 결과는 다음 세 문장으로 요약할 수 있게 만드는 것이 목표입니다:

1. **"스택을 감이 아니라 High Water Mark로 실측해 30% 여유를 확보했다"**
2. **"긴급 정지 End-to-End 응답을 스코프로 측정해 최악 X.X ms임을 증명했다"**
3. **"워치독을 고장 주입 실험으로 검증했고, 업데이트 중 벽돌 위험을 제거했음을 실험 E4로 입증했다"**

이 세 문장은 동기서 한 문단이자, (있다면) 면접에서 5분을 버틸 수 있는 스토리가 됩니다. 측정 보고서는 `measurement_report_template.md` 양식을 사용해 작성하세요.

---

## 부록: 권장 실험 순서 (체크리스트)

- [ ] 하드웨어 조립 및 CAN 루프백 통신 확인
- [ ] FreeRTOSConfig.h에 통계/스택검사 설정 반영
- [ ] DWT 기반 Run-Time Stats 설정
- [ ] 측정용 GPIO 2개 할당 (주기 지터용, E2E용)
- [ ] 스택 측정 (S0~S3 시나리오) → 결과 표 작성
- [ ] CPU 부하 측정 (printf ON/OFF 포함) → SystemView 캡처
- [ ] 주기 지터 측정 (10,000 샘플) → 파이썬 분석
- [ ] 긴급정지 E2E 측정 (50회) → 최악값 기록
- [ ] 워치독 고장 주입 실험 E1~E4
- [ ] 보고서 템플릿 작성 및 원시 데이터 부록 첨부
