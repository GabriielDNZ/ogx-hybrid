# OGX-Mini + USB-BT-Audio — Firmware Híbrido (Pico 2 W)

Este projeto faz o Pico 2 W decidir sozinho, ao ligar, se ele deve se
comportar como **controle de Xbox** (recebendo input de um controle
pareado por Bluetooth) ou como **placa de som USB** que retransmite
áudio do PS5 por Bluetooth para uma caixa de som (ex: JBL).

Não existe um botão para trocar de modo: o Pico tenta o último modo
que funcionou da última vez; se não for reconhecido em alguns
segundos, ele troca para o outro modo sozinho e tenta de novo. Depois
de algumas trocas, ele aprende e passa a ir direto no modo certo.

## Como gerar o arquivo .uf2 final (sem instalar nada no seu computador)

Você só precisa de uma conta gratuita no GitHub. O próprio GitHub vai
compilar o firmware para você, na nuvem.

### Passo 1 — Criar uma conta no GitHub (se ainda não tiver)
Acesse https://github.com/signup e crie uma conta gratuita.

### Passo 2 — Criar um novo repositório com este código
1. Acesse https://github.com/new
2. Dê um nome ao repositório (ex: `meu-ogx-hibrido`)
3. Deixe como **Public** (necessário para o plano gratuito de Actions)
4. Clique em **Create repository**
5. Na página do repositório recém-criado, clique no link **"uploading
   an existing file"** (ou vá em Add file → Upload files)
6. Arraste TODAS as pastas e arquivos deste projeto (bootloader/,
   app-controller/, app-audio/, scripts/, shared/, .github/,
   .gitmodules) para a área de upload
7. Role para baixo e clique em **Commit changes**

   **Importante:** o upload pela interface web do GitHub não
   inicializa submódulos automaticamente. Se after o upload o
   workflow falhar reclamando de "submodule" vazio, peça ajuda a
   alguém com Git instalado para rodar, dentro da pasta do projeto:
   ```
   git init
   git add .
   git submodule add https://github.com/ricardoquesada/bluepad32.git app-controller/Firmware/external/bluepad32
   git submodule add https://github.com/hathach/tinyusb.git app-controller/Firmware/external/tinyusb
   git submodule add https://github.com/wiredopposite/Pico-PIO-USB.git app-controller/Firmware/external/Pico-PIO-USB
   git submodule add https://github.com/PetteriAimonen/libfixmath.git app-controller/Firmware/external/libfixmath
   git commit -m "projeto hibrido"
   git remote add origin <URL do seu repositorio>
   git push -u origin main
   ```

### Passo 3 — Deixar o GitHub compilar
1. No seu repositório, clique na aba **Actions** (no topo da página)
2. Você verá um workflow chamado **"Build Hybrid OGX-Mini +
   USB-BT-Audio"** rodando automaticamente (foi disparado pelo upload)
3. Clique nele e aguarde — leva alguns minutos. Você verá 3 etapas
   rodando em paralelo (bootloader, controller, audio), seguidas de
   uma etapa "combine" que junta tudo
4. Quando tudo ficar verde (✓), role até o final da página e baixe o
   arquivo em **Artifacts** chamado **`OGX-Mini-Hybrid-PICO2W`**
5. Dentro do .zip baixado está o arquivo `OGX-Mini-Hybrid-PICO2W.uf2`
   — esse é o firmware final, pronto para gravar no Pico

### Passo 4 — Gravar no Pico 2 W
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

## Limitações conhecidas / riscos

Este firmware foi escrito e revisado com cuidado, mas **não pôde ser
compilado nem testado em hardware real durante o desenvolvimento**
(ambiente de desenvolvimento sem acesso à internet/toolchain ARM). É
possível que o primeiro build no GitHub Actions revele algum erro de
compilação que precise de um pequeno ajuste. Se isso acontecer, copie
a mensagem de erro completa da aba Actions — ela ajuda a identificar
exatamente o que corrigir.
