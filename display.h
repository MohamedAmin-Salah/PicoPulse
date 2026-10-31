
#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

// Displayens upplosning
#define DISPLAY_WIDTH  128
#define DISPLAY_HEIGHT 160

// RGB565-farger
#define DISPLAY_BLACK  0x0000
#define DISPLAY_WHITE  0xFFFF
#define DISPLAY_RED    0xF800
#define DISPLAY_GREEN  0x07E0
#define DISPLAY_BLUE   0x001F
#define DISPLAY_YELLOW 0xFFE0

void display_init(void);
void display_fill(uint16_t color);
void display_rect(uint16_t x, uint16_t y,
                  uint16_t width, uint16_t height,
                  uint16_t color);

void display_draw_char(uint16_t x, uint16_t y,
                       char character, uint16_t color,
                       uint16_t background, uint8_t scale);

void display_draw_text(uint16_t x, uint16_t y,
                       const char *text, uint16_t color,
                       uint16_t background, uint8_t scale);

#endif
