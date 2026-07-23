#include "display.h"
#include "fortunes.h"
#include "touch.h"

#include "pico/stdlib.h"
#include "pico/rand.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    INTRO_HELLO,
    INTRO_GIFT,
    INTRO_SUN,
    WISH_PROMPT,
    SPINNING,
    SHOWING_FORTUNE,
} ScreenState;

typedef struct {
    const char *text;
    int text_height;
    uint32_t duration_ms;
    bool heart;
} IntroSlide;

static const IntroSlide intro_slides[] = {
    {"Прывітанне,\nсябра!\n\n▼", 38, 4000, false},
    {"Гэты маленькі\nпрыборчык зроблены\nспецыяльна\nдля цябе", 38, 5200, true},
    {"Спытай пра шлях\nці «так ці ні»,\nі аракул\nвырашыць!", 38, 4400, false},
};

static const char *const startup_prompt =
    "Ну што, Мішаня,\nя запусціўся!\nСпытай «так ці ні?»\nі крані пыпку!";

static const char *const prompt_variants[] = {
    "Мішаня, ёсць яшчэ пытанне? Крані капялюшык!",
    "Міша, сумневы? Правер шлях, чапні грыбок!",
    "Мішунь, спытай «так ці ні?», крані носік!",
    "Мішаня, верны шлях ці не? Дакраніся да пыпкі!",
    "Міша, новае пытанне? Смела тронь капялюшык!",
    "Мішунь, хочаш яшчэ адказ? Чапні пупку!",
    "Мішаня, спытай аракула, крані грыбок!",
    "Міша, шукаеш знак? Тронь носік!",
    "Мішунь, так ці ні? Чапні капялюшык!",
    "Мішаня, яшчэ адно пытанне? Крані пыпку!",
    "Міша, правер сваю дарогу, дакраніся да грыбка!",
    "Мішунь, верны шлях? Тронь макушку!",
    "Мішаня, спытай сусвет, чапні носік!",
    "Міша, ну што, па яшчэ адзін адказ? Крані капялюшык!",
    "Мішунь, лаві новы вердыкт, дакраніся да пупкі!",
    "Мішаня, пытанні не скончыліся? Чапні пыпку!",
    "Міша, правер тэму на вошы. Тронь грыбок!",
    "Мішунь, верны шлях ці тупік? Крані капялюшык!",
    "Мішаня, час для новага адказу. Дакраніся да носіка!",
    "Міша, спытай яшчэ разок! Чапні грыбок!",
};

static ScreenState state = INTRO_HELLO;
static uint32_t state_started_ms;
static const char *current_fortune;
static const char *current_prompt;
static uint8_t last_prompt_index = 0xff;
static bool startup_prompt_pending = true;

static uint32_t now_ms(void) { return to_ms_since_boot(get_absolute_time()); }

static void choose_next_prompt(void) {
    const uint8_t count = (uint8_t)(sizeof(prompt_variants) / sizeof(prompt_variants[0]));
    uint8_t index = 0;
    if (last_prompt_index == 0xff) {
        index = 0;
    } else {
        index = (uint8_t)(get_rand_32() % count);
        if (count > 1 && index == last_prompt_index) index = (uint8_t)((index + 1) % count);
    }
    last_prompt_index = index;
    current_prompt = prompt_variants[index];
}

static void enter_state(ScreenState next) {
    if (next == WISH_PROMPT) {
        if (startup_prompt_pending) {
            current_prompt = startup_prompt;
            startup_prompt_pending = false;
        } else {
            choose_next_prompt();
        }
    }
    state = next;
    state_started_ms = now_ms();
}

static void draw_intro(const IntroSlide *slide, uint32_t elapsed_ms) {
    const int distance = DISPLAY_HEIGHT + slide->text_height;
    const int y = DISPLAY_HEIGHT - (int)((uint64_t)distance * elapsed_ms / slide->duration_ms);
    display_multiline_centered(y, slide->text, 10);
    if (slide->heart) display_heart(104, y + 31);
}

static void draw_prompt(void) {
    if (current_prompt == startup_prompt) {
        display_multiline_centered(7, current_prompt, 10);
    } else {
        display_wrapped_centered(8, current_prompt, 10);
    }
}

static void draw_spinner(uint32_t elapsed_ms) {
    static const char *const captions[] = {"Чэкаю шлях...", "Аналізую тэму...", "Лоўлю вердыкт..."};
    display_centered(7, captions[(elapsed_ms / 420) % 3]);
    display_spinner(elapsed_ms / 100);
    display_centered(48, "Мішуне, секунду!");
}

int main(void) {
    stdio_init_all();
    display_init();
    touch_init();
    fortunes_shuffle();
    current_prompt = startup_prompt;
    state_started_ms = now_ms();

    while (true) {
        touch_update();
        const uint32_t elapsed = now_ms() - state_started_ms;
        const bool pressed = touch_take_press();

        display_clear();
        switch (state) {
            case INTRO_HELLO:
            case INTRO_GIFT:
            case INTRO_SUN: {
                const IntroSlide *slide = &intro_slides[state];
                draw_intro(slide, elapsed);
                if (pressed) enter_state(WISH_PROMPT);
                else if (elapsed >= slide->duration_ms) enter_state((ScreenState)(state + 1));
                break;
            }
            case WISH_PROMPT:
                draw_prompt();
                if (pressed) enter_state(SPINNING);
                break;
            case SPINNING:
                draw_spinner(elapsed);
                if (elapsed >= 2800) {
                    current_fortune = fortunes_next();
                    enter_state(SHOWING_FORTUNE);
                }
                break;
            case SHOWING_FORTUNE:
                display_wrapped_centered(7, current_fortune, 9);
                if (elapsed >= 7000) enter_state(WISH_PROMPT);
                break;
        }
        display_present();
        sleep_ms(20);
    }
}
