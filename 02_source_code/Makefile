# =============================================================================
# Makefile — STM32F446RE 2단 이미지 빌드 (부트로더 + 애플리케이션)
#
#   make            부트로더와 앱을 모두 빌드합니다.
#   make boot       부트로더만 빌드합니다.       → build/boot/boot.elf .bin .hex
#   make app        애플리케이션만 빌드합니다.   → build/app/app.elf .bin .hex
#   make image      앱 .bin 에 32바이트 갱신 헤더를 붙입니다. → build/app/app_image.bin
#   make combined   부트로더+앱을 합친 공장 출하용 이미지를 만듭니다.
#   make size       각 이미지의 섹션 크기와 영역 여유를 출력합니다.
#   make deps       HAL / CMSIS / FreeRTOS 커널을 third_party/ 로 내려받습니다.
#   make flash-boot / flash-app / flash-combined    st-flash 로 기록합니다.
#   make clean      빌드 산출물을 지웁니다.
#
# [메모리 배치] 자세한 정의는 bsp/flash_map.h 에 있으며 링커 스크립트와 일치합니다.
#   0x08000000  16KB   섹터 0     부트로더        (STM32F446RE_BOOT.ld)
#   0x08004000  112KB  섹터 1~4   애플리케이션    (STM32F446RE_APP.ld)
#   0x08020000  128KB  섹터 5     스테이징 영역   (수신 이미지 임시 보관)
#   0x40024000  4KB    백업 SRAM  부트 제어 블록  (설치 요청/시도 횟수/결과)
#
# [외부 의존성] 이 저장소는 벤더 소스를 포함하지 않습니다. `make deps` 로 받거나
#   이미 가지고 있다면 경로를 넘기십시오:
#     make HAL_DIR=~/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver \
#          FREERTOS_DIR=~/FreeRTOS-Kernel
# =============================================================================

# ---- 툴체인 ----------------------------------------------------------------
PREFIX  ?= arm-none-eabi-
CC      := $(PREFIX)gcc
AS      := $(PREFIX)gcc -x assembler-with-cpp
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump
SIZE    := $(PREFIX)size
PYTHON  ?= python3

# ---- 디렉터리 --------------------------------------------------------------
ROOT_DIR     := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
BUILD_DIR    ?= $(ROOT_DIR)/build
THIRD_PARTY  ?= $(ROOT_DIR)/third_party

# 벤더 소스 위치입니다. 외부에서 덮어쓸 수 있습니다.
HAL_DIR      ?= $(THIRD_PARTY)/stm32f4xx_hal_driver
FREERTOS_DIR ?= $(THIRD_PARTY)/FreeRTOS-Kernel
# CMSIS 장치/코어 헤더는 이 저장소가 이미 03_static_proof/cmsis 에 보관 중입니다.
# (stm32f446xx.h, core_cm4.h, stm32f4xx_hal_conf.h 등 49종)
CMSIS_DIR    ?= $(ROOT_DIR)/../03_static_proof/cmsis

# ---- 타깃 이름 --------------------------------------------------------------
APP_TARGET  := app
BOOT_TARGET := boot
APP_BUILD   := $(BUILD_DIR)/app
BOOT_BUILD  := $(BUILD_DIR)/boot

# ---- CPU / 아키텍처 ---------------------------------------------------------
# Cortex-M4F: 단정밀도 FPU(fpv4-sp-d16)를 하드웨어 ABI 로 사용합니다.
# FreeRTOS ARM_CM4F 포트가 FPU 컨텍스트를 저장하도록 되어 있으므로 일치해야 합니다.
CPU_FLAGS := -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# ---- 공통 매크로 ------------------------------------------------------------
# HSE_VALUE: stm32f4xx_hal_conf.h 의 기본값은 25MHz 이지만 NUCLEO-F446RE 는
#   ST-LINK MCO 에서 8MHz 를 받습니다. 이 값이 틀리면 PLL 계산이 어긋나
#   SystemClock_Config() 가 180MHz 가 아닌 엉뚱한 주파수를 만듭니다. (필수)
COMMON_DEFS := -DSTM32F446xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000

