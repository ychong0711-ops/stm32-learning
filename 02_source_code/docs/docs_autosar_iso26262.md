# AUTOSAR · ISO 26262 입학 전 최소 개념 요약

> **용도**: 독일 자동차 임베디드 대학원 지원자가 입학 전에 반드시 알아야 할
> 두 표준의 핵심 개념을, **우리 프로젝트(main_fixed.c + bsp/)와 연결**해서 정리.
> 면접과 동기서에서 "이 용어를 내 프로젝트에 이렇게 대응시켜 봤다"고 말할 수 있게 하는 것이 목표.

---

## Part 1 — AUTOSAR (AUTomotive Open System ARchitecture)

### 1.1 왜 생겼나

- 자동차 한 대에 ECU가 수십~수백 개, 부품사(Tier-1)도 다수.
- 부품사마다 SW 구조가 제각각이면 ECU를 바꿀 때마다 SW를 다시 짜야 함.
- 목표: **HW 독립성 + 부품사 간 재사용** → "SW를 표준 아키텍처 위에 조립"하자는 것.

### 1.2 Classic vs Adaptive

| 구분 | Classic Platform (CP) | Adaptive Platform (AP) |
|---|---|---|
| 대상 | 실시간·안전 중시 ECU | 고성능 SoC |
| 언어 | C | C++14/17 |
| OS | OSEK/AUTOSAR OS (정적) | POSIX (동적) |
| 통신 | CAN/CAN-FD/LIN | 이더넷, SOME/IP |
| 예 | 브레이크·엔진·조향 | ADAS·게이트웨이·인포테인먼트 |

> 💡 **우리 프로젝트는 CP 스타일** (C + RTOS + CAN). 면접에서 "제가 한 건 Classic Platform의 원형"이라고 연결할 수 있습니다.

### 1.3 Classic Platform 계층 구조 (아래 → 위)

```
┌─────────────────────────────┐
│  Application Layer (SWC들)   │  ← 기능 단위 컴포넌트
├─────────────────────────────┤
│  RTE (Runtime Environment)   │  ← SWC 간 통신 연결 (핵심)
├─────────────────────────────┤
│  Services Layer              │  ← OS, 통신(CAN), 메모리(NVM), 진단(DEM/DCM)
├─────────────────────────────┤
│  ECU Abstraction Layer       │  ← ECU HW를 감싼 추상화
├─────────────────────────────┤
│  MCAL (Microcontroller Abstraction) │ ← 레지스터 직접 접근
└─────────────────────────────┘
```

### 1.4 핵심 개념 5가지

1. **SWC (Software Component)**: 기능 단위. 다른 SWC를 **직접 호출하지 못하고** RTE를 통해서만 통신.
2. **VFB (Virtual Functional Bus)**: SWC를 "어느 ECU에 배치되든" 동일하게 통신시키는 가상 버스. 배치(매핑) 전 SWC 설계를 가능하게 함.
3. **Port/Interface**: Sender-Receiver(데이터 전달), Client-Server(서비스 호출).
4. **Runnable**: 스케줄러(OS)가 주기/이벤트로 호출하는 SWC의 실행 단위.
5. **BSW (Basic Software)**: Services + ECU Abstraction + MCAL을 통칭.

### 1.5 우리 프로젝트와의 대응표 (면접용)

| AUTOSAR 개념 | 우리 프로젝트의 대응 |
|---|---|
| MCAL | `bsp_can.c`, `bsp_adc.c`, `bsp_pwm.c` 등 BSP — 레지스터/페리퍼럴 추상화 |
| Services (통신) | 큐·세마포어 기반 태스크 간 통신 |
| Runnable | `vTask_*` 함수들의 루프 본문 |
| Sender-Receiver | `xQueueSend`/`xQueueReceive` |
| RTE의 역할 | "태스크 간 직접 호출 금지, 큐로만 통신"이라는 우리 설계 원칙 |

> **면접 한 문장**: "AUTOSAR를 실무로 쓰진 않았지만, 그 존재 이유인 '하드웨어 추상화와 컴포넌트 간 결합 제거'를 이 프로젝트에서 체감했고, 그래서 BSP를 논리 계층으로 분리했습니다."

---

## Part 2 — ISO 26262 (Road vehicles — Functional Safety)

### 2.1 왜 필요한가

- 전자 시스템 오류가 인명 사고로 이어진다 → **"안전하다"는 주장을 표준화된 증거로 입증**해야 한다.
- V-모델 기반: 요구사항 → 설계 → 구현 → 검증을 내려갔다 올라오며, 각 단계마다 산출물을 남긴다.

