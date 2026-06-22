/* memmap_rp2350_offset.ld
 *
 * Linker script para RP2350 (Pico 2 W) executando a partir da FLASH,
 * com endereco de INICIO PARAMETRIZAVEL (__FLASH_OFFSET) para permitir
 * multiplos binarios na mesma flash (bootloader + app controle + app audio).
 *
 * Reconstruido combinando, palavra por palavra, dois arquivos REAIS e
 * atuais obtidos diretamente do repositorio oficial raspberrypi/pico-sdk
 * (branch master, lidos em 2026-06-21):
 *
 *  1) src/rp2_common/pico_crt0/rp2350/memmap_no_flash.ld
 *     -> fonte da estrutura RP2350 (embedded_block, binary_info_header,
 *        ordem de secoes .text/.rodata/.data/.bss/.heap, suporte a
 *        .tdata/.tbss/.sdata para RISC-V, asserts finais)
 *
 *  2) src/rp2_common/pico_crt0/rp2040/memmap_default.ld (== antigo
 *     pico_standard_link/memmap_default.ld, identico em conteudo)
 *     -> fonte do padrao "executa da FLASH" (.data com AT> FLASH,
 *        .scratch_x/.scratch_y AT> FLASH, .flash_begin/.flash_end)
 *
 * Diferenca chave do RP2040: RP2350 NAO tem secao .boot2 (confirmado:
 * ausente em ambos os arquivos oficiais RP2350 consultados, e
 * documentado explicitamente como "not needed" por multiplas fontes
 * da comunidade ao adaptar linker scripts pra RP2350).
 *
 * Uso: este arquivo e um TEMPLATE. As marcações 0x001EE800 e
 * 0x001F4000 abaixo são substituídas por valores literais reais
 * (ex: 0x00040000, 0x001C2000) via configure_file() do CMake, gerando
 * um .ld concreto por sub-projeto antes da compilação. Isso evita
 * depender do linker do GNU ld processar macros de pré-processador,
 * que não é um comportamento garantido/portável para arquivos .ld
 * passados via pico_set_linker_script().
 */

MEMORY
{
    FLASH(rx)  : ORIGIN = 0x10000000 + 0x001EE800, LENGTH = 0x001F4000
    RAM(rwx)   : ORIGIN = 0x20000000, LENGTH = 512k
    SCRATCH_X(rwx) : ORIGIN = 0x20080000, LENGTH = 4k
    SCRATCH_Y(rwx) : ORIGIN = 0x20081000, LENGTH = 4k
}

ENTRY(_entry_point)