# ---- 공통 컴파일 플래그 -----------------------------------------------------
#  -ffunction-sections/-fdata-sections + --gc-sections : 미사용 심볼 제거
#  -fno-common          : 중복 정의를 링크 에러로 드러냄
#  -fstack-usage        : 함수별 .su 파일 생성 → 03_static_proof 스택 분석 입력
#  -fno-tree-loop-distribute-patterns
#                       : 복사 루프가 memcpy 호출로 치환되는 것을 막습니다.
#                         app/tiny_libc.c 의 memcpy 가 자기 자신을 호출하는
#                         무한 재귀를 방지하기 위한 안전장치입니다.
CFLAGS_COMMON := $(CPU_FLAGS) $(COMMON_DEFS) \
                 -std=gnu11 -Og -g3 -gdwarf-4 \
                 -Wall -Wextra -Wshadow -Wundef -Wdouble-promotion \
                 -ffunction-sections -fdata-sections -fno-common \
                 -fstack-usage -fno-tree-loop-distribute-patterns \
                 -MMD -MP

ASFLAGS := $(CPU_FLAGS) $(COMMON_DEFS) -g3 -MMD -MP

# ---- 링크 플래그 ------------------------------------------------------------
# 이 프로젝트는 -nostdlib 로 링크합니다.
#   이유 1) 검증에 사용한 툴체인에 newlib(libc.a) 가 없습니다.
#   이유 2) 무엇보다, 링크되는 코드가 전부 이 저장소 안에 있어야 정적 스택
#           분석(03_static_proof)과 WCET 근거가 성립합니다. newlib 의 printf 는
#           스택을 수백 바이트 쓰고 힙과 재진입 구조체에 의존합니다.
# 대신 필요한 것만 직접 제공합니다:
#   app/tiny_libc.c   memcpy/memset/strlen/... 과 __libc_init_array
#   app/tiny_printf.c 정수 전용 printf (JSON 로그용)
# libgcc 는 여전히 필요합니다. __aeabi_uidiv 등 정수 나눗셈과 부동소수점
# 소프트웨어 루틴이 여기 들어 있습니다. (정적 증빙에서 발견된 결함 #3)
# libgcc.a 의 실제 경로를 찾습니다. -lgcc 로 넘기지 않고 파일을 직접 주는 이유는,
# 툴체인이 요청한 멀티리브 조합(thumb/v7e-m+fp/hard)을 정확히 갖고 있지 않은 경우
# -print-libgcc-file-name 이 경로 없이 'libgcc.a' 만 돌려주고 링크가 실패하기
# 때문입니다. 그럴 때는 설치 트리에서 하드 float 변형을 찾아 씁니다.
LIBGCC_PATH := $(realpath $(shell $(CC) $(CPU_FLAGS) -print-libgcc-file-name))
ifeq ($(LIBGCC_PATH),)
# 폴백: 툴체인 설치 루트에서 하드 float(v7e-m) 변형을 직접 찾습니다.
GCC_INSTALL_DIR := $(shell $(CC) -print-search-dirs | sed -n 's/^install: //p')
LIBGCC_PATH := $(firstword $(wildcard $(GCC_INSTALL_DIR)thumb/v7e-m+dp/hard/libgcc.a) \
                          $(wildcard $(GCC_INSTALL_DIR)thumb/v7e-m+fp/hard/libgcc.a) \
                          $(wildcard $(GCC_INSTALL_DIR)thumb/v7e-m*/hard/libgcc.a))
endif

LDFLAGS_COMMON := $(CPU_FLAGS) -nostdlib -static \
                  -Wl,--gc-sections \
                  -Wl,--print-memory-usage

# libgcc 는 --gc-sections 뒤에 마지막으로 붙입니다. (__aeabi_uidiv 등 해결용)
LIBS := $(LIBGCC_PATH)

# =============================================================================
# 인클루드 경로
# =============================================================================
INC_CMSIS := -I$(CMSIS_DIR)
INC_BSP   := -I$(ROOT_DIR)/bsp
INC_HAL   := -I$(HAL_DIR)/Inc
INC_APP   := -I$(ROOT_DIR)/app
INC_RTOS  := -I$(FREERTOS_DIR)/include -I$(FREERTOS_DIR)/portable/GCC/ARM_CM4F

# =============================================================================
# 소스 목록
# =============================================================================

