# 지원 동기서 (Motivation Letter) 초안 — 독일 자동차 임베디드 석사

> **사용 안내 (한국어)**
> - 독일 대학원 자소서는 미사여구보다 **구조·명확성·프로그램과의 적합성**을 봅니다. 1~2페이지가 표준입니다.
> - 아래 본문은 **영문 제출용 초안**입니다. `[ ]` 표시는 지원자 정보로 채우세요.
> - 하이라이트로 잡은 **안전 설계 프로젝트 서사**가 이 자소서의 핵심 차별점입니다.
> - 독일어 지원 시에는 본문을 독일어로 번역하되, 아래 "작성 전략"의 논리 구조는 그대로 유지하세요.

---

## 제출용 영문 초안

Dear Admissions Committee,

I am writing to apply for the Master's programme in **[Programme name]** at **[University name]** , with a specialisation in embedded systems for automotive applications.

I hold a Bachelor's degree in **[field of study]** from **[university name]** . My coursework in **[control systems / signal processing / embedded systems]** gave me the theoretical foundation, but it was building a real-time engine control prototype that convinced me that embedded software is the field I want to commit to.

The project I am most proud of is a CAN-based engine control application running FreeRTOS on an STM32F4 microcontroller. The system coordinates six tasks — CAN reception, sensor acquisition, actuator control, diagnostics, firmware update, and a supervisory watchdog — through queues, mutexes, and task notifications. What made it more than a programming exercise was a deliberate safety review. I identified two defects in the initial design. First, the independent watchdog was fed only by the CAN task, so a firmware update that suspended that task could trigger a reset mid-flash and permanently damage the firmware. Second, the emergency-stop command travelled through a queue and a mutex with a short timeout, which meant it could be silently dropped under queue overflow or mutex contention. I redesigned both paths: I introduced a dedicated watchdog task that refreshes the IWDG only while every task's heartbeat is confirmed alive, and I moved the emergency stop to an immediate path whose mutex acquisition is guaranteed. I then verified the design mathematically, applying rate-monotonic optimality, the Liu–Layland utilisation bound, and response-time analysis, which showed all tasks schedulable at 24% utilisation — and I prepared a measurement plan to confirm the worst-case execution times on real hardware.

This experience taught me that safety-relevant embedded systems are judged not by whether they work once, but by whether their correctness can be argued and measured. That is precisely the engineering culture I associate with the German automotive industry, and the reason I am applying to **[university name]** . I am particularly interested in **[professor / laboratory / course name]** , and in learning how standards such as ISO 26262 and AUTOSAR turn what I practiced informally into an industrial discipline.

After completing the degree, I aim to work as an embedded software engineer in the automotive sector, contributing to safety-relevant systems. **[Optionally: I am currently learning German at the A1/A2 level and intend to reach B2 during my studies.]**

Thank you for considering my application.

Sincerely,
**[Name]**

---

## 작성 전략 설명 (한국어)

### 1. 이 자소서가 노리는 평가 포인트

| 문단 | 평가자가 읽어내는 것 |
|---|---|
| 1문단 (배경) | 학부 기반 + "왜 임베디드인가"의 최소 서사 |
| 2문단 (프로젝트) | **기술 깊이 + 안전 사고 + 검증 역량** — 이 자소서의 심장 |
| 3문단 (동기 연결) | 프로젝트 → 독일 공학문화 → 이 학교/랩으로의 논리적 연결 |
| 4문단 (목표) | 명확한 진로 + 언어 계획(선택) |

### 2. 2문단이 강력한 이유 — "STAR + Safety" 구조

- **S**(상황): CAN 기반 엔진 제어, FreeRTOS, 6태스크 — 구체적
- **T**(과제): 안전 리뷰에서 결함 2건 발견 (워치독 단일 실패점, 긴급정지 유실)
- **A**(행동): 전용 워치독 태스크 + 즉시 긴급정지 경로로 재설계
- **R**(결과): RMS·LL·RTA로 수학적 검증 + 실측 계획 수립

평가자가 여기서 보는 것은 "코드를 잘 짠다"가 아니라 **"결함을 찾고, 고치고, 증명했다"는 엔지니어 성숙도**입니다. 이는 ISO 26262의 사고방식과 정확히 같은 방향입니다.

### 3. 프로그램별 맞춤 팁

- **TU (연구 중심)**: "수학적 검증(응답시간 분석)" 문장을 더 강조하고, 지도교수·연구실명을 구체적으로 쓰세요.
- **FH/TH (응용 중심)**: "실측 계획 → 하드웨어 검증" 문장을 강조하고, 산학협력·현장실습 의지를 추가하세요.
- 공통: `[Professor/Lab]` 란은 **반드시 실제 교수·연구실을 조사해 채워야** 합니다. 빈칸으로 제출하면 감점.

### 4. 흔한 실수 (독일식 자소서에서 피할 것)

- "어릴 때부터 자동차를 좋아했습니다" 식의 감상적 서사 → 불필요
- 수상 경력·자격증 나열 → CV에 넣을 것
- 모호한 표현("열심히 하겠습니다") → 구체적 행동·근거로 대체
