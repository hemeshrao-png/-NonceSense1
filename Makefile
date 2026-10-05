# task0/Makefile - build the bare-metal blinky (STM32F4). Targets: make, make clean, make flash
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size
CPU     = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
CFLAGS  = $(CPU) -Wall -Wextra -O2 -nostdlib -nostartfiles -ffreestanding \
          -fno-builtin -fno-tree-loop-distribute-patterns -ffunction-sections -fdata-sections
LDFLAGS = $(CPU) -nostdlib -nostartfiles -T linker.ld -Wl,--gc-sections -Wl,-Map=build/task0.map
TARGET  = build/task0
SRCS    = startup.c main.c
OBJS    = $(SRCS:%.c=build/%.o)

all: $(TARGET).elf $(TARGET).bin
$\t$(SIZE) $(TARGET).elf

build/%.o: %.c | build
$\t$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS) linker.ld
$\t$(CC) $(OBJS) $(LDFLAGS) -o $@

$(TARGET).bin: $(TARGET).elf
$\t$(OBJCOPY) -O binary $< $@

build:
$\tmkdir -p build

# ST-Link + OpenOCD. (Alternative over USB DFU: dfu-util -a 0 -s 0x08000000:leave -D $(TARGET).bin)
flash: $(TARGET).elf
$\topenocd -f interface/stlink.cfg -f target/stm32f4x.cfg \\
$\t        -c "program $(TARGET).elf verify reset exit"

clean:
$\trm -rf build

.PHONY: all clean flash