# ---- HAL 드라이버 (hal_conf.h 에서 활성화한 모듈만) -------------------------
HAL_MODULES := hal hal_cortex hal_rcc hal_rcc_ex hal_gpio hal_dma hal_dma_ex \
               hal_pwr hal_pwr_ex hal_flash hal_flash_ex hal_flash_ramfunc \
               hal_can hal_adc hal_adc_ex hal_i2c hal_i2c_ex \
               hal_tim hal_tim_ex hal_uart hal_iwdg
HAL_SRCS := $(foreach m,$(HAL_MODULES),$(HAL_DIR)/Src/stm32f4xx_$(m).c)

# 부트로더는 HAL 을 최소한만 씁니다. 16KB 섹터 하나에 들어가야 하기 때문입니다.
# (CAN/ADC/I2C/TIM/UART 드라이버는 링크하지 않습니다)
HAL_SRCS_BOOT := $(addprefix $(HAL_DIR)/Src/stm32f4xx_, \
                   hal.c hal_cortex.c hal_rcc.c hal_rcc_ex.c hal_gpio.c \
                   hal_pwr.c hal_pwr_ex.c hal_flash.c hal_flash_ex.c \
                   hal_flash_ramfunc.c hal_iwdg.c hal_dma.c hal_dma_ex.c)

# ---- FreeRTOS 커널 (앱 전용) ------------------------------------------------
RTOS_SRCS := $(FREERTOS_DIR)/tasks.c \
             $(FREERTOS_DIR)/queue.c \
             $(FREERTOS_DIR)/list.c \
             $(FREERTOS_DIR)/portable/GCC/ARM_CM4F/port.c \
             $(FREERTOS_DIR)/portable/MemMang/heap_4.c

# ---- 애플리케이션 -----------------------------------------------------------
APP_SRCS_C := $(ROOT_DIR)/main_fixed.c \
              $(ROOT_DIR)/app/stm32f4xx_it.c \
              $(ROOT_DIR)/app/app_hooks.c \
              $(ROOT_DIR)/app/tiny_libc.c \
              $(ROOT_DIR)/app/tiny_printf.c \
              $(ROOT_DIR)/system/system_stm32f4xx.c \
              $(ROOT_DIR)/bsp/system_stm32.c \
              $(ROOT_DIR)/bsp/bsp_can.c \
              $(ROOT_DIR)/bsp/bsp_adc.c \
              $(ROOT_DIR)/bsp/bsp_pwm.c \
              $(ROOT_DIR)/bsp/bsp_gpio.c \
              $(ROOT_DIR)/bsp/bsp_uart.c \
              $(ROOT_DIR)/bsp/bsp_iwdg.c \
              $(ROOT_DIR)/bsp/bsp_flash.c \
              $(ROOT_DIR)/bsp/fw_image.c \
              $(ROOT_DIR)/bsp/bootloader.c \
              $(ROOT_DIR)/bsp/sensor_bmp280.c \
              $(HAL_SRCS) $(RTOS_SRCS)
APP_SRCS_S := $(ROOT_DIR)/startup/startup_stm32f446xx.s

# ---- 부트로더 ---------------------------------------------------------------
# FreeRTOS 없이 동작합니다. BSP_BAREMETAL 매크로가 bsp_baremetal.h 를 끌어와
# BaseType_t / pdPASS 등 최소 정의만 제공합니다.
BOOT_SRCS_C := $(ROOT_DIR)/bootloader/boot_main.c \
               $(ROOT_DIR)/app/tiny_libc.c \
               $(ROOT_DIR)/system/system_stm32f4xx.c \
               $(ROOT_DIR)/bsp/bsp_iwdg.c \
               $(ROOT_DIR)/bsp/bsp_flash.c \
               $(ROOT_DIR)/bsp/fw_image.c \
               $(HAL_SRCS_BOOT)
BOOT_SRCS_S := $(ROOT_DIR)/startup/startup_stm32f446xx.s

# =============================================================================
# 타깃별 플래그
# =============================================================================

# ---- 앱 ---------------------------------------------------------------------
# USER_VECT_TAB_ADDRESS + VECT_TAB_OFFSET=0x4000 :
#   system_stm32f4xx.c 의 SystemInit() 이 SCB->VTOR 를 0x08004000 으로 설정하게
#   합니다. 이것이 없으면 앱이 섹터 0(부트로더)의 벡터 테이블을 계속 사용해서,
#   인터럽트가 발생하는 순간 부트로더의 핸들러로 점프해 버립니다.
APP_CFLAGS  := $(CFLAGS_COMMON) \
               -DUSER_VECT_TAB_ADDRESS -DVECT_TAB_OFFSET=0x4000 \
               $(INC_CMSIS) $(INC_HAL) $(INC_BSP) $(INC_APP) $(INC_RTOS)
