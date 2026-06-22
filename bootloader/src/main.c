// main.c - Bootloader seletor (3rd stage) para Pico 2 W
//
// Responsabilidade UNICA: ler da flash qual foi o ultimo modo que
// funcionou (controle ou audio), e pular a execucao para o firmware
// correspondente. Nao faz mais nada - nao inicializa USB, nao mexe
// em Bluetooth. Deve ser o menor e mais simples possivel, porque eh
// o unico componente que roda SEMPRE, em todo boot.
//
// Logica de fallback:
//   - Se o setor de config nunca foi escrito (chip novo), tenta
//     MODE_CONTROLLER por padrao.
//   - Le last_working_mode e pula pra ele.
//   - Antes de pular, grava boot_attempt_count += 1 (se nao foi
//     resetado por um boot_confirm_mode_success do app anterior).
//   - Se boot_attempt_count >= 3 sem confirmacao -- ou seja, o app
//     escolhido tem travado/resetado repetidas vezes sem nunca dar
//     boot_confirm_mode_success -- alterna automaticamente pro outro
//     modo, pra nao ficar preso num featureset que nao funciona
//     fisicamente naquele console.
//
// O APP em si (controle ou audio) eh responsavel por chamar
// boot_confirm_mode_success() assim que perceber que o host
// realmente o aceitou. Ver boot_confirm.h.

#include <stdint.h>
#include <string.h>
#include <stddef.h>

#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/structs/scb.h"
#include "hardware/watchdog.h"

#include "flash_map.h"
#include "boot_confirm.h"

// ---------------------------------------------------------------
// Util: validar e ler a config atual da flash
// ---------------------------------------------------------------
static bool read_config(boot_config_t* out) {
    const boot_config_t* cfg = (const boot_config_t*)(CONFIG_SECTOR_ADDR);
    if (cfg->magic != CONFIG_MAGIC) {
        return false;
    }
    uint32_t expected_crc = boot_confirm__crc32((const uint8_t*)cfg,
                                                  offsetof(boot_config_t, crc32));
    if (expected_crc != cfg->crc32) {
        return false;
    }
    memcpy(out, cfg, sizeof(*out));
    return true;
}

// ---------------------------------------------------------------
// Salta para o firmware de aplicacao no endereco indicado.
// Baseado no padrao oficial usado em chainloaders RP2350
// (ajusta VTOR para Cortex-M33 e faz o salto manual).
// Esta funcao NUNCA retorna.
// ---------------------------------------------------------------
static void __attribute__((noreturn)) jump_to_app(uint32_t app_flash_addr) {
    // Desliga qualquer periferico que o bootloader tenha ligado
    // (aqui nao ligamos nada alem do basico, mas por seguranca
    // desabilitamos interrupcoes antes do salto).

    uint32_t* vtor_app = (uint32_t*)app_flash_addr;
    uint32_t sp = vtor_app[0];
    uint32_t reset_handler = vtor_app[1];

    // Sanity check basico: o stack pointer deve apontar pra SRAM
    // (0x20000000 - 0x20082000 no RP2350) e o reset handler deve
    // estar dentro da regiao de flash do app. Se nao bater, o
    // binario daquele slot provavelmente nao foi gravado ainda --
    // nesse caso entramos em modo BOOTSEL para permitir reflash.
    bool sp_ok = (sp >= 0x20000000u) && (sp <= 0x20082000u);
    bool pc_ok = (reset_handler >= app_flash_addr) &&
                 (reset_handler < (FLASH_BASE_ADDR + 0x00400000u));

    if (!sp_ok || !pc_ok) {
        // Firmware ausente/corrompido nesse slot: cai pro modo
        // BOOTSEL nativo do RP2350, para permitir gravar via drag&drop.
        reset_usb_boot(0, 0);
        while (1) { tight_loop_contents(); }
    }

    // Desativa watchdog do bootloader antes de entregar o controle
    // (usa a API oficial; o app, se quiser seu proprio watchdog,
    // chama watchdog_enable() de novo logo no inicio dele).
    watchdog_disable();

    // Redireciona a tabela de vetores (VTOR) do Cortex-M33 para o
    // inicio do firmware de aplicacao.
    scb_hw->vtor = app_flash_addr;

    __asm volatile (
        "msr msp, %0   \n"
        "bx   %1       \n"
        :
        : "r" (sp), "r" (reset_handler)
        :
    );

    while (1) { /* nunca chega aqui */ }
}

int main(void) {
    // Pequeno delay para estabilizacao de alimentacao/USB ao plugar.
    sleep_ms(20);

    boot_config_t cfg;
    bool have_cfg = read_config(&cfg);

    uint8_t mode_to_try;

    if (!have_cfg) {
        // Primeira vez (flash de fabrica / nunca configurado):
        // comeca tentando o modo CONTROLE.
        mode_to_try = MODE_CONTROLLER;

        boot_config_t fresh;
        fresh.magic = CONFIG_MAGIC;
        fresh.last_working_mode = MODE_CONTROLLER;
        fresh.boot_attempt_mode = MODE_CONTROLLER;
        fresh.boot_attempt_count = 1;
        fresh.reserved = 0;
        fresh.crc32 = boot_confirm__crc32((const uint8_t*)&fresh,
                                            offsetof(boot_config_t, crc32));
        boot_confirm__write_config(&fresh);
    } else {
        mode_to_try = cfg.last_working_mode;

        // Se o ultimo boot nao foi confirmado pelo app (boot_attempt_count
        // ja estava > 0 quando chegamos aqui de novo == o app anterior
        // resetou/travou sem chamar boot_confirm_mode_success), conta
        // mais uma tentativa falha.
        uint8_t new_attempt_count = cfg.boot_attempt_count + 1;

        if (new_attempt_count >= 3) {
            // 3 falhas seguidas nesse modo: tenta o modo oposto.
            mode_to_try = (cfg.last_working_mode == MODE_CONTROLLER)
                            ? MODE_AUDIO : MODE_CONTROLLER;
            new_attempt_count = 1; // reseta contador para o novo modo
        }

        boot_config_t updated = cfg;
        updated.boot_attempt_mode = mode_to_try;
        updated.boot_attempt_count = new_attempt_count;
        updated.crc32 = boot_confirm__crc32((const uint8_t*)&updated,
                                              offsetof(boot_config_t, crc32));
        boot_confirm__write_config(&updated);
    }

    uint32_t target_addr = (mode_to_try == MODE_AUDIO)
                              ? APP_AUDIO_ADDR
                              : APP_CONTROLLER_ADDR;

    jump_to_app(target_addr);

    // Nunca chega aqui.
    while (1) { tight_loop_contents(); }
}
