cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# PICO_SDK_PATH deve apontar para um checkout do pico-sdk (feito pelo
# workflow do GitHub Actions via "git clone" antes de chamar cmake).
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(ogxm_hybrid_bootloader C CXX ASM)

set(PICO_BOARD pico2_w CACHE STRING "Board type")
set(PICO_PLATFORM rp2350-arm-s CACHE STRING "Platform")

pico_sdk_init()

# --- Mapa de particoes de flash (compartilhado com os apps) ---
set(TARGET_PARTITION "bootloader")
include(${CMAKE_CURRENT_LIST_DIR}/../shared/flash_layout.cmake)
# A partir daqui, FLASH_OFFSET e FLASH_LENGTH estao definidos
# corretamente para esta particao (ver shared/flash_layout.cmake)

add_executable(ogxm_hybrid_bootloader
    src/main.c
)

target_include_directories(ogxm_hybrid_bootloader PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/../shared
)

# O bootloader e propositalmente minimo: SEM cyw43/btstack/tinyusb.
# Isso garante que o blob de firmware Wi-Fi/Bluetooth (~300-400KB) NAO
# seja linkado aqui, mantendo o bootloader bem dentro dos 128KB
# reservados (BOOTLOADER_LENGTH_HEX em shared/flash_layout.cmake).
target_link_libraries(ogxm_hybrid_bootloader
    pico_stdlib
    pico_bootrom
    pico_flash
    hardware_flash
    hardware_sync
    hardware_watchdog
)

target_compile_definitions(ogxm_hybrid_bootloader PRIVATE
    PICO_FLASH_SIZE_BYTES=${TOTAL_FLASH_SIZE_BYTES}
)

# --- Linker script fixo para o bootloader (offset = 0, 128KB) ---
set(BOOTLOADER_LD_OUT ${CMAKE_CURRENT_LIST_DIR}/cmake/memmap_bootloader.ld)
pico_set_linker_script(ogxm_hybrid_bootloader ${BOOTLOADER_LD_OUT})

pico_add_extra_outputs(ogxm_hybrid_bootloader)

# Garante que o binario do bootloader nao excede o espaco reservado
# (checagem adicional em build time, alem do ASSERT dentro do .ld)
add_custom_command(TARGET ogxm_hybrid_bootloader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Bootloader compilado. Verifique o tamanho abaixo:"
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:ogxm_hybrid_bootloader>
)
