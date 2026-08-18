# 빌드 방법

NUCLEO-F446RE 용 **부트로더 + 애플리케이션** 2단 이미지를 만든다.

---

## 1. 준비물

| 항목 | 비고 |
|---|---|
| `arm-none-eabi-gcc` | 하드 float(`thumb/v7e-m*/hard`) 멀티리브가 있어야 한다. 13.2.1 로 검증했다. |
| `python3` | 이미지 헤더 생성 도구용. 표준 라이브러리만 쓴다. |
| `make` | |
| `git` | `make deps` 가 벤더 소스를 받을 때만 필요하다. |

이 저장소는 **ST HAL 드라이버와 FreeRTOS 커널을 포함하지 않는다.** 우리 코드가
아니고 용량이 크기 때문이다. 아래 `make deps` 로 받거나, 이미 갖고 있으면
경로를 넘기면 된다.

---

## 2. 세 줄 요약

```bash
cd 02_source_code
make deps     # ST HAL + FreeRTOS 커널 내려받기 (최초 1회)
make          # 부트로더 + 앱 + 갱신 이미지 전부 빌드
```

이미 벤더 소스를 갖고 있다면:

```bash
make HAL_DIR=~/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver \
     FREERTOS_DIR=~/FreeRTOS-Kernel
```

---

## 3. 타깃

| 명령 | 하는 일 |
|---|---|
| `make` | `boot` + `app` + `image` 전부 |
| `make boot` | 부트로더만 → `build/boot/boot.{elf,bin,hex}` |
| `make app` | 애플리케이션만 → `build/app/app.{elf,bin,hex}` |
| `make image` | 앱 바이너리에 갱신용 32B 헤더 부착 → `build/app/app_image.bin` |
| `make combined` | 부트로더+앱 합본 (공장 출하용) → `build/combined.bin` |
| `make size` | 영역별 사용량과 여유 출력 |
| `make clean` | 산출물 삭제 |
| `make distclean` | 벤더 소스까지 삭제 |
| `make flash-boot` / `flash-app` / `flash-combined` | `st-flash` 로 기록 |

펌웨어 버전은 이미지 생성 시 지정할 수 있다:

```bash
make image FW_VERSION=0x00010203   # v1.2.3
```

---

## 4. 지금 나오는 결과

```
=== 플래시 영역 사용량 ===============================================
부트로더 (섹터 0,   16384 B):    4600 B  (11784 B 여유)
앱       (섹터 1~4, 114688 B):   28240 B  (86448 B 여유)
스테이징 (섹터 5,  131072 B): 앱 이미지 + 512 B 메타 영역
======================================================================

   text    data     bss     dec     hex  filename
   4540      44    1044    5628    15fc  build/boot/boot.elf
  28344      48   32904   61296    ef70  build/app/app.elf
```

경고 0개, 미정의 심볼 0개로 링크된다.

앱의 bss 가 32KB 로 큰 것은 FreeRTOS 힙(`heap_4`, 30KB)이 여기 잡히기 때문이다.
태스크 스택과 큐가 전부 이 안에서 나온다.

---

## 5. 무엇이 어떻게 빌드되는가

### 두 개의 독립된 이미지

| | 부트로더 | 애플리케이션 |
|---|---|---|
| 위치 | `0x08000000` (섹터 0, 16KB) | `0x08004000` (섹터 1~4, 112KB) |
| 링커 스크립트 | `linker/STM32F446RE_BOOT.ld` | `linker/STM32F446RE_APP.ld` |
| 최적화 | `-Os` (16KB 안에 들어가야 함) | `-Og` (디버깅 우선) |
| RTOS | 없음 (베어메탈, `-DBSP_BAREMETAL`) | FreeRTOS |
| 클록 | HSI 16MHz 그대로 | HSE→PLL 180MHz |
| HAL 모듈 | 13개 (flash/rcc/gpio/pwr/iwdg/dma 등) | 21개 (+can/adc/i2c/tim/uart) |
| VTOR | 리셋 기본값 | `-DVECT_TAB_OFFSET=0x4000` |

두 이미지는 `startup_stm32f446xx.s` 와 `system_stm32f4xx.c`, 그리고 플래시
관련 BSP(`bsp_flash.c`, `fw_image.c`)를 공유하되 **서로 다른 플래그로 각각
컴파일된다.** 오브젝트는 `build/boot/` 와 `build/app/` 로 분리되어 섞이지 않는다.

### 중요한 컴파일 플래그

