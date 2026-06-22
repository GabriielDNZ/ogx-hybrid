/*
 * HYBRID tusb_config.h
 * Merges OGX-Mini (XID/XInput/CDC) + PicoW-USB-BT-Audio (UAC2 audio)
 */
#ifndef _TUSB_CONFIG_HYBRID_H_
#define _TUSB_CONFIG_HYBRID_H_

#ifdef __cplusplus
extern "C" {
#endif

// Include audio descriptor helpers (defines TUD_AUDIO_HEADSET_STEREO_DESC_LEN etc.)
#include "tinyusb/usb_descriptors.h"

//--------------------------------------------------------------------
// Board / MCU
//--------------------------------------------------------------------
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
#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif
#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN    __attribute__ ((aligned(4)))
#endif

//--------------------------------------------------------------------
// Device stack
//--------------------------------------------------------------------
#define CFG_TUD_ENABLED           1
#define CFG_TUSB_RHPORT0_MODE     (OPT_MODE_DEVICE)
#define CFG_TUD_MAX_SPEED         BOARD_TUD_MAX_SPEED
#define CFG_TUD_TASK_QUEUE_SZ     64

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE    64
#endif

//--------------------------------------------------------------------
// Class drivers — combined OGX-Mini + BT Audio
//--------------------------------------------------------------------
#define CFG_TUD_CDC               1
#define CFG_TUD_CDC_EP_BUFSIZE    64
#define CFG_TUD_CDC_TX_BUFSIZE    256
#define CFG_TUD_CDC_RX_BUFSIZE    256

#define CFG_TUD_MSC               0
#define CFG_TUD_MIDI              0
#define CFG_TUD_VENDOR            0

#define CFG_TUD_HID               1
#define CFG_TUD_HID_EP_BUFSIZE    64

#define CFG_TUD_XID               1
#define CFG_TUD_XINPUT            1

//--------------------------------------------------------------------
// UAC2 Audio — from PicoW-USB-BT-Audio (exact values)
//--------------------------------------------------------------------
#define CFG_TUD_AUDIO             1
#define CFG_TUD_AUDIO_ENABLE_INTERRUPT_EP                           1
#define CFG_TUD_AUDIO_FUNC_1_DESC_LEN                               TUD_AUDIO_HEADSET_STEREO_DESC_LEN
#define CFG_TUD_AUDIO_FUNC_1_N_FORMATS                              2
#define CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE                        48000
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX                          0
#define CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX                          2
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_RX         2
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_RX                 16
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_2_N_BYTES_PER_SAMPLE_RX         2
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_2_RESOLUTION_RX                 16

#define CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE                        48000
#define CFG_TUD_AUDIO_FUNC_1_FORMAT_1_EP_SZ_OUT \
    TUD_AUDIO_EP_SIZE(CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE, \
                      CFG_TUD_AUDIO_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_RX, \
                      CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX)

#define CFG_TUD_AUDIO_ENABLE_EP_OUT                                 1
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ \
    (CFG_TUD_AUDIO_FUNC_1_FORMAT_1_EP_SZ_OUT) * 2
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX \
    (CFG_TUD_AUDIO_FUNC_1_FORMAT_1_EP_SZ_OUT)
#define CFG_TUD_AUDIO_FUNC_1_N_AS_INT                               1
#define CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ                           64

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_HYBRID_H_ */
