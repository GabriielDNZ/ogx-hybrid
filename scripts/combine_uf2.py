#!/usr/bin/env python3
"""
combine_uf2.py

Combina varios arquivos .uf2 (cada um compilado para um OFFSET de flash
diferente, sem sobreposicao) em um UNICO arquivo .uf2 valido.

Por que isso funciona: o formato UF2 e uma sequencia de blocos de 512
bytes, cada um AUTOCONTIDO -- carrega seu proprio endereco de destino
(targetAddr) dentro do header. Concatenar os blocos de varios arquivos
.uf2 (compilados para enderecos diferentes) produz um arquivo unico
valido, DESDE QUE:
  1) os enderecos de destino de cada arquivo de entrada NUNCA se
     sobreponham entre si (validado abaixo);
  2) os campos blockNo/numBlocks sejam RENUMERADOS para refletir a
     posicao de cada bloco no arquivo COMBINADO final, nao no arquivo
     de origem individual (a especificacao UF2 diz que esses campos
     ajudam o bootloader/SO a saber quando todos os blocos chegaram;
     deixar valores incorretos pode causar um flash incompleto/
     prematuramente considerado concluido por alguns sistemas).

Uso:
    python3 combine_uf2.py \
        --out combined.uf2 \
        bootloader.uf2 controller.uf2 audio.uf2

Os arquivos de entrada sao processados na ordem dada, mas o resultado
final e logicamente equivalente independente da ordem (cada bloco
carrega seu proprio endereco).
"""

import argparse
import struct
import sys

UF2_MAGIC_START0 = 0x0A324655
UF2_MAGIC_START1 = 0x9E5D5157
UF2_MAGIC_END = 0x0AB16F30
BLOCK_SIZE = 512
HEADER_SIZE = 32
DATA_SIZE = 476  # 512 - 32 (header) - 4 (magic final)

FLAG_NOT_MAIN_FLASH = 0x00000001
FLAG_FAMILY_ID_PRESENT = 0x00002000


class UF2Block:
    __slots__ = ("flags", "target_addr", "payload_size", "block_no",
                 "num_blocks", "file_size_or_family", "data")

    def __init__(self, raw: bytes):
        if len(raw) != BLOCK_SIZE:
            raise ValueError(f"Bloco com tamanho invalido: {len(raw)} (esperado {BLOCK_SIZE})")

        magic0, magic1, flags, target_addr, payload_size, block_no, num_blocks, file_size_or_family = \
            struct.unpack_from("<8I", raw, 0)

        if magic0 != UF2_MAGIC_START0 or magic1 != UF2_MAGIC_START1:
            raise ValueError("Numeros magicos de inicio invalidos -- arquivo nao e UF2 valido")

        magic_end = struct.unpack_from("<I", raw, BLOCK_SIZE - 4)[0]
        if magic_end != UF2_MAGIC_END:
            raise ValueError("Numero magico final invalido -- arquivo nao e UF2 valido / bloco corrompido")

        self.flags = flags
        self.target_addr = target_addr
        self.payload_size = payload_size
        self.block_no = block_no
        self.num_blocks = num_blocks
        self.file_size_or_family = file_size_or_family
        self.data = raw[HEADER_SIZE:HEADER_SIZE + DATA_SIZE]

    def to_bytes(self, new_block_no: int, new_num_blocks: int) -> bytes:
        header = struct.pack(
            "<8I",
            UF2_MAGIC_START0,
            UF2_MAGIC_START1,
            self.flags,
            self.target_addr,
            self.payload_size,
            new_block_no,
            new_num_blocks,
            self.file_size_or_family,
        )
        footer = struct.pack("<I", UF2_MAGIC_END)
        # data ja tem exatamente DATA_SIZE bytes (preservado do bloco original)
        return header + self.data + footer


