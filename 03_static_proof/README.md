# STM32F4 Static Proof Package

> Quantitative proof of project depth obtained **without hardware**.
> Core document: `static_proof_method.md` (methodology + actual results).

## Contents
- `static_proof_method.md` — ★ core document (method + results + ready-to-use sentences)
- `stack_analyzer.py` — call-graph based worst-case stack calculator (re-runnable)
- `wcet_demo/` — WCET cycle-counting demo
- `cmsis/` — real STM32F446 CMSIS/HAL headers (50+ files)
- `analysis_results/` — actual .su/.ci static analysis artifacts

## Key results
| Item | Result |
|---|---|
| Full BSP cross-compile | 10 files OK / 0 FAIL |
| Max task stack | 136 B vs 512 B allocated |
| WCET estimate | ≈ 83 ns @180 MHz |
| Defects found | 3 |

## Reproduce
```bash
sudo apt-get install -y gcc-arm-none-eabi
arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -O2 -DSTM32F446xx \
  -I cmsis -I FreeRTOS -I bsp -c bsp/bsp_can.c -fstack-usage
python3 stack_analyzer.py vTask_Watchdog vTask_CAN_Rx vTask_Sensor vTask_Actuator vTask_Debug vTask_Firmware main
```

## Honest framing (important)
- Static analysis gives **upper/lower bounds**, NOT a replacement for measurement.
- Measurement-only items (jitter·E2E·IWDG) are listed in `static_proof_method.md` §10.
- This is the **pre-stage** of measurement; it continues directly once a Nucleo board is available.
