cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(ogxm_hybrid_bootloader C CXX ASM)

set(PICO_BOARD pico2_w CACHE STRING "Board type")
set(PICO_PLATFORM rp2350-arm-s CACHE STRING "Platform")

pico_sdk_init()

add_executable(ogxm_hybrid_bootloader
    src/main.c
)

target_include_directories(ogxm_hybrid_bootloader PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/../shared
)

target_link_libraries(ogxm_hybrid_bootloader
    pico_stdlib
    pico_bootrom
    pico_flash
    hardware_flash
    hardware_sync
    hardware_watchdog
)

target_compile_definitions(ogxm_hybrid_bootloader PRIVATE
    PICO_FLASH_SIZE_BYTES=4194304
)

pico_add_extra_outputs(ogxm_hybrid_bootloader)
