# STM32F4 · FreeRTOS Embedded Portfolio (Master Package)

> **One-line summary**: A single FreeRTOS embedded project, completed end-to-end as
> **"code → theory proof → static verification → learning tool"**.
> This is the full deliverable set for German automotive embedded master's application
> (portfolio · motivation letter · interview).

---

## 1. Map — what is inside

| Folder | Content | Role in application |
|---|---|---|
| **01_learning_program** | 12-tab learning program + gcc/cppcheck verification server | Practice · verification · retrieval training |
| **02_source_code** | Line-by-line Korean-commented code + theory/standards docs | Core portfolio body |
| **03_static_proof** | Static analysis results performed without hardware | Evidence that turned "plan" into "proof" |

---

## 2. 01_learning_program — how to run

```bash
# Method A: open in browser (all offline features)
open 01_learning_program/learning_program.html

# Method B: full features (gcc · cppcheck compile checks)
cd 01_learning_program
python3 server/build_server.py          # → http://localhost:8080/learning_program.html
# (optional) cppcheck: sudo apt-get install -y cppcheck
```

**12 tabs**: Home·progress / Code browser (29 files) / AI review (17 defect rules) / RTA calculator /
Quiz (18 MCQ + 10 recall) / Build (gcc·cppcheck·MISRA combined) / Learning path (M1→M6 line-jump) /
Coding missions (5 fill-in-blank) / Embedded calculator (6 types) / Simulator (GPIO·CAN) /
Interrupt calculator / Project loader (manifest)

> **Load another project**: `sample_bundle.json` (temperature alarm system) + `MANIFEST_GUIDE.md` (schema)
> let you load other STM32F4·FreeRTOS·C projects into the program.

---

## 3. 02_source_code — project body

```
02_source_code/
├── main_fixed.c            ← FreeRTOS app (v2: watchdog & emergency-stop fixes + 2026-08 safety review, fully commented)
├── bsp/ (22 files)         ← STM32F4 HAL BSP (CAN·ADC·PWM·GPIO·UART·IWDG·Flash·Bootloader·BMP280·Clock)
├── docs/
│   ├── docs_realtime_theory_proof.md         ← real-time theory mathematical proof (RMS/LL/Bini/RTA)
│   ├── docs_measurement_plan.md              ← measurement plan (stack·load·jitter·E2E·watchdog)
│   ├── docs_measurement_report_template.md   ← measurement report template
│   ├── docs_autosar_iso26262.md              ← AUTOSAR·ISO 26262 concepts + project mapping
│   └── docs_motivation_letter.md             ← motivation letter draft (English)
├── rta_analysis.py         ← response-time analysis calculator (re-runnable)
└── rta_results.txt         ← latest RTA output (after v2 fixes, all scenarios schedulable)
```

**Core story** (one paragraph for motivation letter / interview):
> Implemented a CAN-based engine control application on FreeRTOS/STM32F4, and through a safety
> review found and re-designed 2 defects (watchdog single point of failure, emergency-stop loss path).
> Proved the priority assignment mathematically with RMS optimality, Liu–Layland bound and
> response-time analysis (all tasks schedulable, 24.3% utilization).

---

## 4. 03_static_proof — evidence obtained without hardware

```
03_static_proof/
├── static_proof_method.md  ← ★ core document (methodology + actual results + ready-to-use sentences)
├── stack_analyzer.py       ← call-graph based worst-case stack calculator
├── wcet_demo/              ← WCET cycle-counting demo
├── cmsis/                  ← real STM32F446 CMSIS/HAL headers (50+ files)
└── analysis_results/       ← actual .su/.ci static analysis artifacts
```

**Actually performed & obtained**:

| Item | Result |
|---|---|
| Full BSP cross-compile (real Cortex-M4) | 10 files OK / 0 FAIL |
| Code size | text 3.8 KB + bss 0.7 KB |
| Max task stack (static) | 136 B vs 512 B allocated (3.8× over-allocation proven) |
| WCET static estimate | bmp280_compensate ≈ 83 ns @180 MHz |
| Defects found | 3 (missing NVIC decl, SPL macro, float→libgcc) |

> **Honest framing**: static analysis proves **upper/lower bounds**, not a "replacement" for measurement.
> Items that require real hardware (jitter·E2E·IWDG) are listed in §10 of the document; once a Nucleo
> board (~30,000 KRW) is available, the same methodology continues directly into measurement.

---

## 5. Recommended usage order (application prep)