APP_LDFLAGS := $(LDFLAGS_COMMON) -T$(ROOT_DIR)/linker/STM32F446RE_APP.ld \
               -Wl,-Map=$(APP_BUILD)/$(APP_TARGET).map

# ---- 부트로더 ---------------------------------------------------------------
# VTOR 는 리셋 기본값(0x08000000)을 그대로 씁니다.
# -Os : 16KB 섹터 하나에 들어가야 하므로 크기를 우선합니다.
# BSP_BAREMETAL : FreeRTOS 헤더 대신 bsp_baremetal.h 를 사용하게 합니다.
BOOT_CFLAGS  := $(filter-out -Og,$(CFLAGS_COMMON)) -Os -DBSP_BAREMETAL \
                $(INC_CMSIS) $(INC_HAL) $(INC_BSP) $(INC_APP)
BOOT_LDFLAGS := $(LDFLAGS_COMMON) -T$(ROOT_DIR)/linker/STM32F446RE_BOOT.ld \
                -Wl,-Map=$(BOOT_BUILD)/$(BOOT_TARGET).map

# =============================================================================
# 오브젝트 파일 경로 (소스 트리 구조를 build/ 아래에 그대로 재현)
# =============================================================================
# 저장소 밖(third_party) 소스는 절대 경로라서 그대로 쓰면 build/ 를 벗어납니다.
# notdir 로 파일명만 뽑아 vendor/ 하위에 평평하게 놓습니다. (파일명 충돌 없음)
app_obj  = $(APP_BUILD)/$(if $(filter $(ROOT_DIR)%,$1),local/$(patsubst $(ROOT_DIR)/%,%,$1),vendor/$(notdir $1))
boot_obj = $(BOOT_BUILD)/$(if $(filter $(ROOT_DIR)%,$1),local/$(patsubst $(ROOT_DIR)/%,%,$1),vendor/$(notdir $1))

APP_OBJS  := $(foreach s,$(APP_SRCS_C),$(patsubst %.c,%.o,$(call app_obj,$(s))))
APP_OBJS  += $(foreach s,$(APP_SRCS_S),$(patsubst %.s,%.o,$(call app_obj,$(s))))
BOOT_OBJS := $(foreach s,$(BOOT_SRCS_C),$(patsubst %.c,%.o,$(call boot_obj,$(s))))
BOOT_OBJS += $(foreach s,$(BOOT_SRCS_S),$(patsubst %.s,%.o,$(call boot_obj,$(s))))

DEPS := $(APP_OBJS:.o=.d) $(BOOT_OBJS:.o=.d)

# =============================================================================
# 기본 타깃
# =============================================================================
.PHONY: all app boot image combined size clean distclean deps check-deps \
        flash-app flash-boot flash-combined help

all: boot app image

help:
	@echo 'STM32F446RE 2단 이미지 빌드'
	@echo '  make deps       벤더 소스(HAL/FreeRTOS)를 third_party/ 로 받습니다'
	@echo '  make            부트로더 + 앱 + 갱신 이미지 전부 빌드'
	@echo '  make boot       부트로더만 (섹터 0, 16KB)'
	@echo '  make app        앱만 (섹터 1~4, 112KB)'
	@echo '  make image      앱 바이너리에 갱신용 32B 헤더 부착'
	@echo '  make combined   부트로더+앱 합본 (공장 출하용)'
	@echo '  make size       영역별 사용량/여유 출력'
	@echo '  make clean      산출물 삭제'

