# ParaWinch Battery Pack

## Overview

| Parameter | Value |
|---|---|
| Configuration | 20s8p (4 modules × 10s, 8 cells per group) |
| Total cells | 160 (40 per module) |
| Cell | LG INR18650 MH1 |
| Nominal voltage | 72V (3.6V × 20) |
| Max charge voltage | 84V (4.2V × 20) |
| Nominal capacity | 25.6 Ah (3200mAh × 8p) |
| Nominal energy | ~1844 Wh |
| Max continuous discharge | 80A (10A × 8p) |
| Charger | 84V 10A |

## Cell Specifications — LG INR18650 MH1

| Parameter | Value |
|---|---|
| Chemistry | INR (Li-NiMnCo) |
| Nominal capacity | 3200 mAh (min 3100 mAh) |
| Nominal voltage | 3.67V |
| Max charge voltage | 4.20V ± 0.05V |
| Cutoff voltage | 2.5V |
| Standard charge | CC 0.5C (1550 mA) / CV 4.2V / cutoff 50 mA |
| Max charge current | 1C (3100 mA) |
| Max discharge current | 10A continuous |
| Size | ø18.4 × 65.1 mm |
| Weight | 49 g |
| Datasheet | LG PS-CY-INR18650MH1 |

Compatible replacement: LG INR18650 MJ1 (3500 mAh, 10A) — same form factor, slightly higher capacity.

## Pack Structure

Four modules connected in series. Each module is 10s with 8 cells per series group (8p). Total: 20s8p, 160 cells.

```
[Module 1] — [Module 2] — [Module 3] — [Module 4]
  10s×8p       10s×8p       10s×8p       10s×8p
  40 cells     40 cells     40 cells     40 cells
```

Each module has balance taps at every series group. Nickel strip welded, 0.3 mm strip on series connections, 0.15 mm acceptable for parallel connections within groups.

### Module 3 — Cell Group Layout

Tap numbers visible on pack:
- Top side: + − 2 4 6 8
- Bottom side: 1 3 5 7 9

Series groups numbered 1–10 from − to +. Each group is 8p (4 cells visible on top side, 4 on bottom side). Adjacent groups share nickel strips at the series boundary.

## BMS

### Hardware
- Ant BMS 10S-24S, Double PCBA variant
- Product page: https://antbms.vip/product/ant-bms-10s-24s-50a-180a-smart-32v-88v-lifepo4-li-ion-lto-battery-protection-board-double-pcba/
- Balance method: Passive, 100 mA
- Balance connector: 21 wires (B0–B20), JST-type connector
- Current sense shunt range: 440A
- Bluetooth built-in (Android/iOS app)
- App: "ANT BMS" — tabs: State, Parameter, Set, BMS Control, Mine
- Admin password: unknown (default passwords 123456/000000/888888/666666 did not work)

### Operating Mode
- Charge-only configuration — BMS does not disconnect on discharge
- Discharge path bypasses BMS entirely — no undervoltage protection during discharge

### Configured Settings (2026-04-28)

| Setting | Value | Notes |
|---|---|---|
| Battery type | Li-ion | |
| Cell count | 20s | |
| Pack capacity | 26 Ah | 8p × 3.2Ah |
| Unit overvoltage protection | 4.20V | Cuts charge |
| Unit overvoltage recovery | 4.15V | Resumes charge |
| Lv2 unit overvoltage protection | 4.30V | Emergency backup |
| Lv2 unit overvoltage recovery | 4.20V | |
| Total overvoltage protection | 84.0V | Was 103V default! |
| Unit diff protection | 1.0V | Temporarily raised from 0.03V to clear fault — set back to 0.03V once cells balanced |
| Unit diff recovery | 0.020V | |
| Balance start voltage | 4.05V | |
| Charge unit balance start voltage | 4.05V | Was 4.10V default |
| Unit diff on voltage | 0.020V | |
| Charge overcurrent protection | 15A | Charger is 10A |
| Charge high temp protection | 45°C | Per MH1 datasheet |
| Charge high temp recovery | 40°C | |
| MOSFET high temp protection | 75°C | Default, leave as is |

### BMS Limitations Discovered

