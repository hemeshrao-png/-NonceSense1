/*
 * task0/startup.c - hand-written startup file for STM32F401/F411 (Cortex-M4).
 * Only <stdint.h> is used. Provides:
 *   1. the vector table (placed at 0x08000000 by the linker script)
 *   2. Reset_Handler: copies .data from flash to RAM, zeroes .bss, calls main
 *
 * Only the 16 core entries are listed; peripheral IRQs (WWDG, EXTI, TIMx...)
 * would follow at index 16+. Add them here when you start using interrupts.
 */
#include <stdint.h>

/* Symbols created by linker.ld */
extern uint32_t _estack;            /* top of RAM = initial stack pointer   */
extern uint32_t _sidata;            /* where .data initial values live (flash) */
extern uint32_t _sdata, _edata;     /* .data start/end in RAM               */
extern uint32_t _sbss,  _ebss;      /* .bss start/end in RAM                */

int  main(void);
void Reset_Handler(void);
void Default_Handler(void);

typedef void (*vector_t)(void);

/* The first two words are special: [0]=initial SP, [1]=reset vector.
 * The remaining core exceptions point to Default_Handler (infinite loop). */
__attribute__((section(".isr_vector"), used))
const vector_t g_vectors[16] = {
    (vector_t)(uintptr_t)&_estack, /*  0 initial stack pointer */
    Reset_Handler,                 /*  1 Reset                 */
    Default_Handler,               /*  2 NMI                   */
    Default_Handler,               /*  3 HardFault             */
    Default_Handler,               /*  4 MemManage             */
    Default_Handler,               /*  5 BusFault              */
    Default_Handler,               /*  6 UsageFault            */
    0, 0, 0, 0,                    /*  7-10 reserved           */
    Default_Handler,               /* 11 SVCall                */
    Default_Handler,               /* 12 DebugMon              */
    0,                             /* 13 reserved              */
    Default_Handler,               /* 14 PendSV                */
    Default_Handler                /* 15 SysTick               */
};

void Default_Handler(void) { for (;;) { } }

void Reset_Handler(void)
{
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;

    while (dst < &_edata) { *dst++ = *src++; }   /* copy initialised globals */
    for (dst = &_sbss; dst < &_ebss; ) { *dst++ = 0; } /* zero uninitialised */

    (void)main();
    for (;;) { }                                  /* main must never return */
}
