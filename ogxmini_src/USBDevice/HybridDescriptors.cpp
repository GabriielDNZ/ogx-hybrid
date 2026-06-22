/**
 * HybridDescriptors.cpp
 * TinyUSB descriptor callbacks for the hybrid firmware.
 * Overrides tud_callbacks.cpp (which delegates to DeviceManager).
 */
#include <cstring>
#include "HybridDescriptors.h"
#include "USBDevice/DeviceManager.h"

// ── Device descriptor ──────────────────────────────────────────────
extern "C" uint8_t const* tud_descriptor_device_cb(void) {
    return reinterpret_cast<const uint8_t*>(&HYBRID_DEVICE_DESCRIPTOR);
}

// ── Configuration descriptor ───────────────────────────────────────
extern "C" uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return HYBRID_CONFIGURATION_DESCRIPTOR;
}

// ── String descriptors ────────────────────────────────────────────
static uint16_t desc_str_buf[32 + 1];

extern "C" uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    size_t chr_count;

    if (index == 0) {
        memcpy(&desc_str_buf[1], HYBRID_STRING_DESCRIPTORS[0], 2);
        chr_count = 1;
    } else {
        size_t arr_len = sizeof(HYBRID_STRING_DESCRIPTORS) / sizeof(HYBRID_STRING_DESCRIPTORS[0]);
        if (index >= arr_len || HYBRID_STRING_DESCRIPTORS[index] == nullptr)
            return nullptr;
        const char* str = HYBRID_STRING_DESCRIPTORS[index];
        chr_count = strlen(str);
        if (chr_count > 32) chr_count = 32;
        for (size_t i = 0; i < chr_count; i++)
            desc_str_buf[1 + i] = str[i];
    }

    desc_str_buf[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return desc_str_buf;
}

// ── HID / XID callbacks — delegate to DeviceManager ──────────────
extern "C" uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
    hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    return DeviceManager::get_instance().get_driver()
        ->get_report_cb(itf, report_id, report_type, buffer, reqlen);
}

extern "C" void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
    hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    DeviceManager::get_instance().get_driver()
        ->set_report_cb(itf, report_id, report_type, buffer, bufsize);
}

extern "C" bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
    tusb_control_request_t const* request) {
    return DeviceManager::get_instance().get_driver()
        ->vendor_control_xfer_cb(rhport, stage, request);
}

extern "C" uint8_t const* tud_hid_descriptor_report_cb(uint8_t itf) {
    return DeviceManager::get_instance().get_driver()
        ->get_hid_descriptor_report_cb(itf);
}

extern "C" uint8_t const* tud_descriptor_device_qualifier_cb() {
    return nullptr;
}

// ── Custom class driver (XID) ─────────────────────────────────────
extern "C" const usbd_class_driver_t* usbd_app_driver_get_cb(uint8_t* driver_count) {
    *driver_count = 1;
    return DeviceManager::get_instance().get_driver()->get_class_driver();
}
