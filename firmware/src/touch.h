#pragma once

#include <stdbool.h>
#include <stdint.h>

void touch_init(void);
void touch_update(void);
bool touch_take_press(void);
bool touch_is_pressed(void);
