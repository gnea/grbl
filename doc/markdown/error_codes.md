## Meanings of Grbl messages and error/alarm codes


#### _'error:' Codes_

Format - `(v1.0)` `:` `(v0.9)` - `Description`

- `error:1` : `error: Expected command letter` - G-code words consist of a letter and a value. Letter was not found.

- `error:2` : `error: Bad number format` - Numeric value format is not valid or missing an expected value.

- `error:3` : `error: Invalid statement` - Grbl '$' system command was not recognized or supported

- `error:4` : `error: Value < 0` - Negative value received for an expected positive value.

- `error:5` : `error: Setting disabled` - Homing cycle is not enabled via settings.

- `error:6` : `error: Value < 3 usec` - Minimum step pulse time must be greater than 3usec

- `error:7` : `error: EEPROM read fail. Using defaults` - EEPROM read failed. Reset and restored to default values.

- `error:8` : `error: Not idle` - Grbl '$' command cannot be used unless Grbl is IDLE. Ensures smooth operation during a job.

- `error:9` : `error: G-code lock` - G-code locked out during alarm or jog state

- `error:10` : `error: Homing not enabled` - Soft limits cannot be enabled without homing also enabled.

- `error:11` : `error: Line overflow` - Max characters per line exceeded. Line was not processed and executed.

- `error:12` : `error: Step rate > 30kHz`* - Grbl '$' setting value exceeds the maximum step rate supported.

- `error:13` : `error: Check Door` - Safety door detected as opened and door state initiated.

- `error:14` : `error: Line length exceeded` - (Grbl-Mega Only) Build info or startup line exceeded EEPROM line length limit.

- `error:15` : `error: Travel exceeded` - Jog target exceeds machine travel. Command ignored.

- `error:16` : `error: Invalid jog command` - Jog command with no '=' or contains prohibited g-code.

- `error:20` : `error: Unsupported command` - Unsupported or invalid g-code command found in block.

- `error:21` : `error: Modal group violation` - More than one g-code command from same modal group found in block.

- `error:22` : `error: Undefined feed rate` - Feed rate has not yet been set or is undefined.

- `error:23` : `error: Invalid gcode ID:23` - G-code command in block requires an integer value.

- `error:24` : `error: Invalid gcode ID:24` - More than one g-code command that requires axis words found in block.

- `error:25` : `error: Invalid gcode ID:25` - Repeated g-code word found in block.

- `error:26` : `error: Invalid gcode ID:26` - No axis words found in block for g-code command or mode which requires them.

- `error:27` : `error: Invalid gcode ID:27` - Line number value is invalid

- `error:28` : `error: Invalid gcode ID:28` - G-code command is missing a required value word.

- `error:29` : `error: Invalid gcode ID:29` - Work coordinate system commanded not supported.

- `error:30` : `error: Invalid gcode ID:30` - G53 only allowed during G0 and G1 motion modes.

- `error:31` : `error: Invalid gcode ID:31` - Axis words found in block while no command uses them.

- `error:32` : `error: Invalid gcode ID:32` - G2/3 arcs require at least one in-plane axis word.

- `error:33` : `error: Invalid gcode ID:33` - Motion command target is invalid.

- `error:34` : `error: Invalid gcode ID:34` - Arc radius value is invalid.

- `error:35` : `error: Invalid gcode ID:35` - G2/3 arcs require at least one in-plane offset word.

- `error:36` : `error: Invalid gcode ID:36` - Unused value words found in block.

- `error:37` : `error: Invalid gcode ID:37` - G43.1 dynamic tool length offset assigned to wrong axis.

`*` indicates feedback enabled only by compile-time option.

-----

#### 'Alarm:' Codes

Format - `(v1.0)` `:` `(v0.9)` - `Description`

- `ALARM:1` : `ALARM: Hard limit` - Hard limit has been triggered. Machine position is likely lost due to sudden stop. Re-homing is highly recommended.
`
- `ALARM:2` : `ALARM: Soft limit` - G-code motion target exceeds machine travel. Machine position safely retained. Alarm may be unlocked.

- `ALARM:3` : `ALARM: Abort during cycle` - Reset while in motion. Grbl cannot guarantee position. Lost steps are likely. Re-homing is highly recommended.

- `ALARM:4` : `ALARM: Probe fail` - If probe is not in the expected initial state before starting probe cycle, where G38.2 and G38.3 is not triggered and G38.4 and G38.5 is triggered.

- `ALARM:5` : `ALARM: Probe fail` - If the probe fails to contact the workpiece within the programmed travel for G38.2 and G38.4.

- `ALARM:6` : `ALARM: Homing fail` - If the active homing cycle was reset.

- `ALARM:7` : `ALARM: Homing fail` - If the safety door was opened during homing cycle.

- `ALARM:8` : `ALARM: Homing fail` - Pull off travel failed to clear limit switch. Try increasing pull-off setting or check wiring.

- `ALARM:9` : `ALARM: Homing fail` - Failed to find limit switch within travel. Defined as `1.5 * max_travel` on search and `5 * pulloff` on locate phases.

-----

#### Message Descriptions

Format - `Message` - `Description`

- `[Reset to continue]` - Critical event message. Reset is required before Grbl accepts any other commands. This prevents ongoing command streaming and risking a motion before the alarm is acknowledged. Hard or soft limit errors will trigger this event.

- `[‘$H’|’$X’ to unlock]`- Alarm message at initialization. All g-code commands and some ‘$’ are blocked until unlocked via homing or $X.

- `[Caution: Unlocked]` - Alarm unlock $X acknowledgement.

- `[Enabled]` - Indicates Grbl’s check-mode is enabled.

- `[Disabled]` - Indicates Grbl’s check-mode is disabled. Grbl is automatically reset afterwards.

- `[Check Door]` - Safety door detected as open. This message appears either immediately upon a safety door ajar or if the safety is open when Grbl initializes after a power-up/reset.

- `[Check Limits]` - If Grbl detects a limit switch is triggered after power-up/reset and hard limits are enabled, this will appear as a courtesy message.

- `[Pgm End]` - M2/30 program end message to denote g-code modes have been restored to defaults according to the M2/30 g-code description.

- `[Restoring defaults]` - Acknowledgement message when restoring EEPROM defaults via a `$RST=` command.
