cmake_minimum_required(VERSION 3.13)

include(${CMAKE_CURRENT_LIST_DIR}/../FWDefines.cmake)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SRC ${CMAKE_CURRENT_LIST_DIR}/src)
set(EXTERNAL_DIR ${CMAKE_CURRENT_LIST_DIR}/../external)
set(PICOSDK_VERSION_TAG "2.1.0")

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/init_submodules.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/patch_libs.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/generate_gatt_header.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/get_pico_sdk.cmake)

set(PICO_PIO_USB_PATH ${EXTERNAL_DIR}/Pico-PIO-USB)
set(PICO_TINYUSB_PATH ${EXTERNAL_DIR}/tinyusb)
set(BLUEPAD32_ROOT ${EXTERNAL_DIR}/bluepad32)
set(BTSTACK_ROOT ${BLUEPAD32_ROOT}/external/btstack)
set(PICO_BTSTACK_PATH ${BTSTACK_ROOT})
set(LIBFIXMATH_PATH ${EXTERNAL_DIR}/libfixmath)

apply_lib_patches(${EXTERNAL_DIR})

set(SOURCES_BOARD 
    ${SRC}/main.cpp
    ${SRC}/OGXMini/OGXMini.cpp
    ${SRC}/OGXMini/Board/Standard.cpp
    ${SRC}/OGXMini/Board/PicoW.cpp
    ${SRC}/OGXMini/Board/Four_Channel_I2C.cpp
    ${SRC}/OGXMini/Board/ESP32_Bluepad32_I2C.cpp
    ${SRC}/OGXMini/Board/ESP32_Blueretro_I2C.cpp
    ${SRC}/TaskQueue/TaskQueue.cpp
    ${SRC}/Board/ogxm_log.cpp
    ${SRC}/Board/esp32_api.cpp
    ${SRC}/Board/board_api.cpp
    ${SRC}/Board/board_api_private/board_api_led.cpp
    ${SRC}/Board/board_api_private/board_api_rgb.cpp
    ${SRC}/Board/board_api_private/board_api_bt.cpp
    ${SRC}/Board/board_api_private/board_api_usbh.cpp
    ${SRC}/UserSettings/UserSettings.cpp
    ${SRC}/UserSettings/UserProfile.cpp
    ${SRC}/UserSettings/JoystickSettings.cpp
    ${SRC}/UserSettings/TriggerSettings.cpp
    ${SRC}/USBDevice/tud_callbacks.cpp
    ${SRC}/USBDevice/DeviceManager.cpp
    ${SRC}/USBDevice/DeviceDriver/DeviceDriver.cpp
    ${SRC}/USBDevice/DeviceDriver/PSClassic/PSClassic.cpp
    ${SRC}/USBDevice/DeviceDriver/PS3/PS3.cpp
    ${SRC}/USBDevice/DeviceDriver/Switch/Switch.cpp
    ${SRC}/USBDevice/DeviceDriver/XInput/XInput.cpp
    ${SRC}/USBDevice/DeviceDriver/XboxOG/XboxOG_GP.cpp
    ${SRC}/USBDevice/DeviceDriver/XboxOG/XboxOG_SB.cpp
    ${SRC}/USBDevice/DeviceDriver/XboxOG/XboxOG_XR.cpp
    ${SRC}/USBDevice/DeviceDriver/DInput/DInput.cpp
    ${SRC}/USBDevice/DeviceDriver/WebApp/WebApp.cpp
    ${SRC}/USBDevice/DeviceDriver/XInput/tud_xinput/tud_xinput.cpp
    ${SRC}/USBDevice/DeviceDriver/XboxOG/tud_xid/tud_xid.cpp
)

set(LIBS_BOARD
    pico_stdlib
    pico_multicore
    pico_rand
    pico_time
    hardware_timer
    hardware_clocks
    hardware_flash
    hardware_watchdog
    pico_flash
    tinyusb_device
    tinyusb_board
    hardware_uart
    hardware_irq
    libfixmath
)

