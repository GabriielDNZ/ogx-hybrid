/**
 * PicoW_Hybrid.cpp
 * 
 * Split version of PicoW.cpp for hybrid firmware.
 * The run loop structure is managed by main_hybrid.c:
 *   Core0: USB device (XInput for Xbox 360) 
 *   Core1: BTstack (A2DP audio + bluepad32 gamepad receive)
 */

#include "Board/Config.h"
#include "OGXMini/Board/PicoW.h"

#if (OGXM_BOARD == PI_PICOW) || (OGXM_BOARD == PI_PICO2W)

#include <hardware/clocks.h>
#include <pico/multicore.h>
#include <pico/cyw43_arch.h>
#include "hardware/watchdog.h"

#include "tusb.h"
#include "bsp/board_api.h"

#include "USBDevice/DeviceManager.h"
#include "UserSettings/UserSettings.h"
#include "Board/board_api.h"
#include "Bluepad32/Bluepad32.h"
#include "BLEServer/BLEServer.h"
#include "Gamepad/Gamepad.h"
#include "TaskQueue/TaskQueue.h"

// Shared gamepad state between cores
Gamepad _hybrid_gamepads[MAX_GAMEPADS];

// ── Called from Core1 (btstack run loop context) ──────────────────
void pico_w::initialize_bluetooth() {
    // board_api::init_bluetooth() calls cyw43_arch_init() — already done in main
    board_api::set_led(true);
    BLEServer::init_server(_hybrid_gamepads);
    // setup_only registers bluepad32 HID without starting run loop
    bluepad32::setup_only(_hybrid_gamepads);
}

// ── Called from Core0 ─────────────────────────────────────────────
void pico_w::initialize_usb() {
    board_api::init_board(); // init board WITHOUT cyw43 (already done)

    UserSettings& user_settings = UserSettings::get_instance();
    user_settings.initialize_flash();

    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i) {
        _hybrid_gamepads[i].set_profile(user_settings.get_profile_by_index(i));
    }

    DeviceManager& device_manager = DeviceManager::get_instance();
    device_manager.initialize_driver(user_settings.get_current_driver(), _hybrid_gamepads);

    tud_init(BOARD_TUD_RHPORT);
}

static void set_gp_check_timer_hybrid(uint32_t task_id) {
    UserSettings& user_settings = UserSettings::get_instance();
    TaskQueue::Core0::queue_delayed_task(task_id, UserSettings::GP_CHECK_DELAY_MS, true,
    [&user_settings] {
        if (user_settings.check_for_driver_change(_hybrid_gamepads[0])) {
            user_settings.store_driver_type(user_settings.get_current_driver());
        }
    });
}

void pico_w::run_usb() {
    uint32_t tid_gp_check = TaskQueue::Core0::get_new_task_id();
    set_gp_check_timer_hybrid(tid_gp_check);

    DeviceDriver* device_driver = DeviceManager::get_instance().get_driver();

    while (true) {
        watchdog_update();
        TaskQueue::Core0::process_tasks();

        for (uint8_t i = 0; i < MAX_GAMEPADS; ++i) {
            device_driver->process(i, _hybrid_gamepads[i]);
            tud_task();
        }
        sleep_ms(1);
    }
}


#endif
