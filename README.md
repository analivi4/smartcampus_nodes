# Smart Campus Nodes — UFC Quixadá

Firmware para três nós IoT de monitoramento de salas, desenvolvidos com **Raspberry Pi Pico + módulo LoRa RFW95**, usando **FreeRTOS**.

## Nós

| Diretório | Função | Payload uplink | Downlink |
|-----------|--------|----------------|----------|
| `no1_temp/` | Temperatura (DHT11) + controle AC via IR | `T\|25.3\n` | `0x31` liga / `0x30` desliga |
| `no2_relay/` | Estado e controle de relé | `R\|1\n` | `0x31` liga / `0x30` desliga |
| `no3_occup/` | Contagem bidirecional de pessoas (VL53L0X × 2) | `O\|3\n` | — |

## Pinos comuns (LoRa SX1276)

| Sinal | GPIO |
|-------|------|
| NSS   | 17   |
| SCK   | 18   |
| MOSI  | 19   |
| MISO  | 16   |
| RESET | 15   |
| DIO0  | 20   |
| DIO1  | 21   |

## Pinos por nó

| Nó | Sinal | GPIO |
|----|-------|------|
| no1 | DHT11 | 22 |
| no1 | IR TX | 3 |
| no2 | Relé | 2 |
| no3 | I2C SDA | 4 |
| no3 | I2C SCL | 5 |
| no3 | XSHUT_A | 8 |
| no3 | XSHUT_B | 7 |

## LoRaWAN

- Ativação: **ABP** | Região: **AU915 FSB2**
- Gateway: RadioEnge | Servidor: lorawan-server Gotthardp

## Build

```bash
export PICO_SDK_PATH=~/pico-sdk
export FREERTOS_KERNEL_PATH=~/FreeRTOS-Kernel
export PICO_LORAWAN_PATH=~/pico-lorawan

mkdir build && cd build
cmake .. && make -j4
```

Os binários `.uf2` ficam em `build/no1_temp/`, `build/no2_relay/` e `build/no3_occup/`.
