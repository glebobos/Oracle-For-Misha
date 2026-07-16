#include "display.h"

#include "font.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <string.h>

#define SSD1306_ADDRESS 0x3C
// XIAO RP2350 pin labels D4/D5 are GPIO6/GPIO7 on I2C1.
#define I2C_PORT i2c1
#define I2C_SDA_PIN 6
#define I2C_SCL_PIN 7

static uint8_t framebuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT / 8];

static void command(uint8_t value) {
    uint8_t packet[2] = {0x00, value};
    i2c_write_blocking(I2C_PORT, SSD1306_ADDRESS, packet, 2, false);
}

static uint32_t decode_utf8(const char **text) {
    const uint8_t *s = (const uint8_t *)*text;
    uint32_t codepoint;
    if (s[0] < 0x80) {
        codepoint = s[0];
        *text += 1;
    } else if ((s[0] & 0xe0) == 0xc0) {
        codepoint = ((s[0] & 0x1f) << 6) | (s[1] & 0x3f);
        *text += 2;
    } else if ((s[0] & 0xf0) == 0xe0) {
        codepoint = ((s[0] & 0x0f) << 12) | ((s[1] & 0x3f) << 6) | (s[2] & 0x3f);
        *text += 3;
    } else {
        codepoint = '?';
        *text += 1;
    }
    return codepoint;
}

void display_init(void) {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    sleep_ms(100);
    const uint8_t init_commands[] = {
        0xae, 0x20, 0x00, 0x40, 0x81, 0x7f, 0xa1, 0xa6, 0xa8, 0x3f,
        0xc8, 0xd3, 0x00, 0xda, 0x12, 0xd5, 0x80, 0xd9, 0xf1, 0xdb,
        0x40, 0x8d, 0x14, 0xaf,
    };
    for (size_t i = 0; i < sizeof(init_commands); ++i) command(init_commands[i]);
    display_clear();
    display_present();
}

void display_clear(void) { memset(framebuffer, 0, sizeof(framebuffer)); }

void display_present(void) {
    command(0x21); command(0); command(DISPLAY_WIDTH - 1);
    command(0x22); command(0); command(7);
    uint8_t packet[17] = {0x40};
    for (size_t offset = 0; offset < sizeof(framebuffer); offset += 16) {
        memcpy(&packet[1], &framebuffer[offset], 16);
        i2c_write_blocking(I2C_PORT, SSD1306_ADDRESS, packet, sizeof(packet), false);
    }
}

void display_pixel(int x, int y, bool on) {
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT) return;
    uint8_t *pixel = &framebuffer[x + (y / 8) * DISPLAY_WIDTH];
    const uint8_t mask = 1u << (y & 7);
    if (on) *pixel |= mask; else *pixel &= (uint8_t)~mask;
}

static void display_char(int x, int y, uint32_t codepoint) {
    const uint8_t *glyph = font_glyph(codepoint);
    for (int column = 0; column < 5; ++column) {
        for (int row = 0; row < 7; ++row) {
            if (glyph[column] & (1u << row)) display_pixel(x + column, y + row, true);
        }
    }
}

void display_text(int x, int y, const char *utf8) {
    while (*utf8) {
        if (*utf8 == '\n') { y += 9; x = 0; ++utf8; continue; }
        display_char(x, y, decode_utf8(&utf8));
        x += 6;
    }
}

int display_text_width(const char *utf8) {
    int width = 0, line_width = 0;
    while (*utf8) {
        if (*utf8 == '\n') { if (line_width > width) width = line_width; line_width = 0; ++utf8; }
        else { (void)decode_utf8(&utf8); line_width += 6; }
    }
    return line_width > width ? line_width : width;
}

void display_centered(int y, const char *utf8) {
    const int x = (DISPLAY_WIDTH - display_text_width(utf8)) / 2;
    display_text(x < 0 ? 0 : x, y, utf8);
}

void display_multiline_centered(int y, const char *utf8, int line_height) {
    const char *line = utf8;
    char buffer[96];
    while (*line) {
        size_t length = 0;
        while (line[length] && line[length] != '\n' && length < sizeof(buffer) - 1) ++length;
        memcpy(buffer, line, length); buffer[length] = '\0';
        display_centered(y, buffer);
        y += line_height;
        line += length;
        if (*line == '\n') ++line;
    }
}

void display_wrapped_centered(int y, const char *utf8, int line_height) {
    char line[64] = {0};
    char word[64];
    while (*utf8) {
        size_t length = 0;
        while (*utf8 == ' ') ++utf8;
        while (utf8[length] && utf8[length] != ' ' && utf8[length] != '\n' && length < sizeof(word) - 1) ++length;
        memcpy(word, utf8, length); word[length] = '\0';
        if (utf8[length] == '\n') ++length;
        const int candidate_width = display_text_width(line) + (line[0] ? 6 : 0) + display_text_width(word);
        if (line[0] && candidate_width > DISPLAY_WIDTH) {
            display_centered(y, line);
            y += line_height;
            line[0] = '\0';
        }
        if (line[0]) strncat(line, " ", sizeof(line) - strlen(line) - 1);
        strncat(line, word, sizeof(line) - strlen(line) - 1);
        utf8 += length;
        if (*utf8 == '\n') ++utf8;
    }
    if (line[0]) display_centered(y, line);
}

void display_spinner(uint32_t frame) {
    static const int points[8][2] = {{0,-8},{6,-6},{8,0},{6,6},{0,8},{-6,6},{-8,0},{-6,-6}};
    const int active = frame % 8;
    for (int i = 0; i < 8; ++i) {
        const int radius = i == active ? 2 : 1;
        for (int dx = -radius; dx <= radius; ++dx)
            for (int dy = -radius; dy <= radius; ++dy)
                if (dx * dx + dy * dy <= radius * radius)
                    display_pixel(64 + points[i][0] + dx, 28 + points[i][1] + dy, true);
    }
}

void display_heart(int x, int y) {
    static const uint8_t heart[7] = {0x36, 0x7f, 0x7f, 0x3e, 0x1c, 0x08, 0x00};
    for (int row = 0; row < 7; ++row)
        for (int col = 0; col < 7; ++col)
            if (heart[row] & (1u << col)) display_pixel(x + col, y + row, true);
}
