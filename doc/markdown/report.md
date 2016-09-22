### _Grbl v1.0 Realtime Status Reports_ (Rev. 2)

--------

#### Summary of Changes from Grbl v0.9 Reports

- Intent of changes is to make parsing cleaner, reduce transmitting overhead without effecting overall Grbl performance, and add more feedback data, which includes three new override values and real-time velocity.

- Data fields are separated by `|` pipe delimiters, rather than `,` commas that were used to separate data values. This should help with parsing.

- The ability to mask and add/remove data fields from status reports via the `$10` status report mask setting has been disabled. Only selecting `MPos:` or `WPos:` coordinates is allowed.
  - All available data is always sent to standardize the reports across all GUIs.
  - For unique situations, data fields can be removed by config.h macros, but it is highly recommended to not alter these.


- `MPos:` OR `WPos:` are always included in a report, but not BOTH at the same time.

  - This reduces transmit overhead tremendously by removing upwards to 40 characters.
  - `WCO:0.000,10.000,2.500` A current work coordinate offset is now sent to easily convert between position vectors, where `WPos = MPos - WCO` for each axis.
    - `WCO:` is included immediately whenever a `WCO:` value changes or intermittently after every **X** status reports as a refresh. Refresh rates can dynamically vary from 10 to 30 (configurable) reports depending on what Grbl is doing.
    - `WCO:` is simply the sum of the work coordinate system, G92, and G43.1 tool length offsets.
    - Basically, a GUI just needs to retain the last `WCO:` and apply the equation to get the other position vector.
    - `WCO:` messages may only be disabled via a config.h compile-option, if a GUI wants to handle the work position calculations on its own to free up more transmit bandwidth.
  - Be aware of the following issue regarding `WPos:`.
    - In Grbl v0.9 and prior, there is an old outstanding bug where the `WPos:` work position reported may not correlate to what is executing, because `WPos:` is based on the g-code parser state, which can be several motions behind. Grbl v1.0 now forces the planner buffer to empty, sync, and stops motion whenever there is a command that alters the work coordinate offsets `G10,G43.1,G92,G54-59`. This is the simplest way to ensure `WPos:` is always correct. Fortunately, it's exceedingly rare that any of these commands are used need continuous motions through them.
    - A compile-time option is available to disable the planner sync and forced stop, but, if used, it's up to the GUI to handle this position correlation issue.


- The `Hold` and `Door` states includes useful sub-state info via a `:` colon delimiter and an integer value. See descriptions for details.

- Limit and other input pin reports have significantly changed to reduce transmit overhead.
  - The data type description is now just `Pn:`, rather than `Lim:000` or `Pin:000|0|0000`
  - It does not appear if no inputs are detected as triggered.
  - If an input is triggered, ```Pn:``` will be followed by a letter or set of letters of every triggered input pin. `XYZPDHRS` for the XYZ-axes limits, Probe, Door, Hold, soft-Reset, cycle Start pins, respectively.
  - For example, a triggered Z-limit and probe pin would report `Pn:ZP`.


- Buffer data (planner and serial RX) reports have been tweaked and combined.

  - `Bf:0,0`. The first value is planner blocks in use and the second is RX bytes in use.


- Override reports are intermittent since they don't change often once set.

  - Overrides are included in every 10 or 20 status reports (configurable) depending on what Grbl is doing or, if an override value or toggle state changes, automatically in the next report.
  - There are two override fields:
    - `Ov:100,100,100` Organized as feed, rapid, and spindle speed overrides in percent.
    - `T:SFM` with each letter `S`, `F`, and `M` are defined as spindle stop active, flood coolant toggled, and mist coolant toggled, respectively.



- Line numbers, when enabled in config.h, are omitted when:

  - No line number is passed to Grbl in a block.
  - Grbl is performing a system motion like homing, jogging, or parking.
  - Grbl is executing g-code block that does not contain a motion, like `G20G54` or `G4P1` dwell. (NOTE: Looking to fixing this later.)

-------

#### Basic Characteristics

