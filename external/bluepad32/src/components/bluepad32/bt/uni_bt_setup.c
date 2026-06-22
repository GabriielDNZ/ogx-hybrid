// SPDX-License-Identifier: Apache-2.0
// HYBRID PATCH: l2cap_init and hci_power_control are skipped when
// HYBRID_BT_ALREADY_INITIALIZED is defined (done by btstack_main audio init)

#include "bt/uni_bt_setup.h"
#include <btstack.h>
#include "sdkconfig.h"
#include "bt/uni_bt.h"
#include "bt/uni_bt_bredr.h"
#include "bt/uni_bt_defines.h"
#include "bt/uni_bt_hci_cmd.h"
#include "bt/uni_bt_le.h"
#include "bt/uni_bt_service.h"
#include "platform/uni_platform.h"
#include "uni_common.h"
#include "uni_config.h"
#include "uni_log.h"

typedef enum {
    SETUP_STATE_BTSTACK_IN_PROGRESS,
    SETUP_STATE_BLUEPAD32_IN_PROGRESS,
    SETUP_STATE_READY,
} setup_state_t;

typedef uint8_t (*fn_t)(void);

static void setup_call_next_fn(void);
static uint8_t setup_set_event_filter(void);
static uint8_t setup_write_simple_pairing_mode(void);

static int setup_fn_idx = 0;
static fn_t setup_fns[] = {
    &setup_write_simple_pairing_mode,
    &setup_set_event_filter,
};
static setup_state_t setup_state = SETUP_STATE_BTSTACK_IN_PROGRESS;
static btstack_packet_callback_registration_t uni_hci_event_callback_registration;

static uint8_t setup_set_event_filter(void) {
    return hci_send_cmd(&hci_set_event_filter_inquiry_cod, 0x01, 0x01, UNI_BT_COD_MAJOR_PERIPHERAL,
                        UNI_BT_COD_MAJOR_MASK);
}
static uint8_t setup_write_simple_pairing_mode(void) {
    return hci_send_cmd(&hci_write_simple_pairing_mode, true);
}
static void setup_call_next_fn(void) {
    uint8_t status;
    if (!hci_can_send_command_packet_now()) {
        logi("HCI not ready, will retry. idx=%d\n", setup_fn_idx);
        return;
    }
    fn_t fn = setup_fns[setup_fn_idx];
    status = fn();
    if (status) {
        loge("Failed idx=%d, status=0x%02x, retrying\n", setup_fn_idx, status);
        return;
    }
    setup_fn_idx++;
    if (setup_fn_idx == ARRAY_SIZE(setup_fns)) {
        setup_state = SETUP_STATE_READY;
        gap_local_bd_addr(uni_local_bd_addr);
        uni_get_platform()->on_init_complete();
        if (IS_ENABLED(UNI_ENABLE_BLE) && uni_bt_service_is_enabled())
            uni_bt_service_init();
    }
}

void uni_bt_setup_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    uint8_t event;
    ARG_UNUSED(channel); ARG_UNUSED(packet); ARG_UNUSED(size);
    if (packet_type != HCI_EVENT_PACKET) return;
    if (setup_state == SETUP_STATE_BLUEPAD32_IN_PROGRESS) {
        setup_call_next_fn();
        return;
    }
    event = hci_event_packet_get_type(packet);
    switch (event) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                setup_state = SETUP_STATE_BLUEPAD32_IN_PROGRESS;
                setup_call_next_fn();
            }
            break;
        case BTSTACK_EVENT_POWERON_FAILED:
            loge("Failed to initialize HCI.\n");
            break;
        default:
            break;
    }
}

bool uni_bt_setup_is_ready() {
    return setup_state == SETUP_STATE_READY;
}

int uni_bt_setup(void) {
    bool bredr_enabled = false;
    bool ble_enabled = false;

#ifndef HYBRID_BT_ALREADY_INITIALIZED
    // Normal mode: bluepad32 owns full btstack init
    l2cap_init();
#endif
    // HYBRID mode: l2cap_init already called by btstack_main (audio)

    if (IS_ENABLED(UNI_ENABLE_BREDR))
        bredr_enabled = uni_bt_bredr_is_enabled();
    if (IS_ENABLED(UNI_ENABLE_BLE))
        ble_enabled = uni_bt_le_is_enabled();

    logi("Max connected gamepads: %d\n", CONFIG_BLUEPAD32_MAX_DEVICES);
    logi("BR/EDR: %s, BLE: %s\n",
         bredr_enabled ? "enabled" : "disabled",
         ble_enabled   ? "enabled" : "disabled");

    uni_hci_event_callback_registration.callback = &uni_bt_packet_handler;
    hci_add_event_handler(&uni_hci_event_callback_registration);

    if (IS_ENABLED(UNI_ENABLE_BREDR) && bredr_enabled)
        uni_bt_bredr_setup();
    if (IS_ENABLED(UNI_ENABLE_BLE) && ble_enabled)
        uni_bt_le_setup();

#ifndef HYBRID_BT_ALREADY_INITIALIZED
    // Normal mode: bluepad32 turns on HCI
    int err = hci_power_control(HCI_POWER_ON);
    if (err != 0) {
        loge("Failed to power on HCI, err=%x\n", err);
        return UNI_ERROR_INIT_FAILED;
    }
#endif
    // HYBRID mode: hci_power_control already called by btstack_main (audio)

    return UNI_ERROR_SUCCESS;
}
