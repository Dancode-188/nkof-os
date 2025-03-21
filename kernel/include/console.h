/**
 * NKOF Console Interface
 * 
 * This file contains declarations for the console functions that handle
 * text output to the screen in the kernel.
 */

#ifndef NKOF_CONSOLE_H
#define NKOF_CONSOLE_H

#include "types.h"

// Console dimensions
#define CONSOLE_WIDTH  80
#define CONSOLE_HEIGHT 25

// Color attributes for text
#define COLOR_BLACK         0
#define COLOR_BLUE          1
#define COLOR_GREEN         2
#define COLOR_CYAN          3
#define COLOR_RED           4
#define COLOR_MAGENTA       5
#define COLOR_BROWN         6
#define COLOR_LIGHT_GREY    7
#define COLOR_DARK_GREY     8
#define COLOR_LIGHT_BLUE    9
#define COLOR_LIGHT_GREEN   10
#define COLOR_LIGHT_CYAN    11
#define COLOR_LIGHT_RED     12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_LIGHT_BROWN   14
#define COLOR_WHITE         15

// Initialize the console system
void console_init(void);

// Clear the screen
void console_clear(void);

// Set the foreground and background colors
void console_set_color(uint8_t foreground, uint8_t background);

// Write a single character to the console
void console_put_char(char c);

// Write a string to the console
void console_write_string(const char* str);

// Write a decimal integer to the console
void console_write_int(int value);

// Write a hexadecimal number to the console
void console_write_hex(uint32_t value);

// Position the cursor
void console_set_cursor(int x, int y);

#endif /* NKOF_CONSOLE_H */