# =============================================================================
# 의존성 확보
# =============================================================================
deps:
	@mkdir -p $(THIRD_PARTY)
	@if [ ! -d "$(HAL_DIR)/Src" ]; then \
	    echo ">> STM32F4xx HAL 드라이버를 받습니다..."; \
	    git clone --depth 1 https://github.com/STMicroelectronics/stm32f4xx_hal_driver.git \
	        $(THIRD_PARTY)/stm32f4xx_hal_driver; \
	else echo ">> HAL 이미 존재: $(HAL_DIR)"; fi
	@if [ ! -d "$(FREERTOS_DIR)/include" ]; then \
	    echo ">> FreeRTOS 커널을 받습니다..."; \
	    git clone --depth 1 https://github.com/FreeRTOS/FreeRTOS-Kernel.git \
	        $(THIRD_PARTY)/FreeRTOS-Kernel; \
	else echo ">> FreeRTOS 이미 존재: $(FREERTOS_DIR)"; fi
	@echo ">> 준비 완료. 이제 'make' 를 실행하십시오."

check-deps:
	@test -d "$(CMSIS_DIR)" || { echo "!! CMSIS 헤더가 없습니다: $(CMSIS_DIR)"; exit 1; }
	@test -d "$(HAL_DIR)/Src" || { echo "!! HAL 소스가 없습니다: $(HAL_DIR)"; echo "   'make deps' 를 먼저 실행하거나 HAL_DIR=... 로 지정하십시오."; exit 1; }
	@test -d "$(FREERTOS_DIR)/include" || { echo "!! FreeRTOS 커널이 없습니다: $(FREERTOS_DIR)"; echo "   'make deps' 를 먼저 실행하거나 FREERTOS_DIR=... 로 지정하십시오."; exit 1; }
	@test -f "$(LIBGCC_PATH)" || { echo "!! libgcc.a 를 찾지 못했습니다 (경로: '$(LIBGCC_PATH)')"; echo "   하드 float 멀티리브가 있는 arm-none-eabi 툴체인이 필요합니다."; exit 1; }

# =============================================================================
# 애플리케이션 빌드
# =============================================================================
app: check-deps $(APP_BUILD)/$(APP_TARGET).bin $(APP_BUILD)/$(APP_TARGET).hex

