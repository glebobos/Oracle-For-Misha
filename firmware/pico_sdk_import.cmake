# Minimal Pico SDK import shim. The Docker image supplies PICO_SDK_PATH.
if (NOT PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif()

if (NOT PICO_SDK_PATH)
    message(FATAL_ERROR "PICO_SDK_PATH is required. Use the provided Docker build.")
endif()

get_filename_component(PICO_SDK_PATH "${PICO_SDK_PATH}" REALPATH BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
include("${PICO_SDK_PATH}/pico_sdk_init.cmake")
