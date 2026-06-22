# OGX_BT_Hybrid — Firmware para Raspberry Pi Pico 2W

Firmware híbrido que combina dois projetos em um único `.uf2`:

- **OGX-Mini**: recebe controle via Bluetooth e envia como XboxOG XID para o Xbox 360
- **PicoW-USB-BT-Audio**: recebe áudio USB do PS5 e transmite via Bluetooth para o JBL Tune Flex 2 (ou qualquer fone A2DP)

---

## Estrutura do ZIP

Este ZIP contém **apenas os arquivos customizados**. As dependências externas precisam ser clonadas separadamente (instruções abaixo).

```
CMakeLists.txt              ← Build principal híbrido
btstack_config.h            ← BTstack config combinado
tusb_config.h               ← TinyUSB config (referência; o real está em hybrid_config/)
FWDefines.cmake
pico_sdk_import.cmake
hybrid_config/
  tusb_config_hybrid.h      ← Config TinyUSB híbrido (XID + UAC2)
src/
  main_hybrid.c             ← main() dual-core
  pico_w_led.c / .h
  btstack/                  ← A2DP audio source (do PicoW-USB-BT-Audio)
  tinyusb/                  ← UAC2 device (do PicoW-USB-BT-Audio)
ogxmini_src/                ← Fontes do OGX-Mini + arquivos híbridos
  USBDevice/
    HybridDescriptors.h/cpp ← Descritores USB combinados (XID + UAC2)
  OGXMini/Board/
    PicoW_Hybrid.cpp        ← Split initialize_usb() / initialize_bluetooth()
  hybrid_wrappers.cpp       ← Wrappers C para funções C++ do OGX-Mini
  Bluepad32/Bluepad32.cpp   ← setup_only() adicionado para modo híbrido
external/patches/
  btstack_l2cap.diff        ← Patch do OGX-Mini no btstack l2cap
external/bluepad32/src/components/bluepad32/
  bt/uni_bt_setup.c         ← PATCHED: suporte a HYBRID_BT_ALREADY_INITIALIZED
  include/sdkconfig.h       ← Standalone (sem dependência de Board/Config.h)
```

---

## Como compilar

### 1. Pré-requisitos

```bash
# Instalar toolchain ARM + cmake
sudo apt install gcc-arm-none-eabi cmake ninja-build python3 git

# Clonar o Pico SDK 2.1.0
git clone --branch 2.1.0 --depth 1 \
  https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk
cd ~/pico-sdk && git submodule update --init --depth 1
```

### 2. Montar a estrutura de dependências

Crie uma pasta `hybrid/` e extraia o ZIP dentro dela. Depois:

```bash
cd hybrid/

# Criar pasta external/
mkdir -p external

# Clonar bluepad32 (o btstack interno é submodule)
git clone --depth 1 https://github.com/ricardoquesada/bluepad32.git \
  external/bluepad32
cd external/bluepad32
git submodule update --init --depth 1
cd ../..

# Aplicar o patch no btstack do bluepad32
cd external/bluepad32/external/btstack
git fetch --depth 50 origin 5d4d8cc7b1d35a90bbd6d5ffd2d3050b2bfc861c
git checkout 5d4d8cc7b1d35a90bbd6d5ffd2d3050b2bfc861c
# Aplicar patch l2cap
patch -p1 < ../../../../external/patches/btstack_l2cap.diff
cd ../../../..

# Clonar tinyusb (versão mais nova, usada como referência no external)
git clone --depth 1 https://github.com/hathach/tinyusb.git \
  external/tinyusb

# Clonar libfixmath
git clone --depth 1 https://github.com/PetteriAimonen/libfixmath.git \
  external/libfixmath

# Clonar Pico-PIO-USB (versão do OGX-Mini)
git clone --depth 1 https://github.com/wiredopposite/Pico-PIO-USB.git \
  external/Pico-PIO-USB

# Clonar OGX-Mini (para pegar os fontes em src/)
git clone --depth 1 https://github.com/wiredopposite/OGX-Mini.git /tmp/ogx
cp -r /tmp/ogx/Firmware/RP2040/src/* ogxmini_src/
# IMPORTANTE: sobrescrever com os arquivos patched do ZIP
# (os arquivos do ZIP já estão em ogxmini_src/ com as modificações)

# Clonar PicoW-USB-BT-Audio (para pegar 3rd-party/)
git clone --depth 1 https://github.com/wasdwasd0105/PicoW-usb2bt-audio.git \
  /tmp/btaudio
cp -r /tmp/btaudio/3rd-party ./
```

### 3. Copiar o uni_bt_setup.c patched

O arquivo `external/bluepad32/src/components/bluepad32/bt/uni_bt_setup.c`
que veio no ZIP **substitui** o original do bluepad32. Ele já está no lugar certo
se você extraiu o ZIP na pasta `hybrid/`.

### 4. Compilar

```bash
cd hybrid/
mkdir build && cd build
cmake .. -DPICO_SDK_PATH=~/pico-sdk -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

O arquivo `OGX_BT_Hybrid.uf2` será gerado em `build/`.

### 5. Gravar no Pico 2 W

1. Segure o botão **BOOTSEL** no Pico 2 W
2. Plugue o cabo USB no computador (aparece como pendrive `RPI-RP2`)
3. Arraste o `OGX_BT_Hybrid.uf2` para dentro

---

## Arquitetura

```
Core 0                          Core 1
──────────────────────────      ──────────────────────────
TinyUSB USB device              BTstack run loop
  ├─ Interface 0: XID           ├─ A2DP Source (áudio → JBL)
  │   (Xbox 360 controle)       └─ Bluepad32 HID
  └─ Interfaces 1+2: UAC2           (controle BT → Core0)
      (PS5 áudio input)
Timer 500µs: tinyusb_task()
Watchdog: 2s
```

## Notas

- O JBL Tune Flex 2 deve estar no **Modo Vídeo** (app JBL Headphones)
  para menor latência (~90-110ms)
- O controle Bluetooth precisa ser pareado uma vez após gravar o firmware
- Ao plugar no **Xbox 360**, o PS5 não reconhece o dispositivo (e vice-versa)
  — isso é o comportamento esperado: cada console vê apenas a interface que conhece
