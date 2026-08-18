/* STM32F446RE_BOOT.ld — 부트로더(섹터 0) 링커 스크립트입니다.
 *
 * 부트로더는 리셋 벡터가 위치한 0x08000000 에서 시작하며 섹터 0 (16KB)만 사용합니다.
 * 16KB 를 초과하면 링크가 실패하도록 되어 있어, 부트로더가 앱 영역을 침범하는 사고를
 * 빌드 시점에 막습니다. 이 값은 bsp/flash_map.h 의 BOOT_REGION_ADDR /
 * BOOT_REGION_SIZE 와 반드시 일치해야 합니다.
 *
 *   FLASH  0x08000000 ~ 0x08003FFF  (16KB, 섹터 0)
 *   RAM    0x20000000 ~ 0x2001FFFF  (128KB) — 앱으로 점프하기 전까지만 사용합니다.
 *
 * 부트로더는 SystemInit() 기본 동작(SCB->VTOR = 0x08000000)을 그대로 쓰므로
 * USER_VECT_TAB_ADDRESS 를 정의하지 않습니다.
 */

ENTRY(Reset_Handler)

_estack = 0x20020000;

_Min_Stack_Size = 0x400;   /* 부트로더가 사용할 MSP 최소 여유(1KB)입니다. */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 16K
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH

  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    *(.RamFunc)
    *(.RamFunc*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> FLASH

  .bss :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /* -nostdlib 로 링크하므로 표준 C 라이브러리는 애초에 들어오지 않습니다.
     (필요한 memcpy/printf 등은 app/tiny_libc.c, app/tiny_printf.c 가 제공)
     여기서는 툴체인이 넣는 C++ 예외 정보만 버립니다. 이 프로젝트는 C 전용이고
     예외를 쓰지 않으므로 .ARM.exidx 등은 순수한 낭비입니다. */
  /DISCARD/ :
  {
    *(.ARM.exidx*)
    *(.ARM.extab*)
    *(.gnu.linkonce.armexidx.*)
    *(.eh_frame*)
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
