# OGX-Mini + USB-BT-Audio — Firmware Híbrido (Pico 2 W)

Este firmware faz o Pico 2 W decidir sozinho, ao ligar, se ele deve se
comportar como **controle de Xbox** (recebendo input de um controle
pareado por Bluetooth) ou como **placa de som USB** que retransmite
áudio do PS5 por Bluetooth para uma caixa de som (ex: JBL).

Não existe botão para trocar de modo: o Pico tenta o último modo que
funcionou. Se não for reconhecido em alguns segundos, troca sozinho e
tenta o outro. Depois de algumas trocas, aprende e vai direto no modo
certo.

## Como instalar no Pico 2 W

1. Baixe o arquivo `OGX-Mini-Hybrid-PICO2W.uf2` na aba **Actions** do
   GitHub (clique no último workflow verde → role até **Artifacts** →
   baixe **OGX-Mini-Hybrid-PICO2W**)

2. Desconecte o Pico de tudo

3. Segure o botão **BOOTSEL** do Pico e, ainda segurando, conecte o
   cabo USB no computador

4. Solte o botão — uma unidade chamada `RP2350` vai aparecer no
   computador, como um pendrive

5. Arraste o arquivo `OGX-Mini-Hybrid-PICO2W.uf2` para dentro dessa
   unidade

6. O Pico vai gravar e reiniciar sozinho

Pronto — agora é só plugar no Xbox ou no PS5.

## Primeiro uso

Na primeira vez, ou ao trocar de console pela primeira vez, pode levar
até ~25 segundos para o Pico descobrir o modo certo. Depois disso ele
lembra e vai direto.
