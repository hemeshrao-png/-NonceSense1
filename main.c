/*
 * task0/main.c - bare-metal blinky, STM32F103 (Blue Pill), LED on PC13.
 * No headers except <stdint.h>. Every register address is defined below
 * from the RM0008 reference manual.  Clock = default 8 MHz HSI (no setup).
 */
#include <stdint.h>

#define REG32(addr)   (*(volatile uint32_t *)(addr))

#define RCC_BASE      0x40021000UL
#define RCC_APB2ENR   REG32(RCC_BASE + 0x18UL)   /* peripheral clock enable */
#define RCC_IOPCEN    (1UL << 4)                 /* GPIOC clock             */

#define GPIOC_BASE    0x40011000UL
#define GPIOC_CRH     REG32(GPIOC_BASE + 0x04UL) /* config pins 8..15       */
#define GPIOC_BSRR    REG32(GPIOC_BASE + 0x10UL) /* atomic set/reset        */

#define LED_PIN       13u                        /* PC13, active LOW        */

/* Rough busy-wait. ~6 CPU cycles per loop pass at -O2 on Cortex-M3,
 * 8 MHz -> 500 ms ~= 4,000,000 cycles ~= 666,000 passes. Tune on hardware. */
#define HALF_PERIOD_LOOPS 666000UL

static void delay(volatile uint32_t n) { while (n--) { } }

int main(void)
{
    RCC_APB2ENR |= RCC_IOPCEN;                   /* 1. clock the GPIOC block */

    /* 2. PC13 is pin 13 -> CRH bits [23:20]. MODE=10 (2 MHz out), CNF=00
     *    (push-pull) => nibble 0x2. */
    GPIOC_CRH = (GPIOC_CRH & ~(0xFUL << 20)) | (0x2UL << 20);

    for (;;) {
        GPIOC_BSRR = 1UL << (LED_PIN + 16u);     /* reset bit -> pin LOW -> LED ON  */
        delay(HALF_PERIOD_LOOPS);
        GPIOC_BSRR = 1UL << LED_PIN;             /* set bit   -> pin HIGH -> LED OFF */
        delay(HALF_PERIOD_LOOPS);
    }
}