SECTIONS
{
    .flash_begin : {
        __flash_binary_start = .;
    } > FLASH

    /* RP2350: nao ha secao .boot2 (o bootrom faz a configuracao XIP
     * automaticamente). A imagem comeca direto com o vetor + bloco
     * embutido obrigatorio (.embedded_block), exigido pelo bootrom
     * do RP2350 para reconhecer um binario valido. */
    .text : {
        __logical_binary_start = .;
        KEEP (*(.vectors))
        KEEP (*(.binary_info_header))
        __binary_info_header_end = .;
        KEEP (*(.embedded_block))
        __embedded_block_end = .;
        __reset_start = .;
        KEEP (*(.reset))
        __reset_end = .;

        *(.init)
        *(EXCLUDE_FILE(*libgcc.a: *libc.a:*lib_a-mem*.o *libm.a:) .text*)
        *(.fini)

        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)

        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)

        *(.eh_frame*)
        . = ALIGN(4);
    } > FLASH

    .rodata : {
        *(EXCLUDE_FILE(*libgcc.a: *libc.a:*lib_a-mem*.o *libm.a:) .rodata*)
        *(.srodata*)
        . = ALIGN(4);
        *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.flashdata*)))
        . = ALIGN(4);
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH
    __exidx_end = .;

    /* Machine inspectable binary information */
    . = ALIGN(4);
    __binary_info_start = .;
    .binary_info :
    {
        KEEP(*(.binary_info.keep.*))
        *(.binary_info.*)
    } > FLASH
    __binary_info_end = .;
    . = ALIGN(4);

    __etext = .;

    .ram_vector_table (COPY): {
        *(.ram_vector_table)
    } > RAM

    .data : {
        __data_start__ = .;
        *(vtable)
        *(.time_critical*)
        /* texto/rodata restantes que foram explicitamente excluidos
         * acima por causa do EXCLUDE_FILE (libgcc/libc/libm trechos
         * sensiveis a tempo, mantidos em RAM como no padrao oficial) */
        *(.text*)
        . = ALIGN(4);
        *(.rodata*)
        . = ALIGN(4);
        *(.data*)
        *(.sdata*)
        . = ALIGN(4);
        *(.after_data.*)
        . = ALIGN(4);

        PROVIDE_HIDDEN (__mutex_array_start = .);
        KEEP(*(SORT(.mutex_array.*)))
        KEEP(*(.mutex_array))
        PROVIDE_HIDDEN (__mutex_array_end = .);
        . = ALIGN(4);

        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(SORT(.preinit_array.*)))
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);
        . = ALIGN(4);

        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);
        . = ALIGN(4);

        PROVIDE_HIDDEN (__fini_array_start = .);
        *(SORT(.fini_array.*))
        *(.fini_array)
        PROVIDE_HIDDEN (__fini_array_end = .);

        *(.jcr)
        . = ALIGN(4);
        __data_end__ = .;
    } > RAM AT> FLASH

    .tdata : {
        . = ALIGN(4);
        *(.tdata .tdata.* .gnu.linkonce.td.*)
        __tdata_end = .;
    } > RAM AT> FLASH

    .uninitialized_data (COPY): {
        . = ALIGN(4);
        *(.uninitialized_data*)
    } > RAM

    .tbss (NOLOAD) : {
        . = ALIGN(4);
        __tls_base = .;
        *(.tbss .tbss.* .gnu.linkonce.tb.*)
        *(.tcommon)
        __tls_end = .;
    } > RAM

    /* Start and end symbols must be word-aligned */
    .scratch_x : {
        __scratch_x_start__ = .;
        *(.scratch_x.*)
        . = ALIGN(4);
        __scratch_x_end__ = .;
    } > SCRATCH_X AT > FLASH
    __scratch_x_source__ = LOADADDR(.scratch_x);

    .scratch_y : {
        __scratch_y_start__ = .;
        *(.scratch_y.*)
        . = ALIGN(4);
        __scratch_y_end__ = .;
    } > SCRATCH_Y AT > FLASH
    __scratch_y_source__ = LOADADDR(.scratch_y);

    .bss : {
        . = ALIGN(4);
        __bss_start__ = .;
        __tbss_end = .;
        *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.bss*)))
        *(COMMON)
        PROVIDE(__global_pointer$ = . + 2K);
        *(.sbss*)
        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM

    .heap (COPY):
    {
        __end__ = .;
        end = __end__;
        KEEP(*(.heap*))
        __HeapLimit = .;
    } > RAM

    .stack1_dummy (COPY):
    {
        *(.stack1*)
    } > SCRATCH_X

    .stack_dummy (COPY):
    {
        KEEP(*(.stack*))
    } > SCRATCH_Y

    .flash_end : {
        __flash_binary_end = .;
    } > FLASH

    __StackLimit = ORIGIN(RAM) + LENGTH(RAM);
    __StackOneTop = ORIGIN(SCRATCH_X) + LENGTH(SCRATCH_X);
    __StackTop = ORIGIN(SCRATCH_Y) + LENGTH(SCRATCH_Y);
    __StackOneBottom = __StackOneTop - SIZEOF(.stack1_dummy);
    __StackBottom = __StackTop - SIZEOF(.stack_dummy);
    PROVIDE(__stack = __StackTop);

    PROVIDE (__heap_start = __end__);
    PROVIDE (__heap_end = __HeapLimit);
    PROVIDE (_end = __end__);
    PROVIDE (__llvm_libc_heap_limit = __HeapLimit);

    ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed")
    ASSERT( __binary_info_header_end - __logical_binary_start <= 1024, "Binary info must be in first 1024 bytes of the binary")
    ASSERT( __embedded_block_end - __logical_binary_start <= 4096, "Embedded block must be in first 4096 bytes of the binary")
    ASSERT( __flash_binary_end <= (ORIGIN(FLASH) + LENGTH(FLASH)), "ERROR: binario excede o espaco de flash reservado para esta particao (__FLASH_OFFSET/__FLASH_LENGTH) -- aumente __FLASH_LENGTH dessa particao no CMakeLists")
}
