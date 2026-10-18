# Requires devkitARM (devkitPro) with arm-none-eabi-gcc and gba toolchain.
TARGET := aventura
BUILD := build
SRC := src/main.c
CC := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
CFLAGS := -mthumb -mthumb-interwork -O2 -Wall -Wextra -std=c99 -mcpu=arm7tdmi -mtune=arm7tdmi -ffreestanding
LDFLAGS := -specs=gba.specs -Wl,--gc-sections

all: $(TARGET).gba
$(BUILD):
	mkdir -p $(BUILD)
$(BUILD)/main.o: $(SRC) src/title_data.h | $(BUILD)
	$(CC) $(CFLAGS) -c $(SRC) -o $@
$(TARGET).elf: $(BUILD)/main.o
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@
$(TARGET).gba: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
clean:
	rm -rf $(BUILD) $(TARGET).elf $(TARGET).gba
.PHONY: all clean
