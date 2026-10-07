# Cartesian Robot Plate Transporter Firmware

This is the DALSA-Lab GRBL-CRPT fork of GRBL for the Cartesian Robot Plate Transporter.

Copyright (c) 2026 Alan Mansour, DALSA. The fork is maintained at
[DALSA-Lab/grbl](https://github.com/DALSA-Lab/grbl).
Original GRBL copyright notices and the GPL license are retained; see [COPYING](COPYING).

This repository is GRBL-CRPT firmware (a GRBL fork) for a laboratory Cartesian robot with:

- X, Y, and Z linear stepper axes
- A rotary stepper axis for table-side orientation
- A future gripper motor controlled by a DRV8871
- Four limit switches
- Arduino Uno / ATmega328P target

The A axis is integrated into the normal GRBL-CRPT planner and stepper interrupt. It is not a separate blocking rotation routine.

## Hardware Pin Map

Connect Arduino signal ground to the signal ground of every motor driver.

### Stepper drivers

| Axis | STEP | DIR |
|---|---:|---:|
| X | D2 | D6 |
| Y | D3 | D7 |
| Z | D4 | D8 |
| A | D5 | D9 |

Motor power must come from suitable external supplies and drivers. Do not power stepper motors from Arduino pins.

### Limit switches

| Axis | Arduino input |
|---|---:|
| X | A0 |
| Y | A1 |
| Z | A2 |
| A | A3 |

The firmware uses the analog pins as digital inputs. All four limit inputs belong to the PORTC pin-change interrupt group.

### Limit-switch wiring

The intended wiring is normally closed (NC):

```text
Arduino input ---- NC switch ---- GND
       |
       +---- pull-up to +5 V
```

The firmware enables the Arduino internal pull-ups. An external 4.7k-10k pull-up may be used if the switch wiring is long or noisy.

Expected electrical states:

| State | Arduino input | GRBL-CRPT logical state |
|---|---|---|
| Switch closed, normal | LOW | Not triggered |
| Switch open, limit reached | HIGH | Triggered |
| Wire broken | HIGH | Triggered |

The active configuration uses `$5=1` for this polarity. Do not add an additional A-only inversion unless measurements prove that the A stage has opposite electrical polarity.

### Control inputs

| Function | Arduino pin |
|---|---:|
| Reset | D10 |
| Feed hold | D12 |
| Cycle start | D13 |

A physical emergency stop must remove motor-driver power independently of the Arduino. It may also be wired into the Arduino reset circuit, but firmware reset alone is not an emergency-stop system.

### Gripper reservation

The gripper is currently only a placeholder and is not active in firmware.

Reserved future pins:

| Function | Pin |
|---|---:|
| DRV8871 PWM / IN1 | D11 |
| DRV8871 IN2 | D10 or fixed LOW, to be finalized |
| Encoder channel A | A4 |
| Encoder channel B | A5 |

Do not connect the encoder to A4/A5 until the gripper module is implemented and tested separately.

The DRV8871 must be connected to the motor according to its datasheet. Arduino pins must connect only to its logic inputs, not to the motor supply or motor output terminals.

## Homing

The homing order is:

1. Z
2. A
3. X and Y together

This is configured in `grbl_crpt/config.h`:

```c
#define HOMING_CYCLE_0 (1<<Z_AXIS)
#define HOMING_CYCLE_1 (1<<A_AXIS)
#define HOMING_CYCLE_2 ((1<<X_AXIS)|(1<<Y_AXIS))
```

The homing direction mask is `$23`:

```text
X = 1
Y = 2
Z = 4
A = 8
```

For example:

```text
$23=7   ; invert X, Y, and Z homing directions
$23=15  ; invert X, Y, Z, and A homing directions
```

In this firmware, a set `$23` bit homes that axis toward its negative direction.
`$28` selects which homed switch is the maximum-coordinate end, using the same
axis bits (X=1, Y=2, Z=4, A=8). For negative-end X/Y switches and positive-end
Z/A switches, use `$28=12`. With `$130=$131=470`, `$132=200`, `$133=360`, and
2-unit pull-offs, the post-home coordinates are approximately X=2, Y=2, Z=198,
A=358. If A homes in the negative direction instead, clear its bit in `$28`
(use `$28=4`) so its pulled-off coordinate remains inside 0-360 degrees.

Pull-off distance is configurable per axis. X/Y/Z values are in mm; A is in
degrees. `$27` remains a compatibility setting that changes X/Y/Z together;
use `$140` through `$143` for individual X/Y/Z/A values.

Normal movement direction is controlled separately by `$3`:

```text
$3=8   ; invert normal A movement direction only
```

Always test direction at low rates before running `$H`.

## Important Settings

### Axis calibration

```text
$100  X steps/mm
$101  Y steps/mm
$102  Z steps/mm
$103  A steps/degree
```

For A:

```text
steps_per_degree = motor_steps_per_revolution * microsteps * gear_ratio / 360
```

Do not leave `$103=200` unless that is correct for the A motor, driver microstepping, and gearing.

### Speed and acceleration

```text
$110  X maximum rate, mm/min
$111  Y maximum rate, mm/min
$112  Z maximum rate, mm/min
$113  A maximum rate, degrees/min

$120  X acceleration, mm/s^2
$121  Y acceleration, mm/s^2
$122  Z acceleration, mm/s^2
$123  A acceleration, degrees/s^2
```

### Travel and homing

```text
$130  X maximum travel, mm
$131  Y maximum travel, mm
$132  Z maximum travel, mm
$133  A maximum travel, degrees

$21   hard limits
$22   homing enable
$23   homing direction mask
$24   homing feed rate
$25   homing seek rate
$26   debounce delay
$27   legacy X/Y/Z homing pull-off distance
$28   homing maximum-end mask

$140  X homing pull-off, mm
$141  Y homing pull-off, mm
$142  Z homing pull-off, mm
$143  A homing pull-off, degrees
```

After changing compile-time defaults or uploading a firmware revision with changed settings, restore EEPROM defaults:

```text
$RST=$
```

This erases the current GRBL-CRPT settings and reloads the firmware defaults. Reapply calibrated values afterward.

## Arduino IDE Setup

1. Install Arduino IDE 2.
2. Install the Arduino AVR Boards package.
3. Copy or synchronize this repository's `grbl_crpt` source files into:

```text
%USERPROFILE%\OneDrive - Danmarks Tekniske Universitet\Dokumenter\Arduino\libraries\grbl_crpt
```

4. Open the GRBL-CRPT upload example:

```text
grbl_crpt/examples/crptUpload/crptUpload.ino
```

5. Select the correct Arduino Uno board and COM port.
6. Compile before connecting motors.
7. Upload only after the compile succeeds.

Arduino IDE compiles the library copy under the Arduino `libraries` directory. Changes made only in the VS Code workspace are not necessarily used by Arduino IDE.

## Initial Test Procedure

Keep motor power low or disconnected during the first signal tests.

### 1. Check firmware startup

Open a serial terminal at 115200 baud. Expected startup text includes:

```text
GRBL-CRPT 1.0.0 ['$' for help]
```

### 2. Inspect settings

```text
$$
```

Confirm `$103`, `$113`, `$123`, `$133`, `$21`, `$22`, `$23`, `$27`, `$28`, and `$140`-`$143`.

### 3. Test limit logic

Send:

```text
?
```

The report should include `Pn:A` only when the A switch is physically triggered. The other limit letters are `X`, `Y`, and `Z`.

A diagnostic command may be enabled during development:

```text
$A
```

It reports the normalized A limit state used by the homing logic:

```text
[A_LIMIT:0]
[A_LIMIT:1]
```

### 4. Test individual axis motion

Unlock only if the machine is physically safe:

```text
$X
```

Use absolute positioning and small moves:

```gcode
G90
G0 X5
G0 X0
G0 Y5
G0 Y0
G0 Z5
G0 Z0
G0 A5
G0 A0
```

Stop immediately if an axis moves in an unexpected direction.

### 5. Test homing

First use conservative values:

```text
$21=0
$22=1
$24=10
$25=50
$27=3
```

Verify that no limit is falsely active, then send:

```text
$H
```

Keep a hand near the Arduino reset button and use a separate emergency-stop circuit for the motor power.

## Current Scope and Limitations

- Spindle and coolant support have been removed from this robot-specific fork.
- The gripper is only a placeholder.
- A is a planner-integrated fourth axis measured conceptually in degrees.
- There is no encoder feedback for A or the gripper yet.
- The Arduino Uno has limited SRAM, flash, timers, and interrupt bandwidth.

## Development Rules

- Keep workspace files and the Arduino library copy synchronized.
- Compile in Arduino IDE after each firmware change.
- Test one axis at a time before coordinated motion.
- Never test homing with a motor direction that has not been verified.
- Never rely on serial commands as an emergency stop.