- Contains real-time data of Grbl’s state, position, and other data required independently of the stream.

- Categorized as a real-time message, where it is a separate message that should not be counted as part of the streaming protocol. It may appear at any given time.

- A status report is initiated by sending Grbl a '?' character.

  - Like all real-time commands, the '?' character is intercepted and never enters the serial buffer. It's never a part of the stream and can be sent at any time.

  - Grbl will generate and transmit a report within ~5-20 milliseconds.

  - Every ’?’ command sent by a GUI is not guaranteed with a response. The following are the current scenarios when Grbl may not immediately or ignore a status report request. _NOTE: These may change in the future and will be documented here._

    - If two or more '?' queries are sent before the first report is generated, the additional queries are ignored.

    - A soft-reset commanded clears the last status report query.

    - When Grbl throws a critical alarm from a limit violation. A soft-reset is required to resume operation.

    - During a homing cycle.

#### Message Construction:

  - A message is a single line of ascii text, completed by a carriage return and line feed.

  - `< >` Chevrons uniquely enclose reports to indicate message type.

  - `|` Pipe delimiters separate data fields inside the report.

  - The first data field is an exception to the following data field rules. See 'Machine State' description for details.

  - All remaining data fields consist of a data type followed by a `:` colon delimiter and data values. `type:value(s)`

  - Data values are given either as as one or more pre-defined character codes to indicate certain states/conditions or as numeric values, which are separated by a `,` comma delimiter when more than one is present. Numeric values are also in a pre-defined order and units of measure.

  - The first (Machine State) and second (Current Position) data fields are always included in every report.

  - Assume any following data field may or may not exist and can be in any order. The `$10` status report mask setting can alter what data is present and certain data fields can be reported intermittently (see descriptions for details.)

  - The `$13` report inches settings alters the units of some data values. `$13=0` false indicates mm-mode, while `$13=1` true indicates inch-mode reporting. Keep note of this setting and which report values can be altered.

