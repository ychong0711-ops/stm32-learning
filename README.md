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
├── main_fixed.c            ← FreeRTOS app (watchdog & emergency-stop safety fixes, fully commented)
├── bsp/ (22 files)         ← STM32F4 HAL BSP (CAN·ADC·PWM·GPIO·UART·IWDG·Flash·Bootloader·BMP280·Clock)
├── docs/
│   ├── docs_realtime_theory_proof.md         ← real-time theory mathematical proof (RMS/LL/Bini/RTA)
│   ├── docs_measurement_plan.md              ← measurement plan (stack·load·jitter·E2E·watchdog)
│   ├── docs_measurement_report_template.md   ← measurement report template
│   ├── docs_autosar_iso26262.md              ← AUTOSAR·ISO 26262 concepts + project mapping
│   └── docs_motivation_letter.md             ← motivation letter draft (English)
└── rta_analysis.py         ← response-time analysis calculator (re-runnable)
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
| AURIX introduction | 🥉 differentiator | automotive safety MCU standard |
| AUTOSAR·ISO 26262 deepening | parallel | concept → practice |

---

## 7. Version · reproducibility

- Target: NUCLEO-F446RE (STM32F446RE, 180 MHz) / FreeRTOS / STM32CubeF4 HAL
- Toolchain: arm-none-eabi-gcc 14.2.1, gcc 14.2, cppcheck 2.17.1
- Verified: JS `node --check` pass, project cross-compile OK, ZIP integrity OK
- Written: 2026-08
