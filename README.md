# NonceSense - Task 0: Bare-Metal Blinky

## Hardware Details
* **Target MCU:** STM32F401RE 
* **Pin Mapping:**
  * **LED:** GPIOA Pin 5 (PA5)
  * **Button:** (Not required for Task 0)
  * **UART TX/RX:** (Not required for Task 0)

## Register Addresses & Offsets
These addresses were pulled directly from the STM32 Reference Manual memory map to control the hardware without CMSIS headers:

* **RCC (Reset and Clock Control)**
  * Base Address: `0x40023800`
  * **RCC_AHB1ENR:** Offset `0x30` -> Address `0x40023830` (Used to enable the clock for Port A)
* **GPIOA (General Purpose I/O Port A)**
  * Base Address: `0x40020000`
  * **GPIOA_MODER:** Offset `0x00` -> Address `0x40020000` (Used to configure PA5 as an output pin)
  * **GPIOA_ODR:** Offset `0x14` -> Address `0x40020014` (Used to write 1s and 0s to toggle the LED)

## Boot Flow (From Reset to main)
In a bare-metal environment, this is how the chip gets to our `main()` function:
1. **Power On/Reset:** The moment the chip is powered, the hardware fetches the Initial Stack Pointer value from the very start of memory (`0x00000000`) and loads it.
2. **Reset Vector:** It then fetches the address of the `Reset_Handler` function from `0x00000004` and jumps to it. 
3. **Memory Setup:** Inside our custom startup file, the Reset Handler copies the initialized variables (`.data` section) from Flash into RAM, and fills uninitialized variables (`.bss` section) in RAM with zeros.
4. **Branch to main:** Once the memory is ready, the startup script calls `main()`, and our C code starts executing the blink loop.