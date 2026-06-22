# flash_layout.cmake
#
# Fonte UNICA da verdade para o mapa de particoes de flash do Pico 2 W
# (4MB / 0x400000 bytes), compartilhada entre bootloader, app-controller
# e app-audio. Estes valores devem corresponder EXATAMENTE aos definidos
# em bootloader/src/flash_map.h (mantidos sincronizados manualmente -
# ver checklist no README do projeto).
#
# Cada sub-projeto faz: include(${CMAKE_CURRENT_LIST_DIR}/../shared/flash_layout.cmake)
# e depois usa FLASH_OFFSET / FLASH_LENGTH (definidos conforme qual
# sub-projeto esta incluindo este arquivo, via TARGET_PARTITION).

set(TOTAL_FLASH_SIZE_BYTES "4194304") # 4MB = 0x400000

set(BOOTLOADER_OFFSET_HEX     "0x00000000")
set(BOOTLOADER_LENGTH_HEX     "0x00020000") # 128KB

set(APP_CONTROLLER_OFFSET_HEX "0x00020000") # 128KB
set(APP_CONTROLLER_LENGTH_HEX "0x001CE800") # 1850KB

set(APP_AUDIO_OFFSET_HEX      "0x001EE800") # 1978KB
set(APP_AUDIO_LENGTH_HEX      "0x001F4000") # 2000KB

set(CONFIG_SECTOR_OFFSET_HEX  "0x003E2800")
set(CONFIG_SECTOR_LENGTH_HEX  "0x00001000") # 4KB

# TARGET_PARTITION deve ser setado pelo CMakeLists.txt do sub-projeto
# ANTES de incluir este arquivo: "bootloader", "controller" ou "audio".
if(TARGET_PARTITION STREQUAL "bootloader")
    set(FLASH_OFFSET ${BOOTLOADER_OFFSET_HEX})
    set(FLASH_LENGTH ${BOOTLOADER_LENGTH_HEX})
elseif(TARGET_PARTITION STREQUAL "controller")
    set(FLASH_OFFSET ${APP_CONTROLLER_OFFSET_HEX})
    set(FLASH_LENGTH ${APP_CONTROLLER_LENGTH_HEX})
elseif(TARGET_PARTITION STREQUAL "audio")
    set(FLASH_OFFSET ${APP_AUDIO_OFFSET_HEX})
    set(FLASH_LENGTH ${APP_AUDIO_LENGTH_HEX})
else()
    message(FATAL_ERROR "flash_layout.cmake: defina TARGET_PARTITION como 'bootloader', 'controller' ou 'audio' ANTES do include()")
endif()

message(STATUS "flash_layout: particao='${TARGET_PARTITION}' offset=${FLASH_OFFSET} length=${FLASH_LENGTH}")
