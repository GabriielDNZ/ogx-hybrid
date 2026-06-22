/**
 * hybrid_wrappers.cpp
 * Expõe as funções C++ do OGX-Mini como extern "C" para o main_hybrid.c
 */
#include "OGXMini/Board/PicoW.h"

extern "C" {

void hybrid_ogx_initialize_usb(void) {
    pico_w::initialize_usb();
}

void hybrid_ogx_initialize_bluetooth(void) {
    pico_w::initialize_bluetooth();
}

void hybrid_ogx_run_usb(void) {
    pico_w::run_usb();
}

} // extern "C"
