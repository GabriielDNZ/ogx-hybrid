cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

include(FWDefines.cmake)
set(PICO_PLATFORM rp2350)
set(PICO_BOARD pico2_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)

# ── Board: always Pico 2W hybrid ──────────────────────────────────
set(FLASH_SIZE_MB 4)

project(OGX_BT_Hybrid C CXX ASM)

# ── Use bluepad32/btstack as the single btstack for the whole project ──
# This resolves version conflicts between pico-sdk btstack and bluepad32 btstack.
# PICO_BTSTACK_PATH must be set before pico_sdk_init() so pico_btstack_classic
# compiles from bluepad32/external/btstack instead of pico-sdk/lib/btstack.
set(PICO_BTSTACK_PATH ${CMAKE_CURRENT_LIST_DIR}/external/bluepad32/external/btstack
    CACHE PATH "BTstack path" FORCE)

pico_sdk_init()

# btstack_config.h must be visible to pico_btstack_base_headers
target_include_directories(pico_btstack_base_headers SYSTEM INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

# ── Paths ─────────────────────────────────────────────────────────
set(OGX_SRC     ${CMAKE_CURRENT_LIST_DIR}/ogxmini_src)
set(EXTERNAL    ${CMAKE_CURRENT_LIST_DIR}/external)
set(HYBRID_SRC  ${CMAKE_CURRENT_LIST_DIR}/src)
set(BLUEPAD32_ROOT ${EXTERNAL}/bluepad32)
set(BTSTACK_ROOT   ${BLUEPAD32_ROOT}/external/btstack)

# ── cmake helpers from OGX-Mini ──────────────────────────────────
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/external)

# ── Third-party audio libs (LDAC, FDK-AAC) ───────────────────────
add_subdirectory(3rd-party/ldacBT)
set(FDK_DIR ${CMAKE_CURRENT_LIST_DIR}/3rd-party/fdk-aac)
set(FDK_BUILD_DIR ${CMAKE_BINARY_DIR}/fdk-aac)
add_subdirectory(${FDK_DIR} ${FDK_BUILD_DIR})
target_compile_definitions(fdk-aac PUBLIC FDK_ARM_ARCH)

# ── libfixmath ───────────────────────────────────────────────────
add_subdirectory(${EXTERNAL}/libfixmath libfixmath)

# ── bluepad32 ────────────────────────────────────────────────────
add_subdirectory(${BLUEPAD32_ROOT}/src/components/bluepad32 libbluepad32)

# ── GATT header (needed by BLEServer) ────────────────────────────
set(GATT_FILE att_delayed_response.gatt)
set(GATT_OUT  ${OGX_SRC}/BLEServer/att_delayed_response.h)
if(NOT EXISTS ${GATT_OUT})
    execute_process(
        COMMAND python3 ${BTSTACK_ROOT}/tool/compile_gatt.py
                ${OGX_SRC}/BLEServer/${GATT_FILE} ${GATT_OUT}
    )
endif()

# ── Source files ──────────────────────────────────────────────────
set(SOURCES
    # ── main híbrido (C) ──
    ${HYBRID_SRC}/main_hybrid.c
    ${HYBRID_SRC}/pico_w_led.c

    # ── BT Audio ──
    ${HYBRID_SRC}/btstack/btstack_avdtp_source.c
    ${HYBRID_SRC}/btstack/btstack_hci.c

    # ── TinyUSB Audio (UAC) ──
    ${HYBRID_SRC}/tinyusb/uac.c
    # usb_descriptors.c from audio: merged into OGX-Mini tud_callbacks.cpp

    # ── OGX-Mini core ──
    ${OGX_SRC}/hybrid_wrappers.cpp
    ${OGX_SRC}/OGXMini/OGXMini.cpp
    ${OGX_SRC}/OGXMini/Board/Standard.cpp
    ${OGX_SRC}/OGXMini/Board/PicoW.cpp
    ${OGX_SRC}/OGXMini/Board/PicoW_Hybrid.cpp
    ${OGX_SRC}/OGXMini/Board/Four_Channel_I2C.cpp
    ${OGX_SRC}/OGXMini/Board/ESP32_Bluepad32_I2C.cpp
    ${OGX_SRC}/OGXMini/Board/ESP32_Blueretro_I2C.cpp
    ${OGX_SRC}/TaskQueue/TaskQueue.cpp
    ${OGX_SRC}/Board/ogxm_log.cpp
    ${OGX_SRC}/Board/esp32_api.cpp
    ${OGX_SRC}/Board/board_api.cpp
    ${OGX_SRC}/Board/board_api_private/board_api_led.cpp
    ${OGX_SRC}/Board/board_api_private/board_api_rgb.cpp
    ${OGX_SRC}/Board/board_api_private/board_api_bt.cpp
    ${OGX_SRC}/Board/board_api_private/board_api_usbh.cpp
    ${OGX_SRC}/UserSettings/UserSettings.cpp
    ${OGX_SRC}/UserSettings/UserProfile.cpp
    ${OGX_SRC}/UserSettings/JoystickSettings.cpp
    ${OGX_SRC}/UserSettings/TriggerSettings.cpp
    ${OGX_SRC}/USBDevice/HybridDescriptors.cpp
    ${OGX_SRC}/USBDevice/DeviceManager.cpp
    ${OGX_SRC}/USBDevice/DeviceDriver/DeviceDriver.cpp
    ${OGX_SRC}/USBDevice/DeviceDriver/PSClassic/PSClassic.cpp
    ${OGX_SRC}/USBDevice/DeviceDriver/PS3/PS3.cpp
    ${OGX_SRC}/USBDevice/DeviceDriver/Switch/Switch.cpp
    ${OGX_SRC}/USBDevice/DeviceDriver/XInput/XInput.cpp
    ${OGX_SRC}/USBDevice/DeviceDriver/XboxOG/XboxOG_GP.cpp
    ${OGX_SRC}/USBDevice/DeviceDriver/XboxOG/XboxOG_SB.cpp
    ${OGX_SRC}/USBDevice/DeviceDriver/XboxOG/XboxOG_XR.cpp
    ${OGX_SRC}/USBDevice/DeviceDriver/DInput/DInput.cpp
    ${OGX_SRC}/USBDevice/DeviceDriver/WebApp/WebApp.cpp
    ${OGX_SRC}/USBDevice/DeviceDriver/XInput/tud_xinput/tud_xinput.cpp
    ${OGX_SRC}/USBDevice/DeviceDriver/XboxOG/tud_xid/tud_xid.cpp
    ${OGX_SRC}/Bluepad32/Bluepad32.cpp
    ${OGX_SRC}/BLEServer/BLEServer.cpp
)

