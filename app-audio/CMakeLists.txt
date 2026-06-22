# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_BOARD pico2_w CACHE STRING "Board type")
set(FAMILY rp2040)
set(BOARD pico_sdk)



# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)


project(PicoW_USB_BT_Audio C CXX ASM)


# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# ---------------------------------------------------------------------
# HIBRIDO: mapa de particoes de flash compartilhado (bootloader / app
# controle / app audio). Ver shared/flash_layout.cmake.
# ---------------------------------------------------------------------
set(HYBRID_SHARED_DIR ${CMAKE_CURRENT_LIST_DIR}/../shared)
set(TARGET_PARTITION "audio")
include(${HYBRID_SHARED_DIR}/flash_layout.cmake)


#LDAC https://github.com/EHfive/ldacBT
add_subdirectory(3rd-party/ldacBT)


# FDK-AAC (encoder only)
set(FDK_DIR ${CMAKE_CURRENT_LIST_DIR}/3rd-party/fdk-aac)
set(FDK_BUILD_DIR ${CMAKE_BINARY_DIR}/fdk-aac)


add_subdirectory(${FDK_DIR} ${FDK_BUILD_DIR})
target_compile_definitions(fdk-aac PUBLIC FDK_ARM_ARCH)


set(BTSTACK_3RD_PARTY_PATH ${PICO_SDK_PATH}/lib/btstack/3rd-party)
#set(BTSTACK_EXAMPLE_PATH $ENV{PICO_SDK_PATH}/lib/btstack/3rd-party)


# Add executable.
add_executable(${PROJECT_NAME}
    src/main.c
    src/pico_w_led.c
    src/btstack/btstack_avdtp_source.c
    src/btstack/btstack_hci.c
    src/tinyusb/uac.c
    src/tinyusb/usb_descriptors.c
)

# HIBRIDO: linker script fixo com offset para particao do audio
set(AUDIO_LD_OUT ${CMAKE_CURRENT_LIST_DIR}/memmap_audio.ld)
pico_set_linker_script(${PROJECT_NAME} ${AUDIO_LD_OUT})


# Including header files directly from project directory
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${HYBRID_SHARED_DIR}
)


target_include_directories(${PROJECT_NAME} PRIVATE
        3rd-party/ldacBT/libldac/inc
        3rd-party/ldacBT/libldac/abr/inc
        3rd-party/fdk-aac
)


# Link the Project to extra libraries
target_link_libraries(PicoW_USB_BT_Audio
pico_stdlib
pico_btstack_ble
pico_btstack_classic
pico_btstack_cyw43
pico_cyw43_arch_threadsafe_background
pico_btstack_sbc_encoder
pico_multicore
pico_flash
tinyusb_device
tinyusb_board
fdk-aac
ldacBT_enc
ldacBT_abr
)

target_compile_definitions(PicoW_USB_BT_Audio PRIVATE
    CYW43_LWIP=0
    PICO_FLASH_ASSUME_CORE1_SAFE=1
    HYBRID_APP_FLASH_OFFSET=${FLASH_OFFSET}
    HYBRID_APP_FLASH_LENGTH=${FLASH_LENGTH}

    # must enable on rp2040 & 330kbps(MQ)
    #LDAC_SOFT_FLOAT=OFF
)



# Export binaries like hex, bin, and uf2 files.
pico_add_extra_outputs(${PROJECT_NAME})
