# ParaWinch

Paragliding winch control system. The winch controller communicates with a handheld remote over 868 MHz LoRa. The remote also connects via BLE to a phone app for monitoring and configuration.

**Phone app:** https://utskottet.github.io/parawinch/

## Repository structure

| Folder | Description | Hardware |
|--------|-------------|----------|
| `winch/` | Winch controller firmware | M5Stack Core (ESP32) |
| `remote/` | Handheld remote firmware | Seeed XIAO nRF52840 + Wio SX1262 |
| `sim/` | LoRa simulator — mimics winch transmissions for development without the winch | Seeed XIAO nRF52840 + Wio SX1262 |
| `archive/itsy-bitsy/` | Old test firmware, not in use | Adafruit ItsyBitsy |
| `docs/` | System specifications and notes | — |

## Communication

```
Phone  <── BLE ──>  Remote (Xiao nRF52840)  <── 868 MHz LoRa ──>  Winch (M5Stack ESP32)
```

## Docs

- [Antenna tests](docs/antenna_tests.md)
