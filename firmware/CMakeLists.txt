cmake_minimum_required(VERSION 3.20)

include(pico_sdk_import.cmake)

project(misha_gadalka C CXX ASM)
pico_sdk_init()

add_executable(misha_gadalka
    src/main.c
    src/display.c
    src/font.c
    src/touch.c
    src/fortunes.c
)

target_include_directories(misha_gadalka PRIVATE src)
target_link_libraries(misha_gadalka
    pico_stdlib
    hardware_adc
    hardware_i2c
    hardware_timer
    pico_rand
)

pico_enable_stdio_usb(misha_gadalka 1)
pico_enable_stdio_uart(misha_gadalka 0)
pico_add_extra_outputs(misha_gadalka)
