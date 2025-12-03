#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __linux__
#endif

#ifndef __i668__
#endif

enum vga_color 	{
    VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t) c | (uint16_t) color << 8;
}

#define MAX_HEIGHT 25
#define MAX_WIDTH 80
#define VGA_MEMORY 0xB8000

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t *terminal_buffer = (uint16_t*)VGA_MEMORY;

void terminal_init() {
	terminal_row = 0;
	terminal_column = 0;

	for (int y=0; y < MAX_HEIGHT; y++) {
		for (int x=0; x < MAX_WIDTH; x++) {
			const size_t index = y * MAX_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', VGA_COLOR_BLACK);
		}
	}
}

void terminal_putentrychar(char c) {
	const size_t index = terminal_row * MAX_WIDTH + terminal_column;
	terminal_buffer[index] = vga_entry(c, VGA_COLOR_LIGHT_GREY);
}

void terminal_putchar(char c) {
	if (c == '\n') {
		terminal_column = 0;
		terminal_row++;
		return;
	}
	else if (c == 8) {
		terminal_column--;
		terminal_putentrychar(' ');
		return;
	}
	else if (c == '\t') {
		terminal_column += 4;
		return;
	}

	if (terminal_column >= MAX_WIDTH) {
		terminal_column = 0;
		terminal_row++;
	}
	if (terminal_row >= MAX_HEIGHT) {
		terminal_row = 0;
	}

	terminal_putentrychar(c);
	terminal_column++;
}

void terminal_write(char *s) {
	while (*s) {
		terminal_putchar(*s);
		s++;
	}
}

// --- I/O Port Access ---

static inline void outb(uint16_t port, uint8_t val) {
	__asm__ volatile (
		"outb %0, %1"
		:
		: "a"(val), "Nd"(port)
	);
}

static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	__asm__ volatile (
		"inb %1, %0"
		: "=a"(ret)
		: "Nd"(port)
	);
	return ret;
}

// -------------------------

// Định nghĩa các cổng I/O của bàn phím (i8042 PS/2 Controller)
#define KBD_DATA_PORT 0x60 // Dữ liệu mã quét

// --- Keyboard Handler ---

// Bảng mã quét cơ bản (Chỉ Series 1 - không bao gồm Shift, Alt, Ctrl, v.v.)
// Đây là một bảng rất đơn giản, chỉ chứa các ký tự cơ bản, không xử lý dịch phím (shift)
static const unsigned char kbdus[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, '\\',
  'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*',
    0,  ' ',   0,
    0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,
    0,
    0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

bool cmpstr(const char *s1, const char *s2) {
    while (*s1 != '\0' && *s2 != '\0') {
        if (*s1 != *s2) {
            return false; // Khác nhau
        }
        s1++;
        s2++;
    }
    // Trả về true chỉ khi cả hai cùng kết thúc (cùng độ dài)
    return *s1 == '\0' && *s2 == '\0'; 
}

#define BUFFER_SIZE 64
char buffer[BUFFER_SIZE];
int idx = 0;

void command_handler(char *command) {
	if (cmpstr(command, "exit")) {
		terminal_write("Goodbye!");
		__asm__ volatile ("hlt");
	}
	else if (cmpstr(command, "osinfo")) {
		terminal_write("----------\nOS Name: SharkaOS\nVersion: 0.0.1-alpha\nCode: Big Bang\nGithub Repository: https://github.com/nguyenbinh-dev11/SharkaOS\n----------\n");
	}
}

void keyboard_handler() {
    uint8_t scancode;

    // Đọc mã quét từ cổng dữ liệu của bàn phím
    scancode = inb(KBD_DATA_PORT);

    // Kiểm tra xem phím có được nhấn (Key Press) hay không.
    // Mã quét Key Press luôn nhỏ hơn 0x80 (128). Key Release là Key Press + 0x80.
    if (scancode < 0x80) {
        char key_char = kbdus[scancode];

        if (key_char != 0) {
            terminal_putchar(key_char);

			if (key_char == '\n') {
				buffer[idx] = '\0';
				command_handler(buffer);
				idx = 0;
				return;
			}
			if (idx < BUFFER_SIZE - 1) { 
				buffer[idx++] = key_char;
			}
        }
    }
}
// -------------------------

void kmain() {
	terminal_init();
	terminal_write("Booting...\n");
	terminal_write("Welcome to SharkaOS!\n");
	terminal_write("----------\n");

	// Thay vì chạy mãi (như trong OS thực), chúng ta sẽ kiểm tra cổng bàn phím
    // Đây là Polling - kiểm tra liên tục
    while (1) {
        // Địa chỉ port 0x64 là Command/Status Port.
        // Bit 0 của Status Port (0x64) là Output Buffer Status. 
        // Nếu bit này là 1, có dữ liệu bàn phím sẵn sàng để đọc ở port 0x60.
        if (inb(0x64) & 0x1) { 
            keyboard_handler();
        }
    }
}