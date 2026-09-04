# TowMonitor V0
## Development & Prototype Specification

**Revision:** 0.1  
**Date:** 18 August 2026  
**Status:** Prototype definition / parts selected  

## 1. Purpose

TowMonitor V0 is a compact battery-powered sensor package for development of the next-generation paraglider tow system.

The unit combines tow-force measurement with flight-state sensing in one nRF52840-based node.

### Primary measurements

- Tow-line tension
- Airspeed
- Barometric altitude
- Vertical speed / vario
- Acceleration
- Angular motion / orientation

### Communications

- **V0:** Bluetooth Low Energy
- **Later:** 868 MHz LoRa using an SX1262 module

---

## 2. System Architecture

```text
WH-C100 load cell
      |
      v
Adafruit NAU7802
      |
      +------------------+
                         |
Adafruit BMP581 ---------+---- I2C ---- XIAO nRF52840 Sense
                         |
Sensirion SDP810 --------+
      |
      v
Pitot / static tubing

XIAO nRF52840 Sense
  |- nRF52840
  |- BLE
  |- onboard 6-axis IMU
  |- USB-C
  `- 1S LiPo charging
```

External digital sensors share:

```text
3V3
GND
SDA
SCL
```

The load-cell bridge connects locally to the NAU7802.

---

## 3. Selected Hardware

| Function | Selected part | Interface | Prototype form | Status |
|---|---|---|---|---|
| MCU / BLE | Seeed XIAO nRF52840 Sense | I2C / BLE / GPIO | Development board | Selected |
| IMU | Onboard XIAO 6-axis IMU | Internal | Integrated | Selected |
| Force sensor | WH-C100 150 kg internal load cell | 4-wire bridge | Bare extracted cell | Selected / characterize |
| Load-cell ADC | Adafruit NAU7802 STEMMA QT | I2C | Breakout + screw terminal | Selected |
| Vario / altitude | Adafruit BMP581 STEMMA QT | I2C | Breakout | Selected |
| Airspeed | Sensirion SDP810-500Pa | I2C | Through-hole sensor | Selected |
| Pitot plumbing | Pitot/static probe + silicone tube | Pressure | External | To select |
| Battery | Protected 1S LiPo ~500 mAh | Battery | Pouch cell | Target |
| Long-range radio | SX1262 868 MHz module | SPI | Module | Deferred |

---

## 4. Component Notes

### 4.1 XIAO nRF52840 Sense

Selected as the main controller because it combines:

- nRF52840 MCU
- BLE
- onboard 6-axis IMU
- USB-C
- LiPo charging
- enough GPIO for later SX1262 LoRa

The three external digital sensors all share the same I2C bus, so the sensor package uses very few MCU pins.

### 4.2 WH-C100 Load Cell + NAU7802

The lightweight load cell is extracted from a WH-C100 150 kg hanging scale.

Prototype work:

1. Identify the four bridge wires.
2. Connect to NAU7802.
3. Determine zero offset.
4. Calibrate against known loads.
5. Measure repeatability and creep.
6. Verify dynamic response.
7. Check temperature drift.

Mechanical mounting must load the cell in its intended axis without significant bending or side-loading.

### 4.3 BMP581 Vario / Altitude

The Adafruit BMP581 breakout is selected for easy STEMMA QT / Qwiic wiring.

Firmware will calculate:

- pressure
- altitude
- filtered vertical speed

The sensor should sample static ambient pressure and must be protected from direct ram airflow.

### 4.4 SDP810-500Pa Airspeed Sensor

Selected instead of the tiny SDP31 for the prototype because the SDP810 has mechanically useful tube connections while retaining high sensitivity at low differential pressure.

Key characteristics:

- differential pressure range: **±500 Pa**
- digital I2C interface
- proper tubing connections
- through-hole mounting
- suitable for pitot/static airspeed measurement

Connection:

```text
Pitot total pressure ---- SDP810 high-pressure port
Static pressure --------- SDP810 low-pressure port
```

Use short flexible silicone tubes and provide strain relief.

---

## 5. Electrical Prototype

V0 should be hand-wired on a small perfboard or carrier plate.

### Shared I2C bus

```text
XIAO 3V3 ----+---- NAU7802
             +---- BMP581
             `---- SDP810

XIAO GND ----+---- NAU7802
             +---- BMP581
             `---- SDP810

XIAO SDA ----+---- NAU7802
             +---- BMP581
             `---- SDP810

XIAO SCL ----+---- NAU7802
             +---- BMP581
             `---- SDP810
```

The NAU7802 and BMP581 can use STEMMA QT cables.

The SDP810 can simply use four soldered wires:

- VDD
- GND
- SDA
- SCL

### Battery

Target prototype battery:

**Protected 1S LiPo, approximately 500 mAh**

Charge through the XIAO USB-C interface.

---

## 6. Firmware Data Model

Minimum V0 telemetry:

```text
force_N
force_kgf

differential_pressure_Pa
airspeed_mps

barometric_pressure_Pa
altitude_m
vario_mps

accelerometer_x
accelerometer_y
accelerometer_z

gyro_x
gyro_y
gyro_z

battery_voltage

sensor_status
fault_flags
```

All sensor values should be timestamped so force, airspeed, vario and IMU data can later be correlated.

---

## 7. Prototype Development Sequence

### V0.1 — Bench assembly

Connect:

- XIAO
- NAU7802
- BMP581
- SDP810
- LiPo

Confirm all I2C devices operate together.

### V0.2 — Load-cell characterization

- Extract WH-C100 load cell
- identify bridge wiring
- tare
- calibrate against known loads
- test repeatability and dynamic response

### V0.3 — Vario

Implement:

- pressure acquisition
- pressure filtering
- altitude calculation
- vertical-speed calculation

### V0.4 — Airspeed

Install pitot/static probe and tubing.

Implement:

- differential pressure zeroing
- pressure filtering
- airspeed calculation
- calibration against a known/reference speed

### V0.5 — Combined telemetry

Combine and timestamp:

- force
- airspeed
- altitude
- vario
- IMU
- battery voltage

Send over BLE.

### V0.6 — Mechanical packaging

Provide:

- tube strain relief
- static-pressure protection
- load-cell mechanical alignment
- battery retention
- sensor enclosure

### V0.7 — Tow / flight testing

Compare recorded data against expected tow behaviour and tune filtering.

### V1 — Custom PCB

Only after V0 works reliably:

- integrate bare sensor ICs where useful
- reduce wiring
- add SX1262 868 MHz LoRa
- optimize battery monitoring
- optimize enclosure and connectors

---

## 8. Deferred / Open Items

- Exact pitot/static probe geometry
- Exact silicone-tube dimensions
- Exact LiPo capacity
- LoRa module and antenna implementation
- Final enclosure
- Whether NAU7802 and BMP581 remain as modules or become bare ICs on V1 PCB
- Characterization of the WH-C100 internal load cell

---

## 9. V0 Summary

The intended first prototype is:

```text
XIAO nRF52840 Sense
+
Adafruit NAU7802
+
WH-C100 load cell
+
Adafruit BMP581
+
Sensirion SDP810-500Pa
+
pitot/static probe
+
1S ~500 mAh LiPo
```

This gives one compact sensor node measuring:

**tow force + airspeed + altitude + vario + acceleration + angular motion**

without requiring a custom PCB for the first development version.