### 2.2 ASIL (Automotive Safety Integrity Level)

- 등급: **QM(비안전) < ASIL A < B < C < D(가장 엄격)**
- 등급이 높을수록 요구되는 개발/검증 활동이 가중됨.
- 등급은 아래 HARA 결과로 결정.

### 2.3 HARA (Hazard Analysis and Risk Assessment)

각 위험 사건(hazard)에 대해 3요소를 평가:

| 요소 | 의미 | 등급 예 |
|---|---|---|
| Severity (S) | 사고의 심각도 | S0(무해) ~ S3(생명 위협) |
| Exposure (E) | 상황 발생 빈도 | E0 ~ E4 |
| Controllability (C) | 운전자가 회피 가능성 | C0(항상 회피) ~ C3(불가) |

S+E+C 조합 → ASIL 결정. 예: "주행 중 스로틀이 갑자기 최대 개방" = S3 + E4 + C3 → **ASIL D**.

### 2.4 핵심 개념

- **Safety Goal**: 최상위 안전 요구사항. 예: "긴급정지 명령 수신 시 10ms 이내 스로틀 출력 차단"
- **Safety Mechanism**: 오류를 감지·완화하는 장치 — 워치독, ECC, 메모리 보호, 런타임 검사 등
- **Safe State**: 위험을 막기 위해 진입하는 안전한 상태 (예: 스로틀 0%)
- **Fail-safe vs Fail-operational**: 안전 상태로 정지 vs 결함에도 계속 동작
- **Freedom from Interference**: 서로 다른 ASIL 등급의 SW가 서로 간섭하지 않아야 함 → MPU/메모리 파티셔닝
- **Decomposition**: 하나의 안전 요구를 여러 독립 메커니즘으로 나눠 등급 부담을 낮추는 기법

### 2.5 우리 프로젝트와의 대응표 (이 표가 핵심 자산)

| ISO 26262 개념 | 우리 프로젝트의 구현 |
|---|---|
| Safety Goal | "긴급정지 CAN 수신 → 즉시 스로틀 0" (E2E 응답시간 요구로 정량화) |
| Safety Mechanism — Program Flow Monitoring | **전용 워치독 태스크** (각 태스크 하트비트 확인 후에만 IWDG 피드) |
| Safe State 진입 | `prvEmergencyStop()` — PWM 0 + 팬 OFF 즉시 적용 |
| 우선순위 역전 방지 | 뮤텍스(우선순위 상속) 사용 |
| 검증 — 최악값(WCET) 기준 | 실측 계획서의 긴급정지 E2E **최악값** 판정 |
| Fault Injection Test | 워치독 고장 주입 실험 E1~E4 |
| 안전 요구의 정량화 | 응답시간 분석(RTA)으로 데드라인 준수 수학적 증명 |

> **면접 한 문장**: "ISO 26262를 정식으로 적용해 본 적은 없지만, 제가 한 '워치독 하트비트 감시'는 Program Flow Monitoring, '긴급정지 즉시 경로'는 Safe State 진입이라는 안전 메커니즘의 축소판입니다."

---

## Part 3 — 한 장 요약표 (암기용)

### AUTOSAR
- 목적: **HW 독립성 + SW 재사용**
- CP vs AP: **C/정적/실시간 vs C++/동적/고성능**
- 계층(위→아래): SWC → RTE → Services → ECU Abstraction → MCAL
- SWC는 **RTE로만** 통신, VFB가 ECU 배치 무관 통신 보장

### ISO 26262
- 목적: **기능 안전의 표준화된 증명**
- ASIL: **QM < A < B < C < D**
- HARA 3요소: **S(심각도) × E(노출) × C(제어가능성)**
- 안전 3종 세트: **Safety Goal → Safety Mechanism → Safe State**
- 우리 코드 3대응: **워치독=프로그램 흐름 감시, 긴급정지=안전 상태, E2E 최악값=정량 검증**

---

## Part 4 — 다음 학습 단계 (우선순위)

1. **MISRA-C 2012** — 우리 코드에 정적 검사기(cppcheck)로 적용해 보고, 위반 항목 정리 → "산업 표준을 안다"는 직접 증거
2. **HIL 개념** — 실제 하드웨어에 CAN 트래픽을 주입해 검증하는 개념 (우리 실측 계획서의 확장)
3. **AUTOSAR OS와 FreeRTOS 비교** — Task/ISR 우선순위 모델, 스케줄링 테이블 vs 우선순위 선점
4. **FMEDA 정량 분석 입문** — ASIL 등급이 실제 고장률 수치로 어떻게 이어지는지 개괄만
