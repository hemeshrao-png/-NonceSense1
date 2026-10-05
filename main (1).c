/*
 * main.c - NonceSense, Task 1: UART challenge-response handshake.
 * Target : STM32F401/F411 "Black Pill" (Cortex-M4), 16 MHz HSI (no clock setup)
 * Pins   : PA2 = USART2_TX (AF7), PA3 = USART2_RX (AF7), PC13 = LED (active LOW)
 * Refs   : RM0368 (F401) / RM0383 (F411)
 *
 * Protocol (115200 8-N-1, lines end with '\n'):
 *   host -> token : AUTH:<8 hex>\n
 *   token -> host : RESP:<8 hex>\n     (CRC32 over challenge||key, big-endian bytes)
 *   anything else : ERR\n
 * LED toggles on every successful reply.
 *
 * All peripherals are driven through direct register access (no HAL/CMSIS).
 */
#include <stdint.h>
#include "crc32.h"

#define SECRET_KEY    0xA5C3E1F7UL   /* must match KEY in host_test.py */

#define REG32(addr)   (*(volatile uint32_t *)(addr))

/* ---------------- RCC ---------------- */
#define RCC_BASE        0x40023800UL
#define RCC_AHB1ENR     REG32(RCC_BASE + 0x30UL)
#define RCC_APB1ENR     REG32(RCC_BASE + 0x40UL)
#define RCC_GPIOAEN     (1UL << 0)
#define RCC_GPIOCEN     (1UL << 2)
#define RCC_USART2EN    (1UL << 17)

/* ---------------- GPIO ---------------- */
#define GPIOA_BASE      0x40020000UL
#define GPIOA_MODER     REG32(GPIOA_BASE + 0x00UL)
#define GPIOA_AFRL      REG32(GPIOA_BASE + 0x20UL)   /* AF select, pins 0..7 */

#define GPIOC_BASE      0x40020800UL
#define GPIOC_MODER     REG32(GPIOC_BASE + 0x00UL)
#define GPIOC_ODR       REG32(GPIOC_BASE + 0x14UL)
#define GPIOC_BSRR      REG32(GPIOC_BASE + 0x18UL)

#define LED_PIN         13u

/* ---------------- USART2 (APB1, 16 MHz) ---------------- */
#define USART2_BASE     0x40004400UL
#define USART2_SR       REG32(USART2_BASE + 0x00UL)
#define USART2_DR       REG32(USART2_BASE + 0x04UL)
#define USART2_BRR      REG32(USART2_BASE + 0x08UL)
#define USART2_CR1      REG32(USART2_BASE + 0x0CUL)

#define USART_SR_ORE    (1UL << 3)
#define USART_SR_RXNE   (1UL << 5)
#define USART_SR_TC     (1UL << 6)
#define USART_SR_TXE    (1UL << 7)
#define USART_CR1_RE    (1UL << 2)
#define USART_CR1_TE    (1UL << 3)
#define USART_CR1_UE    (1UL << 13)

#define PCLK1_HZ        16000000UL
#define BAUD            115200UL
/* Oversampling x16: BRR = fck / baud = 138.89 -> 139 (error 0.08 %) */
#define BRR_VALUE       ((PCLK1_HZ + BAUD / 2UL) / BAUD)

#define LINE_MAX        32u

/* ------------------------------------------------------------------ */
static void led_init(void)
{
    RCC_AHB1ENR |= RCC_GPIOCEN;
    (void)RCC_AHB1ENR;
    GPIOC_BSRR  = 1UL << LED_PIN;                       /* HIGH = LED off */
    GPIOC_MODER = (GPIOC_MODER & ~(0x3UL << (LED_PIN * 2u)))
                | (0x1UL << (LED_PIN * 2u));            /* output */
}

static void led_toggle(void) { GPIOC_ODR ^= 1UL << LED_PIN; }

