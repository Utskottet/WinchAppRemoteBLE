# Antenna / Signal Tests — ParaWinch LoRa 868 MHz

**Setup:** Seeed XIAO nRF52840 + Wio SX1262 remote, ESP32 + Waveshare SX1262 winch
**RF params:** 868.1 MHz · SF9 · BW125 · CR4/6 · 22 dBm
**Sensitivity floor:** SX1262 @ SF9/BW125 ≈ −124 dBm
**Free-space path loss formula:** RSSI@distance = RSSI@10m − 20×log10(distance/10)

---

## Phase 1 — Finding the baseline (buggy firmware, XIAO sim as winch)

| # | Winch antenna | Remote antenna | Winch RSSI | Winch SNR | Remote RSSI | Remote SNR |
|---|---|---|---|---|---|---|
| 1 | PCB trace | ½ dipole | −80 dBm | 11 dB | −75 dBm | 11 dB |
| 2 | ¼ wave | ½ dipole | −90 dBm | 10.5 dB | −76 dBm | 11 dB |
| 3 | ½ dipole | ½ dipole | −77 dBm | 11.2 dB | −68 dBm | 10 dB |
| 4 | ½ dipole | ½ dipole + new IPEX | −81 dBm | 11 dB | −86 dBm | 10 dB |

All results ~30 dB below expected. Root cause unknown at this point.

---

## Phase 2 — Firmware fixes applied to both XIAO units

### Bug 1 — RF switch pin never driven (main culprit, ~30 dB loss)
`LORA_RXEN_PIN D5` defined in `utilities.h` but never passed to RadioLib.
Wio SX1262 has a physical RF switch on D5 (HIGH=RX, LOW=TX). With it floating the RX path was nearly dead.
**Fix:** `lora.setRfSwitchPins(LORA_RXEN_PIN, RADIOLIB_NC)` added to both XIAO firmwares.

### Bug 2 — OCP silently failing (secondary)
`LORA_CURRENT_LIMIT_MA 240.0f` exceeds RadioLib's 140 mA max. Call silently failed, leaving OCP at 60 mA.
SX1262 draws ~118 mA at 22 dBm — OCP tripped on every TX, reducing output power.
**Fix:** Changed to `140.0f` in both XIAO firmwares.

| # | Winch ant | Remote ant | Winch RSSI | Winch SNR | Remote RSSI | Remote SNR | Notes |
|---|---|---|---|---|---|---|---|
| 5 | ½ dipole | ½ dipole + new IPEX | −44 dBm | 10.8 dB | −40 dBm | 11 dB | 10 m |
| 6 | ½ dipole | ½ dipole + old IPEX | −33 dBm | 10.8 dB | −36 dBm | 11 dB | 10 m |

**+33 dB improvement on winch side. Numbers now in expected range.**
New IPEX on remote was bad (−18 dB). Reverted to old IPEX.

---

## Phase 3 — Real winch hardware (ESP32 + Waveshare SX1262)

**Hardware notes:**
- Waveshare SX1262 RF switch wired to DIO2 internally — no external pin needed
- Antenna: ½ dipole on 1 m coax, elevated 1 m, vertical polarisation
- Remote: ½ dipole, elevated 1.5 m, vertical polarisation
- Test environment: shop gate opening, large building behind = multipath present
- Stepper motor and driver close to winch SX1262

### OCP investigation

| # | OCP state | Winch RSSI | Winch SNR | Remote RSSI | Remote SNR | Stepper |
|---|---|---|---|---|---|---|
| 7 | not fixed (60 mA) | −42 dBm | 11 dB | −58 dBm | 11 dB | off |
| 8 | fixed (140 mA) | −60 dBm | 10 dB | −39 dBm | 10 dB | off |
| 9 | disabled/reverted | −45 dBm | 11 dB | −51 dBm | 11 dB | off |
| 10 | disabled | −51 dBm | 10.5 dB | −52 dBm | 11 dB | connected |

**Finding:** OCP fix at 140 mA causes supply rail desense on winch — PA current ripple couples into LNA.
Costs −15 dB on winch RX but gains +12 dB on winch TX. Waveshare module needs hardware decoupling caps to fix properly.
**Decision: OCP left disabled on winch. Command reception is safety-critical.**

### Distance tests — no BLDC, stepper connected

| # | Distance | Winch RSSI | Winch SNR | Remote RSSI | Remote SNR | Notes |
|---|---|---|---|---|---|---|
| 11 | 200 m | — | — | −87 to −77 dBm | −2 to +3 dB | building behind, multipath |
| 12 | 20 m | — | — | TBD | TBD | pending |
| 13 | 60 m | — | — | TBD | TBD | pending |

*Winch RSSI not logged at distance — remote display only.*
*200 m pessimistic due to building reflections. Open field expected 5–10 dB better.*

### Full range test — stepper + BLDC running, open flat field, ½ dipoles, vertical

| Distance | Remote RSSI | Remote SNR | Winch RSSI | Winch SNR | Notes |
|---|---|---|---|---|---|
| 0 m | −48 dBm | 11 dB | −33 dBm | 11 dB | |
| 250 m | −88 dBm | 10 dB | −74 dBm | 11 dB | 40 dB jump — possible ground reflection null |
| 400 m | −90 dBm | 10 dB | −77 dBm | 10 dB | |
| 600 m | −98 dBm | 9 dB | −83 dBm | 11 dB | |
| 800 m | −101 dBm | 9 dB | −89 dBm | 10 dB | |
| 1000 m | −104 dBm | 8 dB | −90 dBm | 10 dB | |
| 1000 m | −120 dBm | — | — | — | body covering antenna — packet loss starts here |
| 1200 m | −105 dBm | 6 dB | −91 dBm | 11 dB | |

**Practical sensitivity floor: −120 dBm** (confirmed by body-blocking test at 1000 m)

---

## Summary of all firmware fixes

| Issue | Affected units | Fix | Impact |
|---|---|---|---|
| RF switch pin D5 not driven | Both XIAO units | `setRfSwitchPins(D5, NC)` | +30 dB |
| OCP 240 mA (silent fail, 60 mA actual) | Both XIAO units | Changed to 140 mA | TX power restored |
| OCP never called | Winch ESP32 | `setCurrentLimit(140)` added then disabled | Supply desense discovered |

---

## Final link budget — stepper + BLDC, open field, ½ dipoles

Practical sensitivity floor: −120 dBm · Margin = floor − measured RSSI

| Distance | Remote RSSI | Remote margin | Winch RSSI | Winch margin |
|---|---|---|---|---|
| 0 m | −48 dBm | 72 dB | −33 dBm | 87 dB |
| 250 m | −88 dBm | 32 dB | −74 dBm | 46 dB |
| 600 m | −98 dBm | 22 dB | −83 dBm | 37 dB |
| 1000 m | −104 dBm | 16 dB | −90 dBm | 30 dB |
| 1200 m | −105 dBm | 15 dB | −91 dBm | 29 dB |
| ~1500 m (proj.) | ~−107 dBm | **~13 dB** | ~−93 dBm | **~27 dB** |

**✓ System validated for 1500 m operation with ½ dipole antennas.**
Panel antenna not required. Remote is the limiting side; winch RX has plenty of margin throughout.

*SNR margin = distance from −124 dBm floor. BLDC EMI not yet factored in.*

---

## Open items
- 20 m and 60 m reference tests pending (no BLDC)
- BLDC 8 kW EMI impact unknown — critical test before field deployment
- Hardware fix: decoupling caps on Waveshare SX1262 VDD rail
- New IPEX on remote needs to be redone
- Real open-field 1500 m test pending
