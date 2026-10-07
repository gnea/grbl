# Cartesian Robot Plate Transporter

This repository contains the DALSA-Lab firmware fork and robot client for a four-axis
laboratory plate transporter. It is based on GRBL, adapted for an Arduino Uno and an
X/Y/Z/A Cartesian motion system.

The A axis is integrated into the GRBL planner and stepper interrupt. The repository
also includes a Python robot client and a simulator.

## Project Documentation

- [Robot firmware, wiring, homing, and settings](README_ROBOT.md)
- [Python robot client](robot_client/README.md)
- [Firmware source](grbl/)

The robot setup documentation is the authoritative guide for the current hardware and
firmware configuration. The `doc/markdown/` and `doc/script/` directories retain inherited
GRBL references and utilities; some describe capabilities that are not part of this robot
firmware. Archived change logs document upstream history.

## Build

The firmware targets the ATmega328P used by the Arduino Uno. The root `Makefile` builds
`grbl.hex` with AVR-GCC; `make flash` uses the configured `avrdude` programmer. Confirm
the programmer and hardware configuration before flashing.

## Attribution and License

The DALSA-Lab fork and robot-specific fourth-axis changes are maintained on the
[`feature/fourth-axis` branch](https://github.com/DALSA-Lab/grbl/tree/feature/fourth-axis).

This project retains the original GRBL attribution and license: GRBL was developed by
Sungeun "Sonny" Jeon and is built on the earlier work of Simen Svale Skogsrud. GRBL is
free software under the GNU General Public License v3 or later; see [COPYING](COPYING)
and the copyright notices in the source files.
