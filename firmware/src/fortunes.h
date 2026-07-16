#pragma once

#include <stddef.h>

#define FORTUNE_COUNT 120

void fortunes_shuffle(void);
const char *fortunes_next(void);
size_t fortunes_positive_count(void);
size_t fortunes_negative_count(void);
