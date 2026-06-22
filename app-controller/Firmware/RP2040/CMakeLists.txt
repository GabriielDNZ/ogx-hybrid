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