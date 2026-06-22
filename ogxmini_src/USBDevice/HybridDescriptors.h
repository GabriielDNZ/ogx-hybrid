#pragma once
/**
 * HybridDescriptors.h
 * Combined USB descriptor for OGX_BT_Hybrid firmware:
 *   Interface 0  : XboxOG XID Gamepad  (for Xbox 360)
 *   Interface 1  : UAC2 Audio Control  (for PS5 audio)
 *   Interface 2  : UAC2 Audio Streaming (for PS5 audio)
 *
 * Xbox 360 only sees Interface 0 (XID class 0x58).
 * PS5 only sees Interfaces 1+2 (Audio class via IAD).
 */
#include "tusb.h"

// ── Interface numbers ──────────────────────────────────────────────
enum HybridInterface {
    ITF_XID        = 0,   // XboxOG gamepad
    ITF_AUDIO_CTRL = 1,   // UAC2 audio control
    ITF_AUDIO_STR  = 2,   // UAC2 audio streaming
    ITF_TOTAL      = 3
};

// ── Endpoint numbers ───────────────────────────────────────────────
// XID uses EP1
#define EPNUM_XID_OUT    0x01
#define EPNUM_XID_IN     0x81
// Audio uses EP2 (ISO) + EP3 (INT)
#define EPNUM_AUDIO_OUT  0x02
#define EPNUM_AUDIO_INT  0x83

// ── XID descriptor (XboxOG Duke) ──────────────────────────────────
#define TUD_XID_DUKE_LEN (9 + 7 + 7)
#define TUD_XID_DUKE_DESCRIPTOR(_itfnum, _epout, _epin) \
    9, TUSB_DESC_INTERFACE, _itfnum, 0, 2, 0x58, 0x42, 0x00, 0x00, \
    7, TUSB_DESC_ENDPOINT, _epin,  TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(32), 4, \
    7, TUSB_DESC_ENDPOINT, _epout, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(32), 4

// ── Audio descriptor length ────────────────────────────────────────
// Reuse the macro from usb_descriptors.h (already included via tusb_config)
#ifndef TUD_AUDIO_HEADSET_STEREO_DESC_LEN
#include "tinyusb/usb_descriptors.h"
#endif

// ── Total config length ────────────────────────────────────────────
#define HYBRID_CONFIG_TOTAL_LEN \
    (TUD_CONFIG_DESC_LEN + TUD_XID_DUKE_LEN + TUD_AUDIO_HEADSET_STEREO_DESC_LEN)

// ── UAC2 entity IDs (from usb_descriptors.h) ──────────────────────
#ifndef UAC2_ENTITY_CLOCK
#define UAC2_ENTITY_CLOCK               0x04
#define UAC2_ENTITY_SPK_INPUT_TERMINAL  0x01
#define UAC2_ENTITY_SPK_FEATURE_UNIT    0x02
#define UAC2_ENTITY_SPK_OUTPUT_TERMINAL 0x03
#endif

// ── Combined configuration descriptor ─────────────────────────────
static const uint8_t HYBRID_CONFIGURATION_DESCRIPTOR[] = {
    // Config header
    TUD_CONFIG_DESCRIPTOR(1, ITF_TOTAL, 0, HYBRID_CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 500),

    // Interface 0: XboxOG XID Duke gamepad
    TUD_XID_DUKE_DESCRIPTOR(ITF_XID, EPNUM_XID_OUT, EPNUM_XID_IN),

    // Interfaces 1+2: UAC2 Audio headset stereo (speaker/headphones)
    TUD_AUDIO_HEADSET_STEREO_DESCRIPTOR(2,
        EPNUM_AUDIO_OUT,
        0x00,              // EP IN unused (TX=0 channels)
        EPNUM_AUDIO_INT)
};

// ── Device descriptor for hybrid (uses MISC class for IAD) ─────────
static const tusb_desc_device_t HYBRID_DEVICE_DESCRIPTOR = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    // MISC class needed for IAD (audio)
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x045E,   // Microsoft (Xbox compatible)
    .idProduct          = 0x028E,   // XInput compatible PID
    .bcdDevice          = 0x0114,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

static const char* HYBRID_STRING_DESCRIPTORS[] = {
    (const char[]) { 0x09, 0x04 },  // 0: English
    "Microsoft",                     // 1: Manufacturer
    "OGX BT Hybrid",                 // 2: Product
    NULL,                            // 3: Serial (auto)
    "Audio",                         // 4: Audio Interface
};
