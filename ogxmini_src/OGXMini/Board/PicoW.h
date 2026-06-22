#pragma once

#include <cstdint>

namespace pico_w {
    void initialize();
    void run();
    // Hybrid split functions
    void initialize_usb();
    void initialize_bluetooth(); // call from Core1, before btstack_run_loop_execute
    void run_usb();              // call from Core0
} // namespace pico_w