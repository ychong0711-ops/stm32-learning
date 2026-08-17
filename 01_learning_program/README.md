# Learning Program — Run Guide

## Quick start
- **Method A (offline)**: open `learning_program.html` in a browser → code browser, review, RTA, MISRA, quiz, path, missions, calculators, simulator, interrupt, project loader all work.
- **Method B (full)**: `python3 server/build_server.py` → http://localhost:8080/learning_program.html
  - adds gcc `-fsyntax-only` + cppcheck static analysis
  - if cppcheck missing: `sudo apt-get install -y cppcheck`

## Server folder
- `build_server.py` — static serving + /api/build + /api/cppcheck + /api/build-project
- `make_stubs.py` — STM32 HAL/FreeRTOS stub header generator
- `make_build_main.py` — build main generator (removes duplicate typedefs)
- `build_learning.py` ~ `build_learning7.py` — idempotent HTML builders (course embedding → build → learning-effect upgrades → embedded → simulator → bundle loader → training modes)
- `stubs/` — 7 stub headers
- `commented_build/main_build.c` — build main

## Load another project
- `sample_bundle.json` — temperature alarm example bundle
- `MANIFEST_GUIDE.md` — full manifest.json schema
