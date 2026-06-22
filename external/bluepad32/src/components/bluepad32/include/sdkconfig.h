#ifndef _SDK_CONFIG_H_
#define _SDK_CONFIG_H_

// Standalone sdkconfig for hybrid build (Pico 2W)
// Replaces OGX-Mini's sdkconfig.h which depends on Board/Config.h

#define CONFIG_BLUEPAD32_MAX_DEVICES    1
#define CONFIG_BLUEPAD32_MAX_ALLOWLIST  4
#define CONFIG_BLUEPAD32_GAP_SECURITY   1
#define CONFIG_BLUEPAD32_ENABLE_BLE_BY_DEFAULT 1

#define CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#define CONFIG_TARGET_PICO_W

// 0 = errors only, 2 = info
#define CONFIG_BLUEPAD32_LOG_LEVEL 0

#endif //_SDK_CONFIG_H_
