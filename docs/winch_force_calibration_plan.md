# Paragliding Winch Force Calibration Plan

## Goal

Build a reliable calibration table/model for the electric paragliding winch so we understand:

- motor current → line pull
- drum diameter → force multiplication
- line-out amount → estimated drum diameter
- safe current limits for different tow settings
- safe state values in VESC Lisp
- what values the ESP32 should send over CAN
- what telemetry should be logged/verified

Final goal: a practical calculator/table that lets us set tow strength safely without needing to plug USB into the VESC and manually edit motor config or Lisp state values.

---

## Current System

### Hardware

- Motor: QS138 90H, 72 V / 4000 W family
- Motor plate marking / serial: `SIA72V4000WM11021`
- Battery: 22S
- VESC: VESC 100/250
- External drive ratio:
  - motor sprocket/pulley: 22T
  - drum sprocket/pulley: 90T
  - ratio = `90 / 22 = 4.09:1`
- Drum diameter:
  - full drum: 250 mm
  - empty drum/core: 120 mm
- Winch line length:
  - total line length: 1500 m
  - normal launch starts with about 800–1300 m line out
  - release usually happens in the air before the drum is empty
  - at release, remaining line on drum is often around half of the start amount
  - example: start with 800–1300 m out, release may happen after pulling in roughly 400–600 m

### Control

- ESP32 communicates with VESC over CAN.
- VESC has LispBM script running.
- ESP32 sends CAN state commands.
- Lisp receives state and calls `set-current`.
- Current Lisp maps states to fixed current values.
- Proposed improvement:
  - ESP32 sends one adjustable “max tow current” or “tow setting”
  - Lisp derives:
    - state 5 = max × 0.8
    - state 6 = max × 1.0
  - Lisp clamps all values before using them
  - VESC motor config remains the hard safety ceiling

---

## Important Realization

A fixed current does **not** mean fixed line pull.

Line pull changes directly with drum radius/diameter.

Same motor torque/current gives more pull on a smaller drum.

### Drum Force Ratio

```text
full drum diameter = 250 mm
empty drum diameter = 120 mm

force ratio = 250 / 120 = 2.083
```

So if a current gives 80 kg at full drum, it gives roughly:

```text
80 × 2.083 = 166.7 kg
```

at empty drum.

This is the main danger.

---

## Full Drum Calibration Question

Earlier estimate/measurement suggested:

```text
~140 A motor current ≈ ~80 kg line pull at full 250 mm drum
```

Independent motor/drivetrain sanity check using likely QS138 data:

```text
approx Kv = 4000 rpm / 72 V = 55.6 rpm/V
Kt ≈ 9.55 / 55.6 = 0.172 Nm/A

motor torque at 140 A:
140 × 0.172 = 24.1 Nm

drum torque:
24.1 × 4.09 = 98.6 Nm

full drum radius:
250 / 2 = 125 mm = 0.125 m

line force:
98.6 / 0.125 = 789 N

kg pull:
789 / 9.81 = 80.4 kg
```

So 140 A ≈ 80 kg at full drum is plausible.

But this must be verified with real load testing.

---

## Static Test Method

Available tool: crane scale.

This is enough for static calibration.

Setup:

```text
Winch line -> crane scale -> fixed rated anchor
```

Test should be done with:

- full drum first
- short pulls only
- no people in line with the rope
- rated anchor, shackles, straps, hooks
- remote operation / stand to the side
- start low and step upward

Suggested test points:

```text
40 A
60 A
80 A
100 A
120 A
140 A
```

Optional finer points later:

```text
50 A
70 A
90 A
110 A
130 A
150 A
```

For each point record:

```text
commanded current A
actual motor current A
battery current A
battery voltage V
ERPM
mechanical RPM if available
duty cycle %
MOSFET temp
motor temp
faults
crane scale peak kg
drum diameter mm
line-out estimate
ambient temperature
```

Important: crane scale peak value is useful, but avoid long static stalls.

---

## What Static Test Gives Us

Static test gives the real relationship:

```text
motor current -> line pull
```

for a known drum diameter.

This is enough to build a safe current table.

It does not fully prove dynamic towing force, but it gives the base torque/pull calibration.

---

## Dynamic Effects Not Fully Captured by Static Test

During real towing, additional factors matter:

- drum is rotating
- line speed changes
- pilot accelerates
- drum diameter changes continuously
- battery voltage sags
- VESC may become voltage/duty limited
- motor and MOSFET temperature rise
- line angle and friction may change
- tow release happens before drum is empty

Without a live load cell, dynamic pull must be calculated/estimated from motor current and drum diameter, not directly measured.

So the calculator can be very good, but it should still be validated conservatively in real use.

---

## Core Force Model

If we know line pull at full drum for a given current:

```text
pull_at_current_drum =
pull_at_full_drum × full_diameter / current_diameter
```

For current limiting:

```text
allowed_current =
full_drum_current × current_diameter / full_diameter
```

For this winch:

```text
allowed_current =
full_drum_current × current_diameter / 250
```

Example if 145 A is safe at full drum:

```text
250 mm drum -> 145 A
200 mm drum -> 116 A
160 mm drum -> 93 A
120 mm drum -> 70 A
```

---

## Required Calculator / Spreadsheet Tabs

