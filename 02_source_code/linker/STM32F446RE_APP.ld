/* STM32F446RE_APP.ld — 애플리케이션(섹터 1~4) 링커 스크립트입니다.
 *
 * 부트로더가 섹터 0(0x08000000~0x08003FFF)을 차지하므로, 애플리케이션은
 * 0x08004000 에서 시작합니다. 이 값은 bsp/flash_map.h 의 APP_REGION_ADDR /
 * APP_REGION_SIZE 와 반드시 일치해야 합니다.
 *
 *   FLASH  0x08004000 ~ 0x0801FFFF  (112KB, 섹터 1~4)
 *   RAM    0x20000000 ~ 0x2001FFFF  (128KB, SRAM1+SRAM2 연속)
 *
 * 애플리케이션은 벡터 테이블이 0x08004000 에 있으므로 SCB->VTOR 을 옮겨야 합니다.
 * 이는 Makefile 의 -DUSER_VECT_TAB_ADDRESS -DVECT_TAB_OFFSET=0x4000 으로 처리되어
 * SystemInit() 안에서 자동 설정됩니다.
 */

ENTRY(Reset_Handler)

/* 스택 최상단은 RAM 의 끝입니다. (풀 디센딩 스택) */
_estack = 0x20020000;

/* 링크 단계에서 최소 크기를 강제 검사하기 위한 값입니다. */
_Min_Heap_Size  = 0x000;   /* FreeRTOS heap_4 를 쓰므로 newlib 힙은 사용하지 않습니다. */
_Min_Stack_Size = 0x400;   /* 스케줄러 시작 전 및 ISR 이 사용할 MSP 최소 여유(1KB)입니다. */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08004000, LENGTH = 112K
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
  /* 인터럽트 벡터 테이블. 반드시 이미지의 맨 앞(0x08004000)에 와야 합니다. */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  /* 실행 코드입니다. */
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

  /* 읽기 전용 데이터(문자열 상수, const 배열)입니다. */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  /* 예외 처리용 테이블입니다. -fno-exceptions 여도 링커가 참조할 수 있어 유지합니다. */
  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  /* C++ 전역 생성자 / 초기화 배열입니다. __libc_init_array 가 순회합니다. */
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

  /* .data 의 초기값이 저장된 플래시 주소입니다. Reset_Handler 가 여기서 RAM 으로 복사합니다. */
  _sidata = LOADADDR(.data);

  /* 초기값이 있는 전역/정적 변수입니다. */
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

  /* 0 으로 초기화되는 변수입니다. Reset_Handler 가 클리어합니다. */
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

  /* 힙/스택을 위한 최소 여유가 실제로 남는지 링크 시점에 검증합니다.
   * 남지 않으면 여기서 "region RAM overflowed" 오류로 즉시 잡힙니다. */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
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
