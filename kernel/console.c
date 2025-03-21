/**
 * NKOF Console Implementation
 * 
 * This file implements the console interface for text output in the kernel.
 * It provides functions to write text to the screen using the VGA text mode.
 */

#include "include/console.h"
#include "include/types.h"

// Video memory address for VGA text mode
static uint16_t* const video_memory = (uint16_t*)0xB8000;

// Current position and color
static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t current_color = 0;

/**
 * Make an entry in the VGA text buffer.
 */
static inline uint16_t make_vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

/**
 * Create a color attribute from foreground and background colors.
 */
static inline uint8_t make_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

/**
 * Initialize the console by clearing the screen.
 */
void console_init(void) {
    current_color = make_color(COLOR_LIGHT_GREY, COLOR_BLACK);
    console_clear();
}

/**
 * Clear the entire console screen.
 */
void console_clear(void) {
    uint16_t blank = make_vga_entry(' ', current_color);
    
    for (int y = 0; y < CONSOLE_HEIGHT; y++) {
        for (int x = 0; x < CONSOLE_WIDTH; x++) {
            video_memory[y * CONSOLE_WIDTH + x] = blank;
        }
    }
    
    cursor_x = 0;
    cursor_y = 0;
}

/**
 * Set the foreground and background colors for subsequent writes.
 */
void console_set_color(uint8_t fg, uint8_t bg) {
    current_color = make_color(fg, bg);
}

/**
 * Scroll the console up one line.
 */
static void console_scroll(void) {
    // Move all existing lines up
    for (int y = 0; y < CONSOLE_HEIGHT - 1; y++) {
        for (int x = 0; x < CONSOLE_WIDTH; x++) {
            video_memory[y * CONSOLE_WIDTH + x] = video_memory[(y + 1) * CONSOLE_WIDTH + x];
        }
    }
    
    // Clear the bottom line
    uint16_t blank = make_vga_entry(' ', current_color);
    for (int x = 0; x < CONSOLE_WIDTH; x++) {
        video_memory[(CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH + x] = blank;
    }
}

/**
 * Write a single character to the console.
 */
void console_put_char(char c) {
    // Handle special characters
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
        }
    } else if (c == '\t') {
        // Tab size is 8 spaces
        cursor_x = (cursor_x + 8) & ~(8 - 1);
    } else {
        // Regular character
        uint16_t* location = video_memory + (cursor_y * CONSOLE_WIDTH + cursor_x);
        *location = make_vga_entry(c, current_color);
        cursor_x++;
    }
    
    // Handle line wrap
    if (cursor_x >= CONSOLE_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    
    // Handle scrolling
    if (cursor_y >= CONSOLE_HEIGHT) {
        console_scroll();
        cursor_y = CONSOLE_HEIGHT - 1;
    }
}

/**
 * Write a string to the console.
 */
void console_write_string(const char* str) {
    while (*str) {
        console_put_char(*str);
        str++;
    }
}

/**
 * Write a decimal integer to the console.
 */
void console_write_int(int value) {
    // Handle negative numbers
    if (value < 0) {
        console_put_char('-');
        value = -value;
    }
    
    // Special case for zero
    if (value == 0) {
        console_put_char('0');
        return;
    }
    
    // Convert the number to characters
    char buffer[12]; // Enough for 32-bit int
    int pos = 0;
    
    while (value > 0) {
        buffer[pos++] = '0' + (value % 10);
        value /= 10;
    }
    
    // Print the digits in reverse (correct) order
    while (pos > 0) {
        console_put_char(buffer[--pos]);
    }
}

/**
 * Write a hexadecimal number to the console.
 */
void console_write_hex(uint32_t value) {
    // Print "0x" prefix
    console_write_string("0x");
    
    // Special case for zero
    if (value == 0) {
        console_put_char('0');
        return;
    }
    
    // Convert the number to hex characters
    char buffer[8]; // Enough for 32-bit int (8 hex digits)
    int pos = 0;
    
    while (value > 0 && pos < 8) {
        uint8_t digit = value & 0xF;
        buffer[pos++] = digit < 10 ? '0' + digit : 'A' + (digit - 10);
        value >>= 4;
    }
    
    // Print the digits in reverse (correct) order
    while (pos > 0) {
        console_put_char(buffer[--pos]);
    }
}

/**
 * Set the cursor position.
 */
void console_set_cursor(int x, int y) {
    if (x >= 0 && x < CONSOLE_WIDTH && y >= 0 && y < CONSOLE_HEIGHT) {
        cursor_x = x;
        cursor_y = y;
    }
}