- _Data Field Descriptions:_

    - **Machine State:**

      - Valid states types:  `Idle, Run, Hold, Jog, Alarm, Door, Check, Home`

      - Sub-states may be included via `:` a colon delimiter and numeric code.

      - Current sub-states are:

        - `Hold:0` Hold complete. Ready to resume.

        - `Hold:1` Hold in-progress. Reset will throw an alarm.

        - `Door:0` Door closed. Ready to resume.

        - `Door:1` Machine stopped. Door still ajar. Can't resume until closed.

        - `Door:2` Door opened. Hold (or parking retract) in-progress. Reset will throw an alarm.

        - `Door:3` Door closed and resuming. Restoring from park, if applicable. Reset will throw an alarm.

      - This data field is always present as the first field.

    - **Current Position:**

        - Depending on `$10` status report mask settings, position may be sent as either:

          - `MPos:0.000,-10.000,5.000` machine position or

          - `WPos:-2.500,0.000,11.000` work position

        - Three position values are given in the order of X, Y, and Z. A fourth position value may exist in later versions for the A-axis.

        - `$13` report inches user setting effects these values and is given as either mm or inches.

        - This data field is always present as the second field.

    - **Work Coordinate Offset:**

        - `WCO:0.000,1.551,5.664` is the current work coordinate offset of the g-code parser, which is the sum of the current work coordinate system, G92 offsets, and G43.1 tool length offset.

        - Machine position and work position are related by this simple equation per axis: `WPos = MPos - WCO`

        - Values are given in the order of the X,Y, and Z axes offsets. A fourth offset value may exist in later versions for the A-axis.
        - `$13` report inches user setting effects these values and is given as either mm or inches.

        - `WCO:` values don't change often during a job once set and only requires intermittent refreshing.

        - This data field appears:

          - In every 10 or 30 (configurable 1-255) status reports, depending on if Grbl is in a motion state or not.

          - Immediately in the next report, if an offset value has changed.

          - In the first report after a reset/power-cycle.

        - This data field will not appear if:

          - It is disabled in the config.h file. No `$` mask setting available.

          - The refresh counter is in-between intermittent reports.       

    - **Buffer State:**

        - `Bf:0,0`. The first value is planner blocks in use and the second is RX bytes in use.

        - This data field will not appear if:

          - It is disabled by the `$` status report mask setting.

    - **Line Number:**

        - `Ln:99999` indicates line 99999 is currently being executed. This differs from the `$G` line `N` value since the parser is usually queued few blocks behind execution.

        - Compile-time option only because of memory requirements. However, if a GUI passes indicator line numbers onto Grbl, it's very useful to determine when Grbl is executing them.

        - This data field will not appear if:

          - It is disabled in the config.h file. No `$` mask setting available.

          - The line number reporting not enabled in config.h. Different option to reporting data field.

          - No line number or `N0` is passed with the g-code block.

          - Grbl is homing, jogging, parking, or performing a system task/motion.

          - There is no motion in the g-code block like a `G4P1` dwell. (May be fixed in later versions.)

    - **Current Rate:**

        - `F:1000.` indicates current actual feed rate (speed) of the executing motion. Depending on machine max rate settings and acceleration, this value may not be the programmed rate.

        - Value units, either in mm/min or inches/min, is dependent on the `$` report inches user setting.

        - As a operational note, reported rate is typically 30-50 msec behind actual position reported.

        - This data field will not appear if:

          - It is disabled in the config.h file. No `$` mask setting available.

    - **Input Pin State:**

        - `Pn:XYZPDHRS` indicates which input pins Grbl has detected as 'triggered'.

        - Pin state is evaluated every time a status report is generated. All input pin inversions are appropriately applied to determine 'triggered' states.

        - Each letter of `XYZPDHRS` denotes a particular 'triggered' input pin.

          - `X Y Z` XYZ limit pins, respectively

          - `P` the probe pin.

          - `D H R S` the door, hold, soft-reset, and cycle-start pins, respectively.

          - Example: `Pn:PZ` indicates the probe and z-limit pins are 'triggered'.

          - Note: `A` may be added in later versions for an A-axis limit pin.

        - Assume input pin letters are presented in no particular order.

        - One or more 'triggered' pin letter(s) will always be present with a `Pn:` data field.

        - This data field will not appear if:

          - It is disabled in the config.h file. No `$` mask setting available.

          - No input pins are detected as triggered.

    - **Override Values:**

        - `Ov:100,100,100` indicates current override values in percent of programmed values for feed, rapids, and spindle speed, respectively.

        - Override values don't change often during a job once set and only requires intermittent refreshing. This data field appears:

          - After 10 or 20 (configurable 1-255) status reports, depending on is in a motion state or not.

          - If an override value has changed, this data field will appear immediately in the next report. However, if `WCO:` is present, this data field will be delayed one report.

          - In the second report after a reset/power-cycle.

        - This data field will not appear if:

          - It is disabled in the config.h file. No `$` mask setting available.

          - The override refresh counter is in-between intermittent reports.

          - `WCO:` exists in current report during refresh. Automatically set to try again on next report.

    - **Toggle Overrides:**

      - `T:SFM` indicates a toggle override is in effect or has been commanded.

      - Like the pin state field, each letter denotes a particular toggle override.

        - `S` indicates the spindle stop toggle override is in effect. It will appear as long as the spindle stop override is active.

        - `F` indicates the flood coolant toggle override was activated. It will only appear once after it has executed the coolant state change.

        - `M` indicates the mist coolant toggle override was activated, if mist coolant is enabled via config.h. It will only appear once after it has executed the coolant state change.

      - Assume toggle override letters are presented in no particular order.

      - One or more active toggle override letter(s) will always be present with a `T:` data field.

      - This data field appears:

        - If a toggle override is active or has recently executed and only when the override values field is also present (see override value field rules).

      - This data field will not appear if:

        - If no toggle override is active or has been executed.

        - It is disabled in the config.h file. No `$` mask setting available.

        - If override refresh counter is in-between intermittent reports.

        - `WCO:` exists in current report during refresh. Automatically set to try again on next report.