add_executable(OGX_BT_Hybrid ${SOURCES})

# ── Fix: missing hci_event_builder.c ─────────────────────────────
target_sources(OGX_BT_Hybrid PRIVATE
    ${BTSTACK_ROOT}/src/hci_event_builder.c
)

# ── Compile definitions ───────────────────────────────────────────
target_compile_definitions(OGX_BT_Hybrid PRIVATE
    # TinyUSB MCU target (RP2350 for Pico 2W)
    CFG_TUSB_MCU=OPT_MCU_RP2040
    # Force our hybrid tusb_config — avoids ogxmini_src/tusb_config.h being picked up first
    CFG_TUSB_CONFIG_FILE=\"hybrid_config/tusb_config_hybrid.h\"

    # OGX-Mini board
    CONFIG_OGXM_BOARD_PI_PICO2W=1
    OGXM_BOARD=PI_PICO2W
    CONFIG_EN_BLUETOOTH=1
    MAX_GAMEPADS=1
    PICO_FLASH_SIZE_BYTES=4*1024*1024
    NVS_SECTORS=4
    BUILD_DATETIME="hybrid"
    FIRMWARE_NAME="OGX-BT-Hybrid"
    FIRMWARE_VERSION="1.0"

    # Hybrid key: skip l2cap_init + hci_power in bluepad32 setup
    HYBRID_BT_ALREADY_INITIALIZED=1

    # Audio project defs
    CYW43_LWIP=0
    PICO_FLASH_ASSUME_CORE1_SAFE=1
)

# ── Include dirs ──────────────────────────────────────────────────
target_include_directories(OGX_BT_Hybrid PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}                   # tusb_config.h (audio) + btstack_config.h — MUST be before ogxmini_src
    ${HYBRID_SRC}
    ${OGX_SRC}
    ${BLUEPAD32_ROOT}/src/components/bluepad32/include
    ${BTSTACK_ROOT}/src
    ${BTSTACK_ROOT}/3rd-party/bluedroid/encoder/include
    ${BTSTACK_ROOT}/3rd-party/bluedroid/decoder/include
    ${BTSTACK_ROOT}/platform/embedded
    3rd-party/ldacBT/inc
    3rd-party/ldacBT/libldac/inc
    3rd-party/fdk-aac/libAACenc/include
    3rd-party/fdk-aac/libSYS/include
    ${PICO_SDK_PATH}/lib/tinyusb/hw          # bsp/board_api.h
    ${PICO_SDK_PATH}/lib/tinyusb/src         # tusb.h
    ${CMAKE_CURRENT_LIST_DIR}/hybrid_config     # tusb_config_hybrid.h
)

# TinyUSB device provided by pico-sdk tinyusb_device

# ── Link libraries ────────────────────────────────────────────────
target_link_libraries(OGX_BT_Hybrid
    pico_stdlib
    pico_multicore
    pico_rand
    hardware_timer
    hardware_clocks
    hardware_flash
    hardware_uart
    hardware_irq
    hardware_pio
    pico_cyw43_arch_none
    pico_btstack_classic
    pico_btstack_cyw43
    pico_btstack_sbc_encoder
    tinyusb_device
    tinyusb_board
    bluepad32
    libfixmath
    fdk-aac
    ldacBT_enc
    ldacBT_abr
)

pico_add_extra_outputs(OGX_BT_Hybrid)
