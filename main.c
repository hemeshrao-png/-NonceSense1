/*
 * task0/main.c - bare-metal blinky, STM32F401/F411 (Black Pill), LED on PC13.
 * No headers except <stdint.h>. Every register address is defined below
 * from the RM0368 (F401) / RM0383 (F411) reference manual.
 * Clock = default 16 MHz HSI (no setup).
 */
#include <stdint.h>

#define REG32(addr)   (*(volatile uint32_t *)(addr))

#define RCC_BASE      0x40023800UL
#define RCC_AHB1ENR   REG32(RCC_BASE + 0x30UL)   /* AHB1 peripheral clock enable */
#define RCC_GPIOCEN   (1UL << 2)                 /* GPIOC clock                  */

#define GPIOC_BASE    0x40020800UL
#define GPIOC_MODER   REG32(GPIOC_BASE + 0x00UL) /* 2 bits per pin: mode         */
#define GPIOC_BSRR    REG32(GPIOC_BASE + 0x18UL) /* atomic set/reset             */

#define LED_PIN       13u                        /* PC13, active LOW             */

/* Rough busy-wait. ~6 CPU cycles per loop pass at -O2 on Cortex-M4,
 * 16 MHz -> 500 ms ~= 8,000,000 cycles ~= 1,333,000 passes. Tune on hardware. */
#define HALF_PERIOD_LOOPS 1333000UL

static void delay(volatile uint32_t n) { while (n--) { } }

int main(void)
{
    RCC_AHB1ENR |= RCC_GPIOCEN;                  /* 1. clock the GPIOC block */
    (void)RCC_AHB1ENR;                           /*    read back: let clock settle */

    /* 2. PC13 -> MODER bits [27:26]. MODE=01 => general purpose output.
     *    OTYPER resets to 0 (push-pull), so nothing else to configure. */
    GPIOC_MODER = (GPIOC_MODER & ~(0x3UL << (LED_PIN * 2u)))
                | (0x1UL << (LED_PIN * 2u));

    for (;;) {
        GPIOC_BSRR = 1UL << (LED_PIN + 16u);     /* reset bit -> pin LOW -> LED ON  */
        delay(HALF_PERIOD_LOOPS);
        GPIOC_BSRR = 1UL << LED_PIN;             /* set bit   -> pin HIGH -> LED OFF */
        delay(HALF_PERIOD_LOOPS);
    }
}
