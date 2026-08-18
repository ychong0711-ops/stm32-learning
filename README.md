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

# Method B: full features (gcc compile checks)
cd 01_learning_program/server
python3 build_server.py                 # → http://localhost:8080/learning_program.html
# (optional) cppcheck: sudo apt-get install -y cppcheck
```

**12 tabs**: Home·progress / Code browser (46 files) / AI review (17 defect rules) / RTA calculator /
Quiz (18 MCQ + 10 recall) / Build (gcc·cppcheck·MISRA combined) / Learning path (M1→M6 line-jump) /
Coding missions (5 fill-in-blank) / Embedded calculator (6 types) / Simulator (GPIO·CAN) /
Interrupt calculator / Project loader (manifest)

> **Load another project**: `sample_bundle.json` (temperature alarm system) + `MANIFEST_GUIDE.md` (schema)
> let you load other STM32F4·FreeRTOS·C projects into the program.

> **Note**: the HTML embeds a snapshot of the sources it shows. After editing anything under
> `02_source_code/`, re-run `python3 server/refresh_html_sources.py` to refresh it
> (`--check` reports staleness without writing, for CI).
> Tooling details — what the build API can and cannot catch, which scripts are historical:
> `01_learning_program/server/README.md`.

---

## 3. 02_source_code — project body

```
02_source_code/
├── Makefile                ← two-image build (bootloader + app), `make deps && make`
├── BUILD.md                ← build instructions, flags, troubleshooting
├── main_fixed.c            ← FreeRTOS app (watchdog & emergency-stop safety fixes + v2 safety review, fully commented)
├── bsp/ (26 files)         ← STM32F4 HAL BSP (CAN·ADC·PWM·GPIO·UART·IWDG·Flash·Bootloader·BMP280·Clock)
├── bootloader/boot_main.c  ← sector-0 bootloader (verify · install · jump)
├── app/                    ← integration layer: IRQ handlers, FreeRTOSConfig.h, hooks, tiny libc/printf
├── startup/ · system/      ← CMSIS startup assembly + SystemInit
├── linker/                 ← two linker scripts (0x08000000 boot / 0x08004000 app)
├── tools/                  ← make_image.py (firmware header + CRC-32), make_combined.py,
│                             make_stage_blob.py (staging-area blob, incl. fault injection)
├── docs/
│   ├── docs_realtime_theory_proof.md         ← real-time theory mathematical proof (RMS/LL/Bini/RTA)
│   ├── docs_measurement_plan.md              ← measurement plan (stack·load·jitter·E2E·watchdog)
│   ├── docs_measurement_report_template.md   ← measurement report template
│   ├── docs_autosar_iso26262.md              ← AUTOSAR·ISO 26262 concepts + project mapping
│   ├── docs_bootloader_design.md             ← staging-based firmware update + sector-0 bootloader
│   ├── hardware_validation_guide.html        ← on-target validation guide (NUCLEO-F446RE, open in a browser)
│   └── docs_motivation_letter.md             ← motivation letter draft (English)
├── rta_analysis.py         ← response-time analysis calculator (re-runnable)
└── rta_results.txt         ← latest RTA output (after v2 fixes, all scenarios schedulable)
```

### Building it

```bash
cd 02_source_code
make deps     # fetch ST HAL + FreeRTOS kernel (once; not vendored here)
make          # bootloader + app + update image
make size     # per-region usage
```

Full instructions, flag rationale and troubleshooting: **`02_source_code/BUILD.md`**.

**Core story** (one paragraph for motivation letter / interview):
> Implemented a CAN-based engine control application on FreeRTOS/STM32F4, and through a safety
> review found and re-designed 3 defects (watchdog single point of failure, emergency-stop loss
> path, and a firmware update that overwrote the running application with no recovery path).
> Proved the priority assignment mathematically with RMS optimality, Liu–Layland bound and
> response-time analysis (all tasks schedulable, 24.3% utilization), then built out the missing
> integration layer — startup, linker scripts, vector relocation, interrupt entry points — until
> the whole thing links clean as a two-image bootloader + application build.

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
| Defects found | 3 (missing NVIC decl, SPL macro, float→libgcc) — all now fixed |
| **Full firmware link (2026-08)** | **bootloader 4,600 B / 16 KB · app 28,240 B / 112 KB · 0 warnings, 0 undefined symbols** |

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
| Firmware image transport (CAN) | 🥈 high | staging API is ready; the chunking/retry protocol is not written |
| Image signing (ECDSA) | medium | CRC-32 catches accidental corruption only, not tampering |
| A/B bank rollback | medium | sectors 6–7 (256 KB) are free; today it is "keep old app on failure" |
| AURIX introduction | 🥉 differentiator | automotive safety MCU standard |
| AUTOSAR·ISO 26262 deepening | parallel | concept → practice |
| Re-sync `learning_program.html` code snapshot | 🥉 low | code-browser tab still embeds the pre-v2 `main_fixed.c` |

> **Resolved in 2026-08**: the project previously had no startup file, linker script or Makefile,
> never called `HAL_Init()`, and had no SysTick/IRQ entry points — it could not actually be built
> or booted. It also flashed *itself* while running, which had an unrecoverable brick window.
> Both are fixed: see `02_source_code/BUILD.md` and `02_source_code/docs/docs_bootloader_design.md`.

---

## 7. Version · reproducibility

- Target: NUCLEO-F446RE (STM32F446RE, 180 MHz) / FreeRTOS / STM32CubeF4 HAL
- Toolchain: arm-none-eabi-gcc 13.2.1 (full link verified) / 14.2.1 (earlier syntax pass), gcc 14.2, cppcheck 2.17.1
- Vendor code (ST HAL, FreeRTOS kernel) is **not vendored**; `make deps` fetches it
- Verified: JS `node --check` pass, full two-image cross-link OK, CRC-32 host/device/zlib agree, ZIP integrity OK
- Written: 2026-08

---

## 8. v2 safety review fixes (2026-08-18) — found by self-review, before anyone else did

A second-pass review of the headline safety story ("fixed watchdog & emergency stop") found that
the two flagship fixes still had gaps. All five are fixed in this revision and the analysis was re-run:

| # | Defect found | Severity | Fix |
|---|---|---|---|
| 1 | **IWDG was never initialized** — `IWDG_Init()` was called nowhere, so every feed was a no-op and the watchdog story (fix 1 of v1) ran on dead hardware | 🔴 critical | ✅ already fixed by the integration layer (PR #1): `main()` calls `IWDG_Init(4000ms)` (staging-sector-erase 2s margin + reset-cause capture). No additional call added here |
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