```
1️⃣ 03_static_proof/static_proof_method.md §9 ready-made paragraph → insert into motivation letter
2️⃣ 02_source_code/docs/docs_motivation_letter.md → expand letter body
3️⃣ 01_learning_program → interview prep (find-mode + recall quiz + coding missions)
4️⃣ (when hardware arrives) 02/docs_measurement_plan.md → 5 measurements → fill report template
5️⃣ (for diversification) after STM32F4, choose Infineon AURIX, not ESP32 (automotive standard MCU)
```

---

## 6. Remaining tasks (honest list)

| Task | Priority | Note |
|---|---|---|
| Hardware measurement (Nucleo) | 🥇 highest | jitter·E2E·IWDG are measurement-only |
| Measured WCET → RTA recompute | 🥈 high | replace static lower bound with measurement |
| Self-flashing → staging-area pattern | 🥈 high | current skeleton erases app sectors while running from them (brick risk); flash to staging region + bootloader copy after reset is the standard pattern |
| Integration layer (HAL_Init, stm32f4xx_it.c, startup, linker script, FreeRTOSConfig.h) | 🥈 high | `HAL_Delay()` in `BMP280_Init` infinite-loops if the HAL tick is not wired; needed before first hardware run |
| AURIX introduction | 🥉 differentiator | automotive safety MCU standard |
| AUTOSAR·ISO 26262 deepening | parallel | concept → practice |
| Re-sync `learning_program.html` code snapshot | 🥉 low | code-browser tab still embeds the pre-v2 `main_fixed.c` |

---

## 7. Version · reproducibility

- Target: NUCLEO-F446RE (STM32F446RE, 180 MHz) / FreeRTOS / STM32CubeF4 HAL
- Toolchain: arm-none-eabi-gcc 14.2.1, gcc 14.2, cppcheck 2.17.1
- Verified: JS `node --check` pass, project cross-compile OK, ZIP integrity OK
- Written: 2026-08

---

## 8. v2 safety review fixes (2026-08-18) — found by self-review, before anyone else did

A second-pass review of the headline safety story ("fixed watchdog & emergency stop") found that
the two flagship fixes still had gaps. All five are fixed in this revision and the analysis was re-run:

| # | Defect found | Severity | Fix |
|---|---|---|---|
| 1 | **IWDG was never initialized** — `IWDG_Init()` was called nowhere, so every feed was a no-op and the watchdog story (fix 1 of v1) ran on dead hardware | 🔴 critical | `main()` now calls `IWDG_Init(IWDG_TIMEOUT_MS=1000)` before starting the scheduler |
| 2 | **CAN lost-wakeup** — binary semaphore (max count 1) + 16-slot ring: after a 2-frame burst the task could block with messages still pending; worst case an emergency-stop frame waited *indefimately* for the next bus traffic | 🔴 critical | `CAN_Receive()` checks ring occupancy *before* blocking on the semaphore |
| 3 | **Emergency stop not latched** — stale `CMD_THROTTLE` commands already in the queue re-energized the actuator after `prvEmergencyStop()` | 🟠 medium | `xEmergencyStopLatched` set on e-stop; non-e-stop commands dropped (counted) until system reset |
| 4 | **Sensor queue structurally overflowing** — production 100 Hz vs debug consumption ~18 Hz ⇒ queue(16) permanently full, drop counter ran away at ~82/s | 🟠 medium | keep-latest: queue length 1 + `xQueueOverwrite()` — always freshest sample, no overflow |
| 5 | **RTA blind spot: tied priorities** — Sensor and Actuator both p=3; FreeRTOS time-slices equal priorities, which the RTA (`hp = strictly higher`) does not model — and "RMS-optimality" claim didn't match the code | 🟠 medium | Sensor=4 > Actuator=3; `rta_analysis.py` now warns if any priorities are tied; scenario B is a full strict-RMS re-assignment; CAN_Rx `B_i=0.05ms` models the mutex critical section |

**Re-run result** (`02_source_code/rta_results.txt`): U = 24.30% unchanged (priorities don't
affect utilization); all tasks schedulable in all three scenarios. Worst R: CAN_Rx 0.14 ms
(was 0.09 — now includes blocking term), Actuator 0.95 ms (was 0.15 — now includes Sensor
interference that the tie previously hid). Watchdog at RMS position (scenario B) still passes
(R = 7.22 ms ≪ 1000 ms), so the "safety-first" top placement remains a justified design choice.

**Framing for interviews**: this table is deliberately honest — the project claims "found and
fixed 2 defects" as its story, so the review that found 5 more in the fix itself is exactly the
ISO 26262 mindset the project is selling. Known remaining gaps are in §6.
