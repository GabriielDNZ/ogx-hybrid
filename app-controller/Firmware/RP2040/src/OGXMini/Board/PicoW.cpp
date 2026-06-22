#include "Board/Config.h"
#include "OGXMini/Board/PicoW.h"
#if (OGXM_BOARD == PI_PICOW)

#include <hardware/clocks.h>
#include <pico/multicore.h>
#include <pico/flash.h>
#include <pico/time.h>

#include "tusb.h"
#include "bsp/board_api.h"

#include "USBDevice/DeviceManager.h"
#include "UserSettings/UserSettings.h"
#include "Board/board_api.h"
#include "Bluepad32/Bluepad32.h"
#include "BLEServer/BLEServer.h"
#include "Gamepad/Gamepad.h"
#include "TaskQueue/TaskQueue.h"
#include "flash_map.h"
#include "boot_confirm.h"

Gamepad _gamepads[MAX_GAMEPADS];

void core1_task() {
    // HIBRIDO: necessario para que boot_confirm_mode_success(), chamado
    // a partir do Core0, possa usar flash_safe_execute() com seguranca
    // (coordenar com este core antes de apagar/escrever flash). Sem
    // isso, a escrita simplesmente falha silenciosamente (ver nota em
    // boot_confirm.h) -- o app continua funcionando, so nao "lembra"
    // o modo de boot.
    flash_safe_execute_core_init();

    board_api::init_bluetooth();
    board_api::set_led(true);
    BLEServer::init_server(_gamepads);
    bluepad32::run_task(_gamepads);
}

void set_gp_check_timer(uint32_t task_id) {
    UserSettings& user_settings = UserSettings::get_instance();
    TaskQueue::Core0::queue_delayed_task(task_id, UserSettings::GP_CHECK_DELAY_MS, true, 
    [&user_settings] {
        //Check gamepad inputs for button combo to change usb device driver
        if (user_settings.check_for_driver_change(_gamepads[0])) {
            //This will store the new mode and reboot the pico
            user_settings.store_driver_type(user_settings.get_current_driver());
        }
    });
}

void pico_w::initialize() {
    board_api::init_board();

    UserSettings& user_settings = UserSettings::get_instance();
    user_settings.initialize_flash();

    for (uint8_t i = 0; i < MAX_GAMEPADS; ++i) {
        _gamepads[i].set_profile(user_settings.get_profile_by_index(i));
    }

    DeviceManager& device_manager = DeviceManager::get_instance();
    device_manager.initialize_driver(user_settings.get_current_driver(), _gamepads);
}

void pico_w::run() {
    // HIBRIDO: arma o watchdog de confirmacao de boot (~8s). Se este
    // app travar antes de conseguirmos confirmar o modo (CONTROLLER)
    // de alguma forma, o Pico reinicia sozinho e o bootloader tenta
    // de novo / eventualmente alterna pro modo audio (ver
    // bootloader/src/main.c, logica de 3 tentativas).
    boot_confirm_arm_watchdog();

    multicore_reset_core1();
    multicore_launch_core1(core1_task);

    uint32_t tid_gp_check = TaskQueue::Core0::get_new_task_id();
    set_gp_check_timer(tid_gp_check);

    DeviceDriver* device_driver = DeviceManager::get_instance().get_driver();

    tud_init(BOARD_TUD_RHPORT);

    // HIBRIDO: confirmacao "fraca" de modo correto -- se o dispositivo
    // USB ficar montado (enumerado) de forma ESTAVEL por alguns
    // segundos sem desconectar, consideramos isso evidencia razoavel
    // de que o host aceitou o dispositivo (mesmo sem ter mandado
    // rumble ainda, o que e o sinal FORTE, disparado em XInput.cpp).
    // Isso evita ficar "preso" esperando rumble caso o usuario nao
    // tenha nenhum controle pareado via Bluetooth ainda.
    static constexpr uint32_t MOUNT_STABLE_CONFIRM_MS = 6000;
    bool weak_confirm_done = false;
    absolute_time_t mount_since = nil_time;

    while (true) {
        TaskQueue::Core0::process_tasks();
        boot_confirm_feed_watchdog();

        if (!weak_confirm_done) {
            if (tud_mounted()) {
                if (is_nil_time(mount_since)) {
                    mount_since = get_absolute_time();
                } else if (absolute_time_diff_us(mount_since, get_absolute_time()) >= (int64_t)MOUNT_STABLE_CONFIRM_MS * 1000) {
                    boot_confirm_mode_success(MODE_CONTROLLER);
                    weak_confirm_done = true;
                }
            } else {
                mount_since = nil_time; // desconectou, reseta a contagem
            }
        }

        for (uint8_t i = 0; i < MAX_GAMEPADS; ++i) {
            device_driver->process(i, _gamepads[i]);
            tud_task();
        }
        sleep_ms(1);
    }
}

// #else // (OGXM_BOARD == PI_PICOW)

// void pico_w::initialize() {}
// void pico_w::run() {}

#endif // (OGXM_BOARD == PI_PICOW)
