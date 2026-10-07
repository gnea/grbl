# GRBL-CRPT — Cartesian Robot Plate Transporter

This repository contains the DALSA-Lab firmware fork and robot client for a four-axis
laboratory plate transporter. GRBL-CRPT is a fork of GRBL (gnea/grbl), adapted for an Arduino Uno and an
X/Y/Z/A Cartesian motion system.

The A axis is integrated into the GRBL-CRPT planner and stepper interrupt. The repository
also includes a Python robot client and a simulator.

Current GRBL-CRPT release: **1.0.0**.

## Project Documentation

- [Robot firmware, wiring, homing, and settings](README_ROBOT.md)
- [Python robot client](robot_client/README.md)
- [Firmware source](GRBL-CRPT/)

The robot setup documentation is the authoritative guide for the current hardware and
firmware configuration. The `doc/markdown/` and `doc/script/` directories retain inherited
GRBL references and utilities; some describe capabilities that are not part of this robot
firmware. Archived change logs document upstream history.

## Build

The firmware targets the ATmega328P used by the Arduino Uno. The root `Makefile` builds
`grbl_crpt.hex` with AVR-GCC; `make flash` uses the configured `avrdude` programmer. Confirm
the programmer and hardware configuration before flashing.

## Attribution and License

GRBL-CRPT is a fork of [GRBL](https://github.com/gnea/grbl), maintained by DALSA-Lab at
[DALSA-Lab/grbl](https://github.com/DALSA-Lab/grbl).

Copyright (c) 2026 Alan Mansour, DALSA (GRBL-CRPT modifications: fourth axis, gripper
support, robot client and simulator, and removal of spindle, probe and coolant support)

Original GRBL:

- Copyright (c) 2012-2017 Sungeun K. Jeon for Gnea Research LLC
- Copyright (c) 2009-2011 Simen Svale Skogsrud

The firmware is free software under the GNU General Public License v3 or later; see
[COPYING](COPYING) and the copyright notices in the source files. The Arduino example
sketches in `GRBL-CRPT/examples/` are under the MIT license; see the `license.txt` in each
example folder.