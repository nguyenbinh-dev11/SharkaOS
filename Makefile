# Makefile
AS = i686-elf-as
CC = i686-elf-gcc
LD = i686-elf-ld

CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Iinclude

C_SOURCES = kernel/kernel.c
ASM_SOURCES = boot/boot.asm

TARGET = os.bin
OBJ = $(C_SOURCES:.c=.o) $(ASM_SOURCES:.asm=.o)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.asm
	$(AS) $< -o $@

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -nostdlib -T linker.ld $(OBJ) -o $@

run: $(TARGET)
	qemu-system-x86_64 -kernel $<

clean:
	rm -f $(OBJ) $(TARGET)