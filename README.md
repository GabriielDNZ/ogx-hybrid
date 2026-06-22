# OGX-Mini + USB-BT-Audio — Firmware Híbrido (Pico 2 W)

Este projeto faz o Pico 2 W decidir sozinho, ao ligar, se ele deve se
comportar como **controle de Xbox** (recebendo input de um controle
pareado por Bluetooth) ou como **placa de som USB** que retransmite
áudio do PS5 por Bluetooth para uma caixa de som (ex: JBL).

Não existe um botão para trocar de modo: o Pico tenta o último modo
que funcionou da última vez; se não for reconhecido em alguns
segundos, ele troca para o outro modo sozinho e tenta de novo. Depois
de algumas trocas, ele aprende e passa a ir direto no modo certo.

### Gravar no Pico 2 W
1. Desconecte o Pico de tudo
2. Segure o botão **BOOTSEL** do Pico e, ainda segurando, conecte o
   cabo USB no computador
3. Solte o botão — uma unidade de armazenamento chamada `RP2350` (ou
   similar) vai aparecer no seu computador, como se fosse um pendrive
4. Arraste o arquivo `OGX-Mini-Hybrid-PICO2W.uf2` para dentro dessa
   unidade
5. O Pico vai gravar e reiniciar sozinho automaticamente

Pronto — agora é só plugar no Xbox ou no PS5 e testar.

## Como funciona por dentro (resumo técnico)

- A flash de 4MB do Pico 2 W é dividida em 3 partes: um **bootloader**
  pequeno no início, depois o firmware de **controle**, depois o
  firmware de **áudio**. Ver `shared/flash_layout.cmake` para os
  endereços exatos.
- O bootloader lê 1 setor de flash reservado (`shared/flash_map.h`)
  para saber qual foi o último modo que funcionou, e pula a execução
  para o firmware correspondente.
- Cada app, ao detectar que o host (Xbox ou PS5) realmente o aceitou,
  grava essa confirmação na flash (ver `shared/boot_confirm.h`).
- Se um app travar ou não for reconhecido em alguns segundos, um
  watchdog reinicia o Pico, e o bootloader tenta de novo / eventualmente
  alterna para o outro modo após 3 tentativas falhas seguidas.

## Primeiro boot ou troca de console

Na primeira vez, ou ao trocar de console pela primeira vez, pode levar
até ~25 segundos para o Pico "descobrir" o modo certo (algumas
tentativas + trocas). Depois disso, ele lembra e vai direto.
