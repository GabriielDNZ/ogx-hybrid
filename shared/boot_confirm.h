// boot_confirm.h
// Biblioteca leve, compartilhada pelos DOIS firmwares de aplicacao
// (controle e audio), para que cada um possa:
//   1) Saber em qual modo o bootloader pediu para ele rodar (nao usado
//      diretamente, pois o proprio binario ja eh especifico de 1 modo)
//   2) Confirmar ao bootloader, via flash, que esse modo "funcionou de
//      verdade" (host aceitou o dispositivo), para o bootloader
//      preferir esse modo no proximo boot.
//
// Isto e o NUCLEO da estrategia "lembra o ultimo modo que funcionou":
// cada app, assim que perceber que o host realmente o reconheceu,
// chama boot_confirm_mode_success(MODE_X) UMA vez. Isso e barato
// (escreve so quando muda) e seguro mesmo chamado varias vezes.

#ifndef _BOOT_CONFIRM_H_
#define _BOOT_CONFIRM_H_

// =================================================================
// REQUISITO CRITICO PARA QUEM INTEGRAR ESTE ARQUIVO NUM APP:
//
// Se o seu app usa Core1 (ex: BTstack rodando em segundo core, como
// e o caso do app de Controle com Bluepad32), o Core1 PRECISA chamar
// flash_safe_execute_core_init() logo no inicio da sua funcao de
// entrada (antes de qualquer loop longo), ANTES que boot_confirm_*
// seja chamado de qualquer core. Caso contrario, flash_safe_execute()
// vai falhar (PICO_ERROR_NOT_PERMITTED) e a confirmacao de modo
// simplesmente nao sera salva (silenciosamente -- o app continua
// funcionando normalmente, so nao "lembra" o modo de boot).
//
// Exemplo (dentro da funcao que roda em Core1):
//     #include "pico/flash.h"
//     void core1_entry(void) {
//         flash_safe_execute_core_init();
//         ... resto do codigo do core1 ...
//     }
//
// Apps que rodam tudo em Core0 (como o app de Audio, neste projeto)
// nao precisam dessa chamada.
// =================================================================

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "pico/flash.h"
#include "flash_map.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline uint32_t boot_confirm__crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int b = 0; b < 8; ++b) {
            uint32_t mask = -(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

// Estrutura auxiliar para passar os dados pro callback do flash_safe_execute
typedef struct {
    uint32_t offset;
    const uint8_t* page_buf;
} boot_confirm__write_ctx_t;

static inline void boot_confirm__do_write(void* param) {
    boot_confirm__write_ctx_t* ctx = (boot_confirm__write_ctx_t*)param;
    flash_range_erase(ctx->offset, FLASH_SECTOR_SIZE);
    flash_range_program(ctx->offset, ctx->page_buf, FLASH_PAGE_SIZE);
}

static inline void boot_confirm__write_config(const boot_config_t* cfg) {
    uint8_t page_buf[FLASH_PAGE_SIZE];
    memset(page_buf, 0xFF, sizeof(page_buf));
    memcpy(page_buf, cfg, sizeof(*cfg));

    uint32_t flash_offset = CONFIG_SECTOR_OFFSET; // offset relativo ao inicio da flash

    boot_confirm__write_ctx_t ctx = {
        .offset = flash_offset,
        .page_buf = page_buf,
    };

    // flash_safe_execute() coordena com o outro core (se estiver
    // rodando) e desliga interrupcoes/XIP de forma segura antes de
    // apagar/escrever a flash. Isso e OBRIGATORIO neste projeto
    // porque o app de controle roda BTstack em Core1 simultaneamente
    // ao Core0 -- escrever flash sem essa coordenacao pode travar o
    // Core1 caso ele esteja buscando instrucoes via XIP nesse exato
    // momento. Timeout generoso de 1s (a escrita real leva poucos ms).
    int rc = flash_safe_execute(boot_confirm__do_write, &ctx, 1000);
    if (rc != PICO_OK) {
        // Nao ha muito a fazer aqui alem de tentar de novo mais tarde;
        // nunca travar o app por causa disso. O modo simplesmente nao
        // sera lembrado nesta tentativa, e o bootloader usara a
        // logica de fallback (3 tentativas) no proximo boot.
        return;
    }
}

// ---------------------------------------------------------------
// Watchdog de confirmacao de boot.
//
// Cada app DEVE chamar boot_confirm_arm_watchdog() bem no inicio do
// seu main(), antes de qualquer coisa que possa travar (init de
// USB, BTstack, etc). Isso arma um watchdog de ~8 segundos.
//
// Se o app travar (loop infinito, hard fault, panic) antes de
// conseguir provar que o host o aceitou, o watchdog reinicia o
// Pico sozinho -- e o bootloader entra em acao de novo, contando
// mais uma tentativa falha (ate alternar de modo apos 3 falhas).
//
// Quando o app perceber sucesso real (host respondeu), ele chama
// boot_confirm_mode_success(), que tambem desarma o watchdog
// (ou o app pode continuar alimentando se preferir manter watchdog
// de runtime normal -- ver nota abaixo).
// ---------------------------------------------------------------
#define BOOT_CONFIRM_WATCHDOG_MS 8000

static inline void boot_confirm_arm_watchdog(void) {
    watchdog_enable(BOOT_CONFIRM_WATCHDOG_MS, true);
}

static inline void boot_confirm_feed_watchdog(void) {
    watchdog_update();
}


// o dispositivo (ex: Xbox mandando rumble via XInput; ou PS5 abrindo
// o stream de audio UAC). Seguro de chamar mais de uma vez.
static inline void boot_confirm_mode_success(uint8_t mode) {
    const boot_config_t* current = (const boot_config_t*)(CONFIG_SECTOR_ADDR);

    // Ja esta gravado como esse modo? nao precisa regravar (flash tem
    // numero finito de gravacoes, evitar desgaste desnecessario).
    if (current->magic == CONFIG_MAGIC &&
        current->last_working_mode == mode &&
        current->boot_attempt_count == 0) {
        return;
    }

    boot_config_t new_cfg;
    new_cfg.magic = CONFIG_MAGIC;
    new_cfg.last_working_mode = mode;
    new_cfg.boot_attempt_mode = mode;
    new_cfg.boot_attempt_count = 0; // sucesso confirmado, zera contador
    new_cfg.reserved = 0;
    new_cfg.crc32 = boot_confirm__crc32((const uint8_t*)&new_cfg,
                                         offsetof(boot_config_t, crc32));

    boot_confirm__write_config(&new_cfg);
}

#ifdef __cplusplus
}
#endif

#endif // _BOOT_CONFIRM_H_