$(APP_BUILD)/local/%.o: $(ROOT_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "  CC(app)   $(patsubst $(ROOT_DIR)/%,%,$<)"
	@$(CC) $(APP_CFLAGS) -c $< -o $@

$(APP_BUILD)/local/%.o: $(ROOT_DIR)/%.s
	@mkdir -p $(dir $@)
	@echo "  AS(app)   $(patsubst $(ROOT_DIR)/%,%,$<)"
	@$(AS) $(ASFLAGS) -c $< -o $@

# 벤더 소스(HAL/FreeRTOS)는 경로가 제각각이라 vpath 로 해결합니다.
vpath %.c $(HAL_DIR)/Src $(FREERTOS_DIR) \
          $(FREERTOS_DIR)/portable/GCC/ARM_CM4F $(FREERTOS_DIR)/portable/MemMang

$(APP_BUILD)/vendor/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "  CC(app)   [vendor] $(notdir $<)"
	@$(CC) $(APP_CFLAGS) -Wno-unused-parameter -c $< -o $@

$(APP_BUILD)/$(APP_TARGET).elf: $(APP_OBJS)
	@mkdir -p $(dir $@)
	@echo "  LD        $(notdir $@)"
	@$(CC) $(APP_OBJS) $(APP_LDFLAGS) $(LIBS) -o $@
	@$(SIZE) $@

# =============================================================================
# 부트로더 빌드
# =============================================================================
boot: check-deps $(BOOT_BUILD)/$(BOOT_TARGET).bin $(BOOT_BUILD)/$(BOOT_TARGET).hex

$(BOOT_BUILD)/local/%.o: $(ROOT_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "  CC(boot)  $(patsubst $(ROOT_DIR)/%,%,$<)"
	@$(CC) $(BOOT_CFLAGS) -c $< -o $@

$(BOOT_BUILD)/local/%.o: $(ROOT_DIR)/%.s
	@mkdir -p $(dir $@)
	@echo "  AS(boot)  $(patsubst $(ROOT_DIR)/%,%,$<)"
	@$(AS) $(ASFLAGS) -c $< -o $@

$(BOOT_BUILD)/vendor/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "  CC(boot)  [vendor] $(notdir $<)"
	@$(CC) $(BOOT_CFLAGS) -Wno-unused-parameter -c $< -o $@

$(BOOT_BUILD)/$(BOOT_TARGET).elf: $(BOOT_OBJS)
	@mkdir -p $(dir $@)
	@echo "  LD        $(notdir $@)"
	@$(CC) $(BOOT_OBJS) $(BOOT_LDFLAGS) $(LIBS) -o $@
	@$(SIZE) $@

# =============================================================================
# 산출물 변환
# =============================================================================
%.bin: %.elf
	@echo "  OBJCOPY   $(notdir $@)"
	@$(OBJCOPY) -O binary -S $< $@

%.hex: %.elf
	@echo "  OBJCOPY   $(notdir $@)"
	@$(OBJCOPY) -O ihex $< $@

%.lst: %.elf
	@$(OBJDUMP) -h -S $< > $@

# ---- 갱신용 이미지 ----------------------------------------------------------
# 앱 바이너리 앞에 32바이트 헤더(매직/크기/CRC-32/버전/로드주소/헤더CRC)를 붙입니다.
# 이 파일을 통째로 장치에 전송하면 앱이 스테이징 영역에 기록하고,
# 부트로더가 같은 형식으로 검증합니다. (형식 정의: bsp/fw_image.h)
FW_VERSION ?= 0x00010000

image: $(APP_BUILD)/$(APP_TARGET)_image.bin

$(APP_BUILD)/$(APP_TARGET)_image.bin: $(APP_BUILD)/$(APP_TARGET).bin
	@echo "  IMAGE     $(notdir $@)"
	@$(PYTHON) $(ROOT_DIR)/tools/make_image.py \
	    --input $< --output $@ --version $(FW_VERSION)

# ---- 공장 출하용 합본 -------------------------------------------------------
# 0x08000000 에 부트로더, 0x08004000 에 앱을 배치한 하나의 바이너리입니다.
# 갭은 0xFF(플래시 소거 상태)로 채웁니다.
combined: $(BUILD_DIR)/combined.bin

$(BUILD_DIR)/combined.bin: $(BOOT_BUILD)/$(BOOT_TARGET).bin $(APP_BUILD)/$(APP_TARGET).bin
	@echo "  COMBINE   $(notdir $@)"
	@$(PYTHON) $(ROOT_DIR)/tools/make_combined.py \
	    --boot $(BOOT_BUILD)/$(BOOT_TARGET).bin \
	    --app  $(APP_BUILD)/$(APP_TARGET).bin \
	    --output $@

# =============================================================================
# 크기 점검 — 각 이미지가 배정된 영역에 들어가는지 확인합니다.
# =============================================================================
size: $(BOOT_BUILD)/$(BOOT_TARGET).bin $(APP_BUILD)/$(APP_TARGET).bin
	@echo ""
	@echo "=== 플래시 영역 사용량 ==============================================="
	@printf "부트로더 (섹터 0,   16384 B): %7d B  (%d B 여유)\n" \
	    $$(stat -c%s $(BOOT_BUILD)/$(BOOT_TARGET).bin) \
	    $$((16384 - $$(stat -c%s $(BOOT_BUILD)/$(BOOT_TARGET).bin)))
	@printf "앱       (섹터 1~4, 114688 B): %7d B  (%d B 여유)\n" \
	    $$(stat -c%s $(APP_BUILD)/$(APP_TARGET).bin) \
	    $$((114688 - $$(stat -c%s $(APP_BUILD)/$(APP_TARGET).bin)))
	@echo "스테이징 (섹터 5,  131072 B): 앱 이미지 + 512 B 메타 영역"
	@echo "======================================================================"
	@echo ""
	@$(SIZE) $(BOOT_BUILD)/$(BOOT_TARGET).elf $(APP_BUILD)/$(APP_TARGET).elf

# =============================================================================
# 기록 (st-flash 사용. openocd 를 쓴다면 이 부분을 교체하십시오)
# =============================================================================
flash-boot: $(BOOT_BUILD)/$(BOOT_TARGET).bin
	st-flash --reset write $< 0x08000000

flash-app: $(APP_BUILD)/$(APP_TARGET).bin
	st-flash --reset write $< 0x08004000

flash-combined: $(BUILD_DIR)/combined.bin
	st-flash --reset write $< 0x08000000

# =============================================================================
# 정리
# =============================================================================
clean:
	@rm -rf $(BUILD_DIR)
	@echo "산출물을 삭제했습니다."

distclean: clean
	@rm -rf $(THIRD_PARTY)
	@echo "벤더 소스도 삭제했습니다."

-include $(DEPS)
