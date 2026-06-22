/*
 * tusb_config.h — HYBRID (OGX-Mini XInput + UAC Audio)
 * Combines OGX-Mini (HID XInput device) with BT Audio (UAC device) configs
 */

#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "Board/Config.h"
#include "src/tinyusb/usb_descriptors.h"

//--------------------------------------------------------------------+
// Board / Port Configuration
//--------------------------------------------------------------------+
#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT      0
#endif
#ifndef BOARD_TUD_MAX_SPEED
#define BOARD_TUD_MAX_SPEED   OPT_MODE_DEFAULT_SPEED
#endif

#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined
#endif
#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS           OPT_OS_NONE
#endif
#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG        0
#endif

#define CFG_TUD_ENABLED       1
#define CFG_TUD_MAX_SPEED     BOARD_TUD_MAX_SPEED
#define CFG_TUD_TASK_QUEUE_SZ 64

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif
#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN    __attribute__ ((aligned(4)))
#endif

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE  64
#endif

//--------------------------------------------------------------------+
// Device class config
//--------------------------------------------------------------------+

// ── OGX-Mini: HID/XInput device (USB to Xbox/PS etc) ──
#define CFG_TUD_CDC              0
#define CFG_TUD_MSC              0
#define CFG_TUD_MIDI             0
#define CFG_TUD_VENDOR           0

// ── UAC Audio (for PS5 audio input) ──
#define CFG_TUD_AUDIO            1

// HID endpoints for OGX XInput
#if defined(CONFIG_EN_USB_HOST)
  // USB host mode: no HID device
  #define CFG_TUD_HID            0
#else
  // BT mode (Pico W/2W): HID device for XInput output
  #define CFG_TUD_HID            2
#endif

//--------------------------------------------------------------------+
// UAC Audio function config (from bt-audio project)
//--------------------------------------------------------------------+
#define CFG_TUD_AUDIO_ENABLE_INTERRUPT_EP                            1
#define CFG_TUD_AUDIO_FUNC_1_DESC_LEN                                TUD_AUDIO_HEADSET_STEREO_DESC_LEN
#define CFG_TUD_AUDIO_FUNC_1_N_FORMATS                               2
#define CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE                         48000
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX                           0
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX                           2
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_RX          2
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_RX                  16
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_2_N_BYTES_PER_SAMPLE_RX          2
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_2_RESOLUTION_RX                  16
#define CFG_TUD_AUDIO_ENABLE_EP_OUT                                   1
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_EP_SZ_OUT   TUD_AUDIO_EP_SIZE(CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE, CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_RX, CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX)
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ     (CFG_TUD_AUDIO_FUNC_1_FORMAT_1_EP_SZ_OUT)*2
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX        (CFG_TUD_AUDIO_FUNC_1_FORMAT_1_EP_SZ_OUT)
#define CFG_TUD_AUDIO_FUNC_1_N_AS_INT              1
#define CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ           64

//--------------------------------------------------------------------+
// USB Host (only for non-BT boards)
//--------------------------------------------------------------------+
#if defined(CONFIG_EN_USB_HOST)
#define CFG_TUH_ENABLED        1
#define CFG_TUH_MAX_SPEED      BOARD_TUH_MAX_SPEED
#define CFG_TUH_RPI_PIO_USB    1
#define TUH_OPT_RHPORT         1
#define CFG_TUH_ENUMERATION_BUFSIZE 512
#define CFG_TUH_HUB            1
#define CFG_TUH_CDC            0
#define CFG_TUH_HID            MAX_GAMEPADS
#define CFG_TUH_MSC            0
#define CFG_TUH_VENDOR         0
#define CFG_TUH_XINPUT         MAX_GAMEPADS
#define CFG_TUH_DEVICE_MAX     (CFG_TUH_HUB ? 4 : 1)
#define CFG_TUH_HID_EPIN_BUFSIZE  64
#define CFG_TUH_HID_EPOUT_BUFSIZE 64
#endif

#ifdef __cplusplus
}
#endif

#endif // _TUSB_CONFIG_H_
