# ParaWinch — Claude Code Context

## Repositories
- **Remote + sim:** https://github.com/Utskottet/WinchAppRemoteBLE (this repo)
- **Winch controller:** https://github.com/Utskottet/868-ParaWinch

## Hardware
| Unit | MCU | LoRa module |
|---|---|---|
| Remote | Seeed XIAO nRF52840 | Wio SX1262 (SKU 102010710), B2B connector |
| Winch | ESP32 (M5Stack) | Waveshare SX1262 (Core1262-868M) |

## RF Configuration (must match on all nodes)
- 868.1 MHz · SF9 · BW125 kHz · CR4/6 · 22 dBm
- OCP: 140 mA on XIAO/remote, disabled on winch (see known issues)

## LoRa Pin Mapping
**XIAO remote + XiaoWinchSim:**
```
CS=D4, DIO1=D1, BUSY=D3, RST=D2, RXEN=D5
SPI: SCK=D8, MISO=D9, MOSI=D10
```
**Winch ESP32:**
```
NSS=5, DIO1=34, RST=13, BUSY=26, SCK=18, MISO=19, MOSI=23
```

## Packet Protocol
```
0x01  CmdPacket     remote → winch   (type, seq, state 0-6)
0x02  MetricsPacket winch → remote   (type, seq, lastCmdSeq, amps_x10, distance_m, vesc_mV, lineState)
0x03  AckPacket     winch → remote   (type, seq, pad, pad)
```

## Critical Firmware Fixes (already applied — do not revert)

### 1. RF switch pin — XIAO/Wio SX1262 only (the big one, ~30 dB recovered)
The Wio SX1262 module has a physical RF switch on pin D5 (HIGH=RX, LOW=TX).
It was defined in `utilities.h` but never used. RX path was nearly dead without it.
```cpp
lora.setRfSwitchPins(LORA_RXEN_PIN, RADIOLIB_NC);  // in setup(), after begin()
```

### 2. OCP silent failure — both XIAO units
`setCurrentLimit(240.0f)` silently fails in RadioLib (max is 140 mA).
OCP stayed at 60 mA default, tripping on every TX at 22 dBm.
Fixed: `LORA_CURRENT_LIMIT_MA 140.0f` in `lorastruct.h`.

### 3. Waveshare SX1262 (winch) — RF switch
Waveshare Core1262-868M wires DIO2 directly to the RF switch on-module.
`setDio2AsRfSwitch(true)` is called automatically by RadioLib `begin()`.
No external pin needed — do NOT add `setRfSwitchPins()` to winch firmware.

### 4. Winch OCP — supply desense issue (known hardware problem)
Adding `setCurrentLimit(140 mA)` on the winch improves TX by ~12 dB but
degrades winch RX by ~15 dB due to PA current ripple on the supply rail.
Fix requires hardware: decoupling caps on Waveshare SX1262 VDD pin.
`setCurrentLimit()` is currently commented out in `868-ParaWinch/src/LoRaComm.cpp`.

## Signal Quality Reference (10 m bench test, fixed firmware)
| Config | Winch RX | Remote RX |
|---|---|---|
| Stepper only, OCP disabled | −51 dBm | −52 dBm |
| BLDC running, OCP disabled | −39 dBm | −49 dBm |

BLDC does not degrade signal — stepper driver is the noisier load.
Full test log: `antenna_tests.md` in this repo.

## Link Budget — Target 1500 m
- Path loss 10 m → 1500 m: +43.5 dB
- Est. RSSI at 1500 m (BLDC running): ~−83 dBm winch RX, ~−93 dBm remote RX
- Sensitivity floor SF9/BW125: −124 dBm
- Margin: ~41 dB winch side, ~31 dB remote side
- Panel antenna not needed — ½ dipole sufficient in open flat field

## Project Structure
```
WinchAppRemoteBLE/
├── Xiao Firmware/     XIAO nRF52840 remote (BLE + LoRa)
├── XiaoWinchSim/      Diagnostic: simulates winch LoRa side, prints RSSI/SNR to serial
├── ItsyBitsy Firmware/
├── antenna_tests.md   Full signal test log with all findings and firmware fix history
└── CLAUDE.md          This file
```

## Open Items
- Hardware: decoupling caps on Waveshare SX1262 VDD to fix supply desense
- Hardware: new IPEX connector on remote needs resoldering (old one is better)
- Field test at 1500 m in open terrain pending
- BLDC EMI at longer range not yet measured (10 m test was fine)