set(INC_DIRS_BOARD "")
set(MAX_GAMEPADS 1 CACHE STRING "Set number of gamepads, 1 to 4")
set(OGXM_BOARD "PI_PICO2W" CACHE STRING "Fixado em PI_PICO2W")
set(FLASH_SIZE_MB 4)
set(PICO_BOARD none)

# --- (Mantendo toda a sua lógica de Boards original) ---
if (OGXM_BOARD STREQUAL "PI_PICO")
    set(EN_USB_HOST TRUE)
elseif (OGXM_BOARD STREQUAL "PI_PICO2")
    set(EN_USB_HOST TRUE)
    set(PICO_PLATFORM rp2350)
    set(FLASH_SIZE_MB 4)
elseif(OGXM_BOARD STREQUAL "PI_PICOW")
    set(EN_BLUETOOTH TRUE)
    set(EN_USB_HOST TRUE)
    set(PICO_BOARD pico_w)
elseif(OGXM_BOARD STREQUAL "PI_PICO2W")
    set(EN_BLUETOOTH TRUE)
    set(EN_USB_HOST TRUE)
    set(PICO_BOARD pico2_w)
    set(PICO_PLATFORM rp2350)
    set(FLASH_SIZE_MB 4)
    list(APPEND INC_DIRS_BOARD ${PICO_PIO_USB_PATH}/src)
# ... (restante dos elseif mantidos)
endif()

# --- (Mantendo os blocos if(EN_USB_HOST), if(EN_RGB), etc original) ---

if(EN_BLUETOOTH)
    message(STATUS "Bluetooth enabled.")
    generate_gatt_header(${BTSTACK_ROOT} ${SRC}/BLEServer/att_delayed_response.gatt ${SRC}/BLEServer/att_delayed_response.h)
    
    list(APPEND SOURCES_BOARD ${SRC}/Bluepad32/Bluepad32.cpp ${SRC}/BLEServer/BLEServer.cpp)
    list(APPEND INC_DIRS_BOARD ${SRC} ${BTSTACK_ROOT}/3rd-party/bluedroid/encoder/include ${BTSTACK_ROOT}/3rd-party/bluedroid/decoder/include)
    
    # CORREÇÃO: Removidos targets complexos da lista LIBS_BOARD (eles causarão erro se tratados como arquivo)
    list(APPEND LIBS_BOARD
        pico_cyw43_arch_none
        pico_btstack_ble
        pico_btstack_classic
        pico_btstack_cyw43
        bluepad32
    )
endif()

# ... (Mantendo todos os seus add_compile_definitions originais)

include(${PICO_SDK_PATH}/pico_sdk_init.cmake)
project(${FW_NAME} C CXX ASM)
pico_sdk_init()

add_executable(${FW_NAME} ${SOURCES_BOARD})
pico_set_linker_script(${FW_NAME} ${CMAKE_CURRENT_LIST_DIR}/memmap_controller.ld)

target_include_directories(${FW_NAME} PRIVATE ${SRC} ${HYBRID_SHARED_DIR} ${INC_DIRS_BOARD})

# --- LINKAGEM ---
target_link_libraries(${FW_NAME} PRIVATE ${LIBS_BOARD})

if(EN_BLUETOOTH)
    # Vinculação direta dos alvos internos de execução e chipset
    target_link_libraries(${FW_NAME} PRIVATE
        pico_btstack_run_loop_background
        pico_btstack_chipset_cyw43
    )
    add_subdirectory(${BLUEPAD32_ROOT}/src/components/bluepad32 libbluepad32)
    target_include_directories(bluepad32 PRIVATE ${SRC})
endif()

if(OGXM_BOARD STREQUAL "PI_PICO2W")
    target_compile_definitions(tinyusb_board INTERFACE 
        PICO_DEFAULT_PIO_USB_DP_PIN=0
        PICO_DEFAULT_PIO_USB_DM_PIN=1
    )
endif()

# ... (Restante final do seu arquivo mantido)