static void uart_init(void)
{
    RCC_AHB1ENR |= RCC_GPIOAEN;
    RCC_APB1ENR |= RCC_USART2EN;
    (void)RCC_APB1ENR;

    /* PA2, PA3 -> alternate function mode (MODER = 10) */
    GPIOA_MODER = (GPIOA_MODER & ~((0x3UL << 4) | (0x3UL << 6)))
                |               ((0x2UL << 4) | (0x2UL << 6));
    /* AF7 = USART2 : AFRL bits [11:8] (PA2) and [15:12] (PA3) */
    GPIOA_AFRL  = (GPIOA_AFRL & ~((0xFUL << 8) | (0xFUL << 12)))
                |               ((0x7UL << 8) | (0x7UL << 12));

    USART2_BRR = BRR_VALUE;
    USART2_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;  /* 8-N-1 default */
}

static void uart_putc(char c)
{
    while (!(USART2_SR & USART_SR_TXE)) { }
    USART2_DR = (uint32_t)(uint8_t)c;
}

static void uart_puts(const char *s) { while (*s) { uart_putc(*s++); } }

static char uart_getc(void)            /* blocking (polling) */
{
    for (;;) {
        uint32_t sr = USART2_SR;
        if (sr & (USART_SR_RXNE | USART_SR_ORE)) {
            return (char)(USART2_DR & 0xFFu);   /* reading DR also clears ORE */
        }
    }
}

/* ------------------------------------------------------------------ */
static int hex_val(char c)
{
    if (c >= '0' && c <= '9') { return c - '0'; }
    if (c >= 'A' && c <= 'F') { return c - 'A' + 10; }
    if (c >= 'a' && c <= 'f') { return c - 'a' + 10; }
    return -1;
}

/* Parse exactly 8 hex chars. Returns 1 on success. */
static int parse_hex8(const char *s, uint32_t *out)
{
    uint32_t v = 0;
    for (uint32_t i = 0; i < 8u; i++) {
        int h = hex_val(s[i]);
        if (h < 0) { return 0; }
        v = (v << 4) | (uint32_t)h;
    }
    *out = v;
    return 1;
}

static void put_hex8(uint32_t v)
{
    static const char digits[] = "0123456789ABCDEF";
    for (int shift = 28; shift >= 0; shift -= 4) {
        uart_putc(digits[(v >> shift) & 0xFu]);
    }
}

/* response = CRC32( challenge[3..0] || key[3..0] ), both big-endian */
static uint32_t compute_response(uint32_t challenge)
{
    uint8_t buf[8];
    uint32_t key = SECRET_KEY;
    for (uint32_t i = 0; i < 4u; i++) {
        buf[i]      = (uint8_t)(challenge >> (24u - 8u * i));
        buf[4u + i] = (uint8_t)(key       >> (24u - 8u * i));
    }
    return crc32_calc(buf, sizeof buf);
}

static int starts_with_auth(const char *s)
{
    return s[0] == 'A' && s[1] == 'U' && s[2] == 'T' && s[3] == 'H' && s[4] == ':';
}

static void handle_line(const char *line, uint32_t len)
{
    uint32_t challenge;

    if (len == 0u) { return; }                        /* blank line: ignore */

    if (len == 13u && starts_with_auth(line) && parse_hex8(line + 5, &challenge)) {
        uart_puts("RESP:");
        put_hex8(compute_response(challenge));
        uart_putc('\n');
        led_toggle();                                 /* only on success */
    } else {
        uart_puts("ERR\n");
    }
}

int main(void)
{
    char     line[LINE_MAX];
    uint32_t len = 0;
    int      overflow = 0;

    led_init();
    uart_init();

    for (;;) {
        char c = uart_getc();

        if (c == '\r') { continue; }                  /* tolerate CRLF */

        if (c == '\n') {
            if (overflow) { uart_puts("ERR\n"); }
            else          { handle_line(line, len); }
            len = 0;
            overflow = 0;
        } else if (len < LINE_MAX) {
            line[len++] = c;
        } else {
            overflow = 1;                             /* too long: drop until '\n' */
        }
    }
}
