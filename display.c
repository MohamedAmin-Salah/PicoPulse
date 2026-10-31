
#include "display.h"

#include "pico/stdlib.h"
#include "hardware/spi.h"

#define TFT_SPI spi0

#define TFT_SCK  18
#define TFT_MOSI 19
#define TFT_CS   17
#define TFT_DC   20
#define TFT_RST  21

static void select_display(void)
{
    gpio_put(TFT_CS, 0);
}

static void deselect_display(void)
{
    gpio_put(TFT_CS, 1);
}

static void write_command(uint8_t command)
{
    gpio_put(TFT_DC, 0);
    select_display();
    spi_write_blocking(TFT_SPI, &command, 1);
    deselect_display();
}

static void write_data(const uint8_t *data, size_t length)
{
    gpio_put(TFT_DC, 1);
    select_display();
    spi_write_blocking(TFT_SPI, data, length);
    deselect_display();
}

static void write_byte(uint8_t value)
{
    write_data(&value, 1);
}

static void set_window(uint16_t x0, uint16_t y0,
                       uint16_t x1, uint16_t y1)
{
    uint8_t column[] = {
        (uint8_t)(x0 >> 8), (uint8_t)x0,
        (uint8_t)(x1 >> 8), (uint8_t)x1
    };

    uint8_t row[] = {
        (uint8_t)(y0 >> 8), (uint8_t)y0,
        (uint8_t)(y1 >> 8), (uint8_t)y1
    };

    write_command(0x2A); // CASET
    write_data(column, sizeof(column));

    write_command(0x2B); // RASET
    write_data(row, sizeof(row));

    write_command(0x2C); // RAMWR
}

void display_init(void)
{
    spi_init(TFT_SPI, 1 * 1000 * 1000);
    spi_set_format(TFT_SPI, 8,
                   SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(TFT_SCK, GPIO_FUNC_SPI);
    gpio_set_function(TFT_MOSI, GPIO_FUNC_SPI);

    gpio_init(TFT_CS);
    gpio_set_dir(TFT_CS, GPIO_OUT);
    gpio_put(TFT_CS, 1);

    gpio_init(TFT_DC);
    gpio_set_dir(TFT_DC, GPIO_OUT);
    gpio_put(TFT_DC, 1);

    gpio_init(TFT_RST);
    gpio_set_dir(TFT_RST, GPIO_OUT);

    // Hardvarureset
    gpio_put(TFT_RST, 1);
    sleep_ms(10);
    gpio_put(TFT_RST, 0);
    sleep_ms(20);
    gpio_put(TFT_RST, 1);
    sleep_ms(150);

    write_command(0x01); // SWRESET
    sleep_ms(150);

    write_command(0x11); // SLPOUT
    sleep_ms(150);

    write_command(0x3A); // COLMOD
    write_byte(0x05);    // 16-bit RGB565

    write_command(0x36); // MADCTL
    write_byte(0xC8);

    // ST7735S: konfigurera normal visning.
    write_command(0x20); // INVOFF
    sleep_ms(10);

    write_command(0x13); // NORON
    sleep_ms(10);

    // Aktivera full displayyta.
    write_command(0x2A); // CASET
    uint8_t col[] = {0x00, 0x00, 0x00, 0x7F};
    write_data(col, sizeof(col));

    write_command(0x2B); // RASET
    uint8_t row[] = {0x00, 0x00, 0x00, 0x9F};
    write_data(row, sizeof(row));
    sleep_ms(10);

    write_command(0x29); // DISPON
    sleep_ms(100);

    display_fill(DISPLAY_BLACK);
}

void display_rect(uint16_t x, uint16_t y,
                  uint16_t width, uint16_t height,
                  uint16_t color)
{
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT ||
        width == 0 || height == 0) {
        return;
    }

    if (width > DISPLAY_WIDTH - x) {
        width = DISPLAY_WIDTH - x;
    }

    if (height > DISPLAY_HEIGHT - y) {
        height = DISPLAY_HEIGHT - y;
    }

    set_window(x, y,
               x + width - 1,
               y + height - 1);

    // Skicka fargdata i block.
    uint8_t pixels[128];

    for (size_t i = 0; i < sizeof(pixels); i += 2) {
        pixels[i] = (uint8_t)(color >> 8);
        pixels[i + 1] = (uint8_t)color;
    }

    uint32_t remaining = (uint32_t)width * height;

    gpio_put(TFT_DC, 1);
    select_display();

    while (remaining > 0) {
        uint32_t count =
            remaining > 64 ? 64 : remaining;

        spi_write_blocking(TFT_SPI, pixels, count * 2);
        remaining -= count;
    }

    deselect_display();
}

