# NonceSense - bare-metal 2FA token (STM32F401/F411 Black Pill)

Status: **Task 0 done, Task 1 done**. Tasks 2 and 3 not attempted yet.

## Target MCU and pin mapping
| Function | Pin | Notes |
|---|---|---|
| LED | PC13 | on-board, active LOW |
| UART TX | PA2 (USART2_TX, AF7) | connect to adapter RX |
| UART RX | PA3 (USART2_RX, AF7) | connect to adapter TX |
| GND | GND | common ground with the USB-UART adapter |
| Button | (Task 2) | not used yet |

Clock: default 16 MHz HSI, no PLL. UART 115200 8-N-1, BRR = 139.

## Register map used
| Block | Base | Registers (offset) |
|---|---|---|
| RCC | 0x40023800 | AHB1ENR 0x30 (GPIOAEN b0, GPIOCEN b2), APB1ENR 0x40 (USART2EN b17) |
| GPIOA | 0x40020000 | MODER 0x00, AFRL 0x20 |
| GPIOC | 0x40020800 | MODER 0x00, ODR 0x14, BSRR 0x18 |
| USART2 | 0x40004400 | SR 0x00, DR 0x04, BRR 0x08, CR1 0x0C |

(Task 0 uses RCC, GPIOC MODER and BSRR only.)

## Boot flow: reset to main()
1. On reset the core reads the initial stack pointer from 0x08000000 (`_estack`, top of RAM) and the reset vector from 0x08000004 (`Reset_Handler`). The linker script puts `.isr_vector` first in flash so these words are there.
2. `Reset_Handler` (startup.c) copies `.data` from its flash load address (`_sidata`) to RAM (`_sdata`..`_edata`), then zeroes `.bss` (`_sbss`..`_ebss`).
3. It calls `main()`. `main` never returns; if it did, `Reset_Handler` loops forever.

## Task 1 protocol
- Host sends `AUTH:<8 hex>\n`.
- Token replies `RESP:<8 hex>\n` where response = CRC32 (IEEE, same as zlib) over 8 bytes: challenge (big-endian) followed by the 32-bit key (big-endian).
- Anything malformed gets `ERR\n`. LED toggles on every successful reply.
- Key is `SECRET_KEY` in `main.c` and `KEY` in `host_test.py`.

## Build / run
```
make            # builds build/noncesense.elf and .bin
make flash      # OpenOCD + ST-Link
python3 host_test.py /dev/ttyUSB0
```
Task 0 is in `task0/` with its own Makefile (`cd task0 && make`).

## Logs
(Paste the output of `host_test.py` here, or a terminal capture.)
