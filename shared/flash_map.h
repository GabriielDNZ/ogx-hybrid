// flash_map.h
// Mapa de particoes de flash compartilhado entre o bootloader (selector)
// e os dois firmwares de aplicacao (controle / audio).
//
// Pico 2 W (RP2350) - 4MB (0x400000) de flash total, comecando em 0x10000000.
//
//  0x10000000 .......... Bootloader (este projeto)               128 KB
//  0x10020000 .......... App CONTROLE (OGX-Mini)                 1850 KB
//  0x101EE800 .......... App AUDIO (USB->BT Audio)                2000 KB
//  0x103F3800 .......... Setor de configuracao persistente (4KB)
//  0x10400000 .......... Fim da flash (sobram ~2.1MB de respiro)
//
// IMPORTANTE: estes valores devem ser EXATAMENTE os mesmos usados nos
// linker scripts / CMake (__FLASH_OFFSET / __FLASH_LENGTH) dos dois apps.
// O bootloader NAO linka CYW43/BTstack (so flash+GPIO+watchdog), por
// isso cabe folgado em 128KB; os apps linkam o blob de firmware
// Wi-Fi/Bluetooth do CYW43 (~300-400KB), por isso tem bem mais espaco.

#ifndef _FLASH_MAP_H_
#define _FLASH_MAP_H_

#define FLASH_BASE_ADDR        0x10000000u

#define BOOTLOADER_OFFSET       0x00000000u
#define BOOTLOADER_SIZE         0x00020000u   // 128 KB

#define APP_CONTROLLER_OFFSET   0x00020000u   // 128 KB
#define APP_CONTROLLER_SIZE     0x001CE800u   // 1850 KB

#define APP_AUDIO_OFFSET        0x001EE800u   // 1978 KB
#define APP_AUDIO_SIZE          0x001F4000u   // 2000 KB

#define CONFIG_SECTOR_OFFSET    0x003E2800u   // antes do fim, com folga
#define CONFIG_SECTOR_SIZE      0x00001000u   // 4 KB (1 setor de flash)

#define APP_CONTROLLER_ADDR    (FLASH_BASE_ADDR + APP_CONTROLLER_OFFSET)
#define APP_AUDIO_ADDR         (FLASH_BASE_ADDR + APP_AUDIO_OFFSET)
#define CONFIG_SECTOR_ADDR     (FLASH_BASE_ADDR + CONFIG_SECTOR_OFFSET)

// Magic number para validar se o setor de config foi inicializado
#define CONFIG_MAGIC            0x4F475842u   // "OGXB"

// Modos possiveis
#define MODE_CONTROLLER          0x01u
#define MODE_AUDIO                0x02u

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint8_t  last_working_mode;   // MODE_CONTROLLER ou MODE_AUDIO
    uint8_t  boot_attempt_mode;   // modo que o bootloader acabou de tentar
    uint8_t  boot_attempt_count;  // tentativas consecutivas sem confirmacao
    uint8_t  reserved;
    uint32_t crc32;               // simples checksum dos campos acima
} boot_config_t;

#endif // _FLASH_MAP_H_
