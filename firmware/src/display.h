#pragma once

#include <stdbool.h>
#include <stdint.h>

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

void display_init(void);
void display_clear(void);
void display_present(void);
void display_pixel(int x, int y, bool on);
void display_text(int x, int y, const char *utf8);
int display_text_width(const char *utf8);
void display_centered(int y, const char *utf8);
void display_multiline_centered(int y, const char *utf8, int line_height);
void display_wrapped_centered(int y, const char *utf8, int line_height);
void display_spinner(uint32_t frame);
void display_heart(int x, int y);