- Passive balance at 100 mA cannot close large voltage gaps (>200 mV) without overheating the balance circuit (reached 59°C)
- BMS latches "unit diff protection" fault when cell difference exceeds threshold — does not auto-clear on parameter change, requires power cycle or clearing protection log
- "Balance limit" warning blocks balancing when gap is too large — BMS treats it as fault, not a condition to fix
- In charge-only mode, balancing may only activate when charger is connected
- To clear stuck fault: use "clear protect time" in BMS Control tab, or power cycle BMS
- Factory default settings are dangerously permissive (total OVP at 103V, unit diff at 1V, charge OC at 50A)

## Protection & Power Path

- 500A KILOVAC LEV200 contactor
- Precharge: 100Ω 25W resistor (permanent passive snubber)
- 300A ANL fuse
- QS10 connectors

## Incident Log

### 2026-04-28 — Module 3 Group 10 Failure (Overcharge)

**Root cause:** Module 4 positive power lead was disconnected (missed during previous maintenance). BMS balance leads for all 20s were still connected. Charger pushed 20s charge voltage (84V) through only 15 series groups (modules 1+2+3). This resulted in approximately 5.6V per cell — far above the 4.2V maximum.

**Failure:** 4 of 8 cells in group 10 of module 3 (between tap 9 and + terminal) were destroyed. All 4 dead cells read 0V — internal short from severe overcharge. The remaining 4 cells in the group survived but were pulled to a lower voltage (~3.5V) by the dead cells.

**Diagnosis steps:**
1. Full pack + to − read 0V
2. Modules 1, 2, 4 verified healthy
3. Module 3: groups 1–9 all reading 4.0V per group (36V total from − to 9)
4. Group 10 (9 to +): 0V
5. Removed nickel strip on + side of group 10
6. All 4 cells in the dead half read 0V — confirmed destroyed

**Repair:**
- Replaced 4 dead cells with new LG INR18650 MH1 (sourced from Electrokit, 54 SEK/cell)
- New cells arrived at ~3.6–3.7V storage voltage
- Old 0.3 mm nickel strip reused for series connection to group 9
- New 0.15 mm nickel strip for parallel connections within the replacement group
- Spot weld settings: 9 ms pulse at 7.5V for 0.3 mm strip, 7 ms for 0.15 mm strip

**Post-repair state:**
- 19 groups at ~4.07V, group 10 at ~3.65V (average of 4 new cells at ~3.6V + 4 surviving cells at ~3.5V)
- BMS entered "unit diff protection" fault due to 400 mV gap
- BMS refused to charge or balance — fault latched
- Balance circuit overheated (59°C) attempting to close gap at 100 mA passive
- Resolution: external bench PSU charge of group 10 through balance connector pins (B19 and B20), 4.0V / 300 mA until group reaches ~3.9–4.0V, then BMS can handle remaining balance

**Key correction:** Pack is 20s8p (not 20s4p as initially assumed). Each group has 8 cells, not 4. The 4 replaced cells are paralleled with 4 surviving cells in group 10.

### Pre-Charge Safety Checklist

Before every charge:
- [ ] Physically verify all 4 module positive power leads are connected (tug test)
- [ ] Measure each module voltage across power terminals (not balance leads) — each should read ~40V
- [ ] Measure full string voltage across main power terminals — should be ~72–84V depending on SOC
- [ ] If full string reads ~60V (15s), a module power lead is disconnected — do not charge
- [ ] Check BMS app State tab — all 20 cells should show voltage, no fault flags

**Important:** Measuring full string voltage alone is NOT sufficient — BMS balance leads can show voltage from cells even when the power path is disconnected. Always verify power lead connections physically or measure across individual module power terminals.

### Post-Incident TODO

- [ ] External bench PSU charge group 10 to ~4.0V via balance pins B19/B20
- [ ] Reconnect BMS and verify fault clears
- [ ] Set unit diff protect back to 0.03V once cells balanced
- [ ] First full charge: monitor cell temps and BMS balance status
- [ ] Capacity test module 3 vs modules 1/2/4 — overcharged groups may have reduced cycle life
- [ ] Consider ordering 4 more MH1 cells as spares

## Supplier Notes

| Part | Source | Notes |
|---|---|---|
| LG INR18650 MH1 | Electrokit (SE) | 54 SEK incl. VAT, in stock |
| LG INR18650 MH1 | Nkon.nl | Alternative, fast EU shipping |
| 0.15 mm nickel strip | — | Sufficient for 8p at 10A/cell |
| 0.3 mm nickel strip | — | Use for series connections, harder to spot weld (needs 9ms+) |
| Ant BMS 10S-24S | antbms.vip | $59–90 USD depending on current rating |