| 플래그 | 왜 필요한가 |
|---|---|
| `-DHSE_VALUE=8000000` | HAL 기본값은 25MHz 인데 NUCLEO-F446RE 는 ST-LINK 에서 8MHz 를 받는다. 틀리면 PLL 계산이 어긋나 180MHz 가 안 나온다. |
| `-DUSER_VECT_TAB_ADDRESS -DVECT_TAB_OFFSET=0x4000` | (앱 전용) `SystemInit()` 이 `SCB->VTOR` 를 `0x08004000` 으로 옮기게 한다. 없으면 첫 인터럽트에서 부트로더 핸들러로 뛴다. |
| `-DBSP_BAREMETAL` | (부트로더 전용) FreeRTOS 헤더 대신 `bsp/bsp_baremetal.h` 의 최소 타입 정의를 쓴다. |
| `-fno-tree-loop-distribute-patterns` | 복사 루프가 `memcpy` 호출로 치환되는 것을 막는다. `tiny_libc.c` 의 `memcpy` 가 자기 자신을 부르는 무한 재귀를 방지한다. |
| `-fstack-usage` | 함수별 `.su` 파일을 만든다. `03_static_proof/` 의 스택 분석 입력이 된다. |
| `-mfpu=fpv4-sp-d16 -mfloat-abi=hard` | Cortex-M4F 하드웨어 FPU. FreeRTOS `ARM_CM4F` 포트와 반드시 일치해야 한다. |

### `-nostdlib` 로 링크하는 이유

표준 C 라이브러리를 링크하지 않는다. 이유가 두 가지다.

1. 검증에 쓴 툴체인에 newlib(`libc.a`)이 없었다.
2. 더 중요한 이유 — **링크되는 코드가 전부 이 저장소 안에 있어야** 정적 스택
   분석과 WCET 근거가 성립한다. newlib 의 `printf` 는 스택을 수백 바이트 쓰고
   힙과 재진입 구조체에 의존해서, 최악 스택 사용량을 계산할 수 없다.

대신 필요한 것만 직접 제공한다.

| 파일 | 내용 |
|---|---|
| `app/tiny_libc.c` | `memcpy`/`memset`/`strlen`/`strcpy` 등 8종 + `__libc_init_array` |
| `app/tiny_printf.c` | 정수 전용 `printf`/`puts`/`putchar` (JSON 로그용, 부동소수점 미지원) |

`libgcc.a` 는 여전히 링크한다. `__aeabi_uidiv` 같은 정수 나눗셈 루틴이 여기
들어 있고, 이건 컴파일러가 알아서 부르는 것이라 대체할 수 없다.
(`03_static_proof` 에서 찾은 결함 #3 이 바로 이 누락이었다)

`printf` 가 부동소수점을 지원하지 않는 것은 의도한 것이다. `main_fixed.c` 의
로그는 전부 정수 포맷만 쓴다. 임베디드에서 `%f` 는 코드 크기와 스택을 크게
늘리고 실행 시간도 예측하기 어렵다.

---

## 6. 굽는 법

### 처음 (부트로더가 아직 없을 때)
```bash
make combined
st-flash --reset write build/combined.bin 0x08000000
```

### 앱만 다시 (부트로더는 그대로)
```bash
make app && make flash-app
```

### 무선/CAN 갱신용 이미지
```bash
make image
# → build/app/app_image.bin 을 장치로 전송
#    앱이 스테이징 영역에 기록 → 부트로더가 검증·설치
```

동작 원리는 `docs/docs_bootloader_design.md` 참조.

---

## 7. 안 될 때

**`libgcc.a 를 찾지 못했습니다`**
툴체인에 하드 float 멀티리브가 없다. `arm-none-eabi-gcc -print-multi-lib` 로
`thumb/v7e-m+fp/hard` 또는 `thumb/v7e-m+dp/hard` 가 있는지 확인한다.
ARM 공식 GNU Toolchain 배포판을 쓰면 확실하다.

**`HAL 소스가 없습니다` / `FreeRTOS 커널이 없습니다`**
`make deps` 를 먼저 돌리거나 `HAL_DIR=` / `FREERTOS_DIR=` 로 경로를 지정한다.

**부트로더가 16KB 를 넘칠 때**
`HAL_SRCS_BOOT` 에서 안 쓰는 모듈을 빼거나, `BOOT_CFLAGS` 의 `-g3` 를 지운다.
(디버그 정보는 `.bin` 크기에 안 들어가지만 `.elf` 는 커진다)

**앱이 `main()` 까지는 가는데 인터럽트에서 죽을 때**
`-DUSER_VECT_TAB_ADDRESS -DVECT_TAB_OFFSET=0x4000` 이 빠졌는지 본다.
`SCB->VTOR` 값을 디버거로 확인하면 바로 보인다 — `0x08004000` 이어야 한다.
