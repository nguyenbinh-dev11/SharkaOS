# Makefile
NASM = nasm
CC = gcc
LD = ld
OBJCOPY = objcopy

os.bin: boot.o kernel.o
	i686-elf-gcc -T linker.ld -o $@ -ffreestanding -O2 -nostdlib $^ -lgcc

boot.o: boot.asm
	i686-elf-as $< -o $@
	
kernel.o: kernel.c
	i686-elf-gcc -Iinclude -c $< -o $@ -std=gnu99 -ffreestanding -O2 -Wall -Wextra

run: os.bin
	qemu-system-x86_64 -kernel os.bin

clean:
	rm -f *.bin *.o *.elf
