/**
 * HYBRID FIRMWARE - Pico 2W
 *
 * Core0: TinyUSB audio device (PS5 → buffer) + OGX-Mini USB XInput (Xbox 360)
 * Core1: BTstack run loop:
 *          A2DP Source  → áudio buffer → JBL Tune Flex 2
 *          Bluepad32    → controle BT  → XInput para Xbox 360
 */

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/cyw43_arch.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include "btstack_run_loop.h"

/* BT Audio */
#include "btstack/btstack_avdtp_source.h"
#include "btstack/btstack_hci.h"

/* USB Audio (UAC device) */
#include "tinyusb/uac.h"
#include "pico_w_led.h"

/* OGX-Mini (C++ side — called via extern "C" wrappers) */
#ifdef __cplusplus
extern "C" {
#endif
void hybrid_ogx_initialize_usb(void);
void hybrid_ogx_initialize_bluetooth(void);
void hybrid_ogx_run_usb(void);
#ifdef __cplusplus
}
#endif

/* ── Core1: BTstack run loop ─────────────────────────────────────── */
static void core1_bt_entry(void) {
    /* 1. Audio BT: l2cap_init + A2DP/AVRCP/SDP + hci_power_control(ON) */
    btstack_main(0, NULL);
    sleep_ms(200);

    /* 2. Bluepad32: registra HID host no mesmo btstack.
          uni_bt_setup() pula l2cap_init + hci_power graças ao HYBRID_BT_ALREADY_INITIALIZED */
    hybrid_ogx_initialize_bluetooth();

    /* 3. Único run loop — processa tudo junto */
    btstack_run_loop_execute();
}

/* ── USB timer (Core0) ───────────────────────────────────────────── */
static repeating_timer_t s_usb_timer;
static bool usb_timer_cb(repeating_timer_t *rt) {
    (void)rt;
    tinyusb_task();
    return true;
}

/* ── main (Core0) ───────────────────────────────────────────────── */
int main(void) {
    vreg_set_voltage(VREG_VOLTAGE_1_20);
    sleep_ms(10);
    set_sys_clock_khz(250000, true);

    stdio_init_all();
    flash_safe_execute_core_init();

    /* Flash slot para áudio (do projeto original) */
    uint8_t current_slot = read_uint8_last_flash();
    if (current_slot != 0x1 && current_slot != 0x2) {
        write_uint8_last_flash(0x1);
    }

    /* CYW43 — uma única vez */
    if (cyw43_arch_init()) {
        panic("cyw43_arch_init() failed");
    }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    /* TinyUSB audio device (PS5) */
    tinyusb_main();
    audio_slot_queue_init();

    /* Lançar Core1 com BTstack */
    multicore_launch_core1(core1_bt_entry);

    /* Timer USB 500µs */
    add_repeating_timer_us(-500, usb_timer_cb, NULL, &s_usb_timer);

    /* OGX-Mini USB XInput (Xbox 360) */
    hybrid_ogx_initialize_usb();

    /* Watchdog */
    watchdog_enable(2000, true);

    /* Loop principal */
    hybrid_ogx_run_usb();

    return 0;
}
