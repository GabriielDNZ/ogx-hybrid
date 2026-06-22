# OGX-BT-Hybrid — Firmware Híbrido (Pico 2 W)

Este firmware faz o Pico 2 W funcionar como **controle de Xbox** (recebendo input de um controle pareado por Bluetooth) E como **placa de som USB** que retransmite áudio do PS5 por Bluetooth para uma caixa de som (ex: JBL) — tudo ao mesmo tempo, num único firmware.

Não precisa trocar de modo nem replugar: basta conectar no Xbox ou no PS5 que ele reconhece automaticamente.

## Como instalar no Pico 2 W

1. Baixe o arquivo `.uf2` na aba **Actions** do GitHub (clique no último workflow verde → role até **Artifacts** → baixe **OGX-BT-Hybrid-PICO2W**)

2. Desconecte o Pico de tudo

3. Segure o botão **BOOTSEL** do Pico e, ainda segurando, conecte o cabo USB no computador

4. Solte o botão — uma unidade chamada `RP2350` vai aparecer no computador, como um pendrive

5. Arraste o arquivo `.uf2` para dentro dessa unidade

6. O Pico vai gravar e reiniciar sozinho

Pronto — agora é só plugar no Xbox ou no PS5.