### 1. Constants

```text
motor model
motor plate marking / serial
battery voltage / cells
gear ratio
full drum diameter
empty drum diameter
line length
line diameter
drum width if needed
VESC hard current limit
VESC battery current limit
VESC ERPM limit
state factors
safety margin
```

### 2. Static Pull Test Data

Columns:

```text
test number
date
drum diameter mm
line out m
commanded current A
actual motor current A
battery current A
battery voltage V
ERPM
duty %
MOSFET temp C
motor temp C
crane scale kg
notes
```

### 3. Calibration Curve

Use measured data to fit:

```text
pull_kg = a × current_A + b
```

or if better:

```text
pull_kg = a × current_A
```

Then compare measured vs predicted.

### 4. Drum Diameter / Line-Out Model

Need to estimate drum diameter based on line remaining on drum.

Known:

```text
total line length = 1500 m
start line out usually = 800–1300 m
release after pulling in roughly 400–600 m
full drum diameter = 250 mm
empty drum diameter = 120 mm
```

This tab estimates:

```text
line remaining on drum
current drum diameter
current drum radius
force multiplier
```

### 5. Current Limit Table

For each tow setting:

```text
target pull kg
equivalent full-drum current
current diameter
diameter compensation
allowed current
state 5 current = allowed × 0.8
state 6 current = allowed × 1.0
```

### 6. Dynamic Log

For later real tow logging:

```text
time
state
commanded current
actual motor current
battery current
battery voltage
ERPM
duty
estimated drum diameter
estimated line speed
estimated line pull
MOSFET temp
motor temp
faults
```

### 7. Safety Settings

List:

```text
VESC motor current max
VESC absolute max current
VESC battery current max
VESC ERPM max
Lisp max current clamp
CAN timeout behavior
max allowed temperature
max allowed duty
max allowed estimated pull
```

---

## ESP32 / Lisp Architecture

### ESP32 Should Handle

- user display
- pilot weight / tow class / max setting
- maybe convert UI value to target current
- send CAN command to VESC Lisp
- receive accepted value back
- show accepted value, not just requested value
- log telemetry if possible

### VESC Lisp Should Handle

- receive CAN state command
- receive max tow current or tow setting
- clamp received values
- derive state 5 and state 6
- ramp current
- timeout to safe state
- send accepted values back
- optionally apply drum compensation if diameter/line-out is available

### VESC Config Should Handle

Hard limits only:

```text
motor current max
battery current max
ERPM max
temperature limits
voltage limits
absolute max current
```

Do not rewrite VESC config during normal use.

---

## Proposed State Logic

Fixed lower states:

```text
state 0 = 0 A
state 1 = fixed low current
state 2 = fixed low/medium current
state 3 = fixed medium current
state 4 = fixed high-ish current
```

Variable high states:

```text
state 5 = max_tow_current × 0.8
state 6 = max_tow_current × 1.0
```

But `max_tow_current` must be compensated for drum diameter if we want constant line pull.

---

## Guardrails

Guardrails are checks that prevent bad input from becoming dangerous motor output.

Required:

```text
1. VESC config hard ceiling
2. Lisp clamps CAN values
3. Lisp accepts only valid states
4. Lisp timeout if ESP/CAN command disappears
5. Lisp ramps current changes
6. Lisp sends accepted max value back to ESP
7. ESP displays accepted value
8. telemetry verifies actual current / ERPM / temp
```

Example fault logic:

```text
CAN timeout -> state 0 / current 0
invalid state -> ignore or state 0
requested max current above clamp -> clamp down
actual current too high too long -> cut
ERPM too high -> reduce/cut
motor/MOSFET temp too high -> reduce/cut
```

---

## Key Open Questions / Data Still Needed

### From VESC Tool

Need to record:

```text
motor resistance
motor inductance
flux linkage / lambda
pole pairs
detected sensors / hall status
motor current max
absolute max current
battery current max
ERPM max
temperature limits
```

### During Static Test

Need to log:

```text
actual motor current
battery current
battery voltage
ERPM
duty
MOSFET temp
motor temp
faults
crane scale kg
drum diameter
```

### Mechanical / Line Model

Need:

```text
line diameter
drum width
how evenly the line winds
whether drum diameter can be estimated from line-out
whether get-dist is accurate enough
```

---

## Practical Next Step

1. Set up crane scale static test at full drum.
2. Test 40, 60, 80, 100, 120, 140 A.
3. Log VESC realtime values for each point.
4. Build first spreadsheet:
   - current vs kg at full drum
   - drum compensation
   - safe state 5/state 6 table
5. Then modify Lisp and ESP32:
   - ESP sends max tow/current setting
   - Lisp clamps and derives state 5/6
   - Lisp sends accepted value back
6. Later add drum diameter compensation using line-out estimate.

---

## Main Safety Conclusion

If full drum 140 A is around 80 kg, then the same 140 A at empty drum is roughly:

```text
80 × 250 / 120 = 166.7 kg
```

So fixed current states are unsafe across the full drum range.

The winch needs either:

```text
A) current compensation by drum diameter / line-out
```

or

```text
B) conservative current limits based on smallest drum diameter
```

or best:

```text
C) direct live load cell feedback
```

For now, with only a crane scale, static calibration + drum compensation is the practical route.