void display_fill(uint16_t color)
{
    display_rect(0, 0,
                 DISPLAY_WIDTH, DISPLAY_HEIGHT, color);
}


/*
 * Enkel 5x7-font.
 * Varje byte beskriver en vertikal kolumn.
 * Bit 0 ar den oversta pixeln.
 */

static const uint8_t font_letters[26][5] = {
    {0x7E,0x11,0x11,0x11,0x7E}, // A
    {0x7F,0x49,0x49,0x49,0x36}, // B
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x41,0x41,0x22,0x1C}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x09,0x09,0x09,0x01}, // F
    {0x3E,0x41,0x49,0x49,0x7A}, // G
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x00,0x41,0x7F,0x41,0x00}, // I
    {0x20,0x40,0x41,0x3F,0x01}, // J
    {0x7F,0x08,0x14,0x22,0x41}, // K
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x7F,0x02,0x0C,0x02,0x7F}, // M
    {0x7F,0x04,0x08,0x10,0x7F}, // N
    {0x3E,0x41,0x41,0x41,0x3E}, // O
    {0x7F,0x09,0x09,0x09,0x06}, // P
    {0x3E,0x41,0x51,0x21,0x5E}, // Q
    {0x7F,0x09,0x19,0x29,0x46}, // R
    {0x46,0x49,0x49,0x49,0x31}, // S
    {0x01,0x01,0x7F,0x01,0x01}, // T
    {0x3F,0x40,0x40,0x40,0x3F}, // U
    {0x1F,0x20,0x40,0x20,0x1F}, // V
    {0x7F,0x20,0x18,0x20,0x7F}, // W
    {0x63,0x14,0x08,0x14,0x63}, // X
    {0x03,0x04,0x78,0x04,0x03}, // Y
    {0x61,0x51,0x49,0x45,0x43}  // Z
};

static const uint8_t font_digits[10][5] = {
    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0x00,0x42,0x7F,0x40,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46}, // 2
    {0x21,0x41,0x45,0x4B,0x31}, // 3
    {0x18,0x14,0x12,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 6
    {0x01,0x71,0x09,0x05,0x03}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x06,0x49,0x49,0x29,0x1E}  // 9
};

static const uint8_t font_space[5] =
    {0,0,0,0,0};

static const uint8_t font_colon[5] =
    {0,0x36,0x36,0,0};

static const uint8_t font_dot[5] =
    {0,0x60,0x60,0,0};

static const uint8_t font_dash[5] =
    {0x08,0x08,0x08,0x08,0x08};

static const uint8_t *font_get(char c)
{
    if (c >= 'A' && c <= 'Z') {
        return font_letters[c - 'A'];
    }

    if (c >= 'a' && c <= 'z') {
        return font_letters[c - 'a'];
    }

    if (c >= '0' && c <= '9') {
        return font_digits[c - '0'];
    }

    switch (c) {
        case ':': return font_colon;
        case '.': return font_dot;
        case '-': return font_dash;
        default:  return font_space;
    }
}

void display_draw_char(uint16_t x, uint16_t y,
                       char character, uint16_t color,
                       uint16_t background, uint8_t scale)
{
    if (scale == 0 || scale > 3) {
        return;
    }

    uint16_t width = 6 * scale;
    uint16_t height = 8 * scale;

    if (x + width > DISPLAY_WIDTH ||
        y + height > DISPLAY_HEIGHT) {
        return;
    }

    const uint8_t *glyph = font_get(character);

    // Maxstorlek: 6*3*8*3 pixlar, 2 byte per pixel.
    uint8_t pixels[864];
    size_t position = 0;

    for (uint16_t py = 0; py < height; py++) {
        uint16_t row = py / scale;

        for (uint16_t px = 0; px < width; px++) {
            uint16_t col = px / scale;

            bool active =
                col < 5 &&
                row < 7 &&
                ((glyph[col] >> row) & 1u);

            uint16_t pixel_color =
                active ? color : background;

            pixels[position++] =
                (uint8_t)(pixel_color >> 8);

            pixels[position++] =
                (uint8_t)pixel_color;
        }
    }

    set_window(x, y,
               x + width - 1,
               y + height - 1);

    write_data(pixels, position);
}

void display_draw_text(uint16_t x, uint16_t y,
                       const char *text, uint16_t color,
                       uint16_t background, uint8_t scale)
{
    if (text == NULL || scale == 0 || scale > 3) {
        return;
    }

    while (*text != '\0') {
        uint16_t width = 6 * scale;

        if (x + width > DISPLAY_WIDTH) {
            break;
        }

        display_draw_char(x, y, *text,
                          color, background, scale);

        x += width;
        text++;
    }
}
