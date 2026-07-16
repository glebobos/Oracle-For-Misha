#pragma once

#include <stdint.h>

// Returns five column bitmaps; bit 0 is the top pixel. Unknown characters use '?'.
const uint8_t *font_glyph(uint32_t codepoint);
