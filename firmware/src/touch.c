#include "touch.h"

#include "hardware/adc.h"
#include "pico/stdlib.h"

// Two-pin capacitive sensor on XIAO RP2350:
// D1/GPIO27 -- 1..4.7 MOhm -- D2/GPIO28 -- touch electrode.
//
// GPIO28 is sampled with the ADC after a short charge window. A finger adds
// capacitance, so the voltage rises more slowly and the ADC reading becomes
// lower than the idle baseline.
#define TOUCH_SEND_PIN 27
#define TOUCH_SENSE_PIN 28
#define TOUCH_ADC_INPUT 2
#define DISCHARGE_US 1200
#define CHARGE_SHORT_US 24
#define CHARGE_LONG_US 120
#define INIT_SAMPLES 24
#define PRESS_DEBOUNCE_SAMPLES 2
#define RELEASE_DEBOUNCE_SAMPLES 3

static uint32_t baseline_level;
static uint32_t threshold_level;
static uint32_t last_sample_level;
static uint8_t low_samples;
static uint8_t high_samples;
static bool pressed;
static bool press_event;

static uint16_t sample_level_after_us(uint32_t charge_us) {
    gpio_put(TOUCH_SEND_PIN, 0);
    busy_wait_us_32(DISCHARGE_US);

    gpio_put(TOUCH_SEND_PIN, 1);
    busy_wait_us_32(charge_us);
    const uint16_t level = adc_read();
    gpio_put(TOUCH_SEND_PIN, 0);
    return level;
}

static uint32_t sample_charge_level(void) {
    const uint32_t short_level = sample_level_after_us(CHARGE_SHORT_US);
    const uint32_t long_level = sample_level_after_us(CHARGE_LONG_US);
    return short_level + long_level;
}

static uint32_t compute_threshold(uint32_t baseline) {
    uint32_t threshold = baseline / 12;
    if (threshold < 120) threshold = 120;
    return threshold;
}

void touch_init(void) {
    gpio_init(TOUCH_SEND_PIN);
    gpio_set_dir(TOUCH_SEND_PIN, GPIO_OUT);
    gpio_put(TOUCH_SEND_PIN, 0);

    adc_init();
    adc_gpio_init(TOUCH_SENSE_PIN);
    adc_select_input(TOUCH_ADC_INPUT);

    uint32_t total = 0;
    for (int i = 0; i < INIT_SAMPLES; ++i) {
        total += sample_charge_level();
        sleep_ms(4);
    }
    baseline_level = total / INIT_SAMPLES;
    threshold_level = compute_threshold(baseline_level);
    last_sample_level = baseline_level;
}

void touch_update(void) {
    const uint32_t level = sample_charge_level();
    last_sample_level = level;
    const bool below_threshold = level + threshold_level < baseline_level;

    if (!pressed && !below_threshold) {
        // Track slow ambient drift but do not learn a held touch as baseline.
        baseline_level = (baseline_level * 31 + level) / 32;
        threshold_level = compute_threshold(baseline_level);
    }

    if (below_threshold) {
        low_samples++;
        high_samples = 0;
        if (!pressed && low_samples >= PRESS_DEBOUNCE_SAMPLES) {
            pressed = true;
            press_event = true;
        }
    } else {
        high_samples++;
        low_samples = 0;
        if (pressed && high_samples >= RELEASE_DEBOUNCE_SAMPLES) pressed = false;
    }
}

bool touch_take_press(void) {
    const bool event = press_event;
    press_event = false;
    return event;
}

bool touch_is_pressed(void) { return pressed; }