def read_uf2_blocks(path: str):
    blocks = []
    with open(path, "rb") as f:
        raw = f.read()

    if len(raw) % BLOCK_SIZE != 0:
        raise ValueError(f"{path}: tamanho do arquivo ({len(raw)} bytes) nao e multiplo de {BLOCK_SIZE}")

    num_chunks = len(raw) // BLOCK_SIZE
    for i in range(num_chunks):
        chunk = raw[i * BLOCK_SIZE:(i + 1) * BLOCK_SIZE]
        block = UF2Block(chunk)

        # Blocos com FLAG_NOT_MAIN_FLASH sao "comentarios"/metadados,
        # nao devem ser escritos na flash -- mas sao raros em builds
        # normais do pico-sdk. Mantemos eles tambem (preserva o
        # comportamento original), so avisamos no log se aparecerem.
        blocks.append(block)

    return blocks


def check_overlap(file_ranges):
    """file_ranges: lista de (nome, addr_min, addr_max) -- addr_max EXCLUSIVO"""
    sorted_ranges = sorted(file_ranges, key=lambda r: r[1])
    for i in range(len(sorted_ranges) - 1):
        name_a, _, end_a = sorted_ranges[i]
        name_b, start_b, _ = sorted_ranges[i + 1]
        if end_a > start_b:
            raise ValueError(
                f"SOBREPOSICAO DE ENDERECOS DETECTADA entre '{name_a}' e '{name_b}': "
                f"'{name_a}' termina em 0x{end_a:08X}, mas '{name_b}' comeca em 0x{start_b:08X}. "
                f"Isso indica um erro no mapa de particoes (shared/flash_layout.cmake) -- "
                f"os firmwares iriam se sobrescrever mutuamente na flash real. ABORTANDO "
                f"para evitar gravar um .uf2 corrompido."
            )


def main():
    parser = argparse.ArgumentParser(description="Combina multiplos .uf2 (offsets diferentes, sem sobreposicao) em um unico .uf2")
    parser.add_argument("inputs", nargs="+", help="Arquivos .uf2 de entrada, em qualquer ordem")
    parser.add_argument("--out", required=True, help="Arquivo .uf2 de saida combinado")
    args = parser.parse_args()

    all_blocks = []
    file_ranges = []

    for path in args.inputs:
        print(f"Lendo {path} ...")
        blocks = read_uf2_blocks(path)
        if not blocks:
            print(f"  AVISO: {path} nao contem blocos. Pulando.")
            continue

        addrs = [b.target_addr for b in blocks]
        addr_min = min(addrs)
        # payload_size normalmente e 256; o range real ocupado e
        # [addr_min, addr_max_do_ultimo_bloco + payload_size_desse_bloco)
        last_block = max(blocks, key=lambda b: b.target_addr)
        addr_max_exclusive = last_block.target_addr + last_block.payload_size

        print(f"  {len(blocks)} blocos, enderecos 0x{addr_min:08X} - 0x{addr_max_exclusive:08X} "
              f"({(addr_max_exclusive - addr_min) / 1024:.1f} KB)")

        file_ranges.append((path, addr_min, addr_max_exclusive))
        all_blocks.extend(blocks)

    print()
    print("Verificando sobreposicao de enderecos entre os arquivos de entrada...")
    check_overlap(file_ranges)
    print("OK: nenhuma sobreposicao detectada.")
    print()

    # Ordena todos os blocos pelo endereco de destino, para que o
    # arquivo final fique em ordem crescente de endereco (mais facil
    # de inspecionar/depurar; nao e estritamente necessario pelo
    # formato UF2, ja que cada bloco e autocontido).
    all_blocks.sort(key=lambda b: b.target_addr)

    total = len(all_blocks)
    print(f"Total de blocos combinados: {total}")

    with open(args.out, "wb") as out_f:
        for i, block in enumerate(all_blocks):
            out_f.write(block.to_bytes(new_block_no=i, new_num_blocks=total))

    print(f"Arquivo combinado escrito em: {args.out}")
    print(f"Tamanho final: {total * BLOCK_SIZE} bytes ({total * BLOCK_SIZE / 1024:.1f} KB)")


if __name__ == "__main__":
    try:
        main()
    except ValueError as e:
        print(f"\nERRO: {e}", file=sys.stderr)
        sys.exit(1)
