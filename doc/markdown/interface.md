# Grbl Interface Basics

The interface for Grbl is fairly simple and straightforward. With Grbl v1.1, steps have been taken to try to make it even easier for new users to get started, and for GUI developers to write their own custom interfaces to Grbl.

In short, Grbl communicates through the serial interface on the Arduino. You just need to connect your Arduino to your computer with a USB cable. Use any standard serial terminal program to connect to Grbl, such as: the Arduino IDE serial monitor, Coolterm, puTTY, etc. Or use one of the many great Grbl GUIs out there in the Internet wild.

Just about every user interaction with Grbl is performed by sending it a string of characters, followed by a carriage return. Grbl will then process the string, execute it accordingly, and then reply back with a response message to tell you how it went. These strings include sending Grbl: a G-code block to execute, commands to configure Grbl's system settings, to view how Grbl is doing, etc. At times, Grbl may not respond immediately. This happens only when Grbl is busy doing something else, or waiting for some room to clear in its look-ahead planner buffer so it can finish processing the previous line sent.

In other words, both commands sent to Grbl and messages received from Grbl have a format of a single line of characters, terminated by a return. To provide more feedback on what Grbl is doing, additional messages may be "pushed" from Grbl to the user in response to a query or to let the user know something important just happened. Finally, an exception to the command and response interface are Grbl's real-time commands. These commands are single, special characters that may be sent to Grbl at any time to immediately alter or report what its doing and do not have a response message. See the [realtime commands] document to see what they are and how they work.


#### Start Up Message

**`Grbl X.Xx ['$' for help]`**

The start up message always prints upon startup and after a reset. Whenever you see this message, this also means that Grbl has completed re-initializing all its systems, so everything starts out the same every time you use Grbl.

* `X.Xx` indicates the major version number, followed by a minor version letter. The major version number indicates the general release, while the letter simply indicates a feature update or addition from the preceding minor version letter.
* Bug fix revisions are tracked by the build info version number, printed when an `$I` command is sent. These revisions don't update the version number and are given by date revised in year, month, and day, like so `20161014`.

#### Grbl `$` Help Message

Every string Grbl receives is assumed to be a G-code block/line for it to execute, except for some special system commands Grbl uses for configuration, provide feedback to the user on what and how it's doing, or perform some task such as a homing cycle. To see a list of these system commands, type `$` followed by an enter, and Grbl will respond with:

```
[HLP:$$ $# $G $I $N $x=val $Nx=line $J=line $SLP $C $X $H ~ ! ? ctrl-x]
```

- _**NOTE:** Grbl v1.1's new override real-time commands are not included in the help message. They use the extended-ASCII character set, which are not easily type-able, and require a GUI that supports them. This is for two reasons: Establish enough characters for all of the overrides with extra for later growth, and prevent accidental keystrokes or characters in a g-code file from enacting an override inadvertently._

* Check out our [Configuring Grbl](https://github.com/grbl/grbl/wiki/Configuring-Grbl-v0.9) wiki page to find out what all of these commands mean and how to use them.


---------

# Grbl Response Messages

Every G-code block sent to Grbl and Grbl `$` system command that is terminated with a return will be parsed and processed by Grbl. Grbl will then respond either if it recognized the command with an `ok` line or if there was a problem with an `error` line.

* **`ok`**: All is good! Everything in the last line was understood by Grbl and was successfully processed and executed.

  - If an empty line with only a return is sent to Grbl, it considers it a valid line and will return an `ok` too, except it didn't do anything.


* **`error:X`**: Something went wrong! Grbl did not recognize the command and did not execute anything inside that message. The `X` is given as a numeric error code to tell you exactly what happened. The table below decribes every one of them.

	| ID | Error Code Description |
|:-------------:|----|
| **`1`** | G-code words consist of a letter and a value. Letter was not found. |
| **`2`** | Numeric value format is not valid or missing an expected value. |
| **`3`** | Grbl '$' system command was not recognized or supported. |
| **`4`** | Negative value received for an expected positive value. |
| **`5`** | Homing cycle is not enabled via settings. |
| **`6`** | Minimum step pulse time must be greater than 3usec |
| **`7`** | EEPROM read failed. Reset and restored to default values. |
| **`8`** | Grbl '$' command cannot be used unless Grbl is IDLE. Ensures smooth operation during a job. |
| **`9`** | G-code locked out during alarm or jog state |
| **`10`** | Soft limits cannot be enabled without homing also enabled. |
| **`11`** | Max characters per line exceeded. Line was not processed and executed. |
| **`12`** | (Compile Option) Grbl '$' setting value exceeds the maximum step rate supported. |
| **`13`** | Safety door detected as opened and door state initiated. |
| **`14`** | (Grbl-Mega Only) Build info or startup line exceeded EEPROM line length limit. |
| **`15`** | Jog target exceeds machine travel. Command ignored. |
| **`16`** | Jog command with no '=' or contains prohibited g-code. |
| **`20`** | Unsupported or invalid g-code command found in block. |
| **`21`** | More than one g-code command from same modal group found in block.|
| **`22`** | Feed rate has not yet been set or is undefined. |
| **`23`** | G-code command in block requires an integer value. |
| **`24`** | Two G-code commands that both require the use of the `XYZ` axis words were detected in the block.|
| **`25`** | A G-code word was repeated in the block.|
| **`26`** | A G-code command implicitly or explicitly requires `XYZ` axis words in the block, but none were detected.|
| **`27`**| `N` line number value is not within the valid range of `1` - `9,999,999`. |
| **`28`** | A G-code command was sent, but is missing some required `P` or `L` value words in the line. |
| **`29`** | Grbl supports six work coordinate systems `G54-G59`. `G59.1`, `G59.2`, and `G59.3` are not supported.|
| **`30`**| The `G53` G-code command requires either a `G0` seek or `G1` feed motion mode to be active. A different motion was active.|
| **`31`** | There are unused axis words in the block and `G80` motion mode cancel is active.|
| **`32`** | A `G2` or `G3` arc was commanded but there are no `XYZ` axis words in the selected plane to trace the arc.|
| **`33`** | The motion command has an invalid target. `G2`, `G3`, and `G38.2` generates this error, if the arc is impossible to generate or if the probe target is the current position.|
| **`34`** | A `G2` or `G3` arc, traced with the radius definition, had a mathematical error when computing the arc geometry. Try either breaking up the arc into semi-circles or quadrants, or redefine them with the arc offset definition.|
| **`35`** | A `G2` or `G3` arc, traced with the offset definition, is missing the `IJK` offset word in the selected plane to trace the arc.|
| **`36`** | There are unused, leftover G-code words that aren't used by any command in the block.|
| **`37`** | The `G43.1` dynamic tool length offset command cannot apply an offset to an axis other than its configured axis. The Grbl default axis is the Z-axis.|


----------------------

# Grbl Push Messages

Along with the response message to indicate successfully executing a line command sent to Grbl, Grbl provides additional push messages for important feedback of its current state or if something went horribly wrong. These messages are "pushed" from Grbl and may appear at anytime. They are usually in response to a user query or some system event that Grbl needs to tell you about immediately. These push messages are organized into five general classes:

- **_ALARM messages_** - Means an emergency mode has been enacted and shut down normal use.

- **_'$' settings messages_** - Contains the type and data value for a Grbl setting.

- **_Feedback messages_** - Contains general feedback and can provide useful data.

- **_Startup line execution_** - Indicates a startup line as executed with the line itself and how it went.

- **_Real-time status reports_** - Contains current run data like state, position, and speed.


------

#### Alarm Message

Alarm is an emergency state. Something has gone terribly wrong when these occur. Typically, they are caused by limit error when the machine has moved or wants to move outside the machine travel and crash into the ends. They also report problems if Grbl is lost and can't guarantee positioning or a probe command has failed. Once in alarm-mode, Grbl will lock out all g-code functionality and accept only a small set of commands. It may even stop everything and force you to acknowledge the problem until you issue Grbl a reset. While in alarm-mode, the user can override the alarm manually with a specific command, which then re-enables g-code so you can move the machine again. This ensures the user knows about the problem and has taken steps to fix or account for it.

Similar to error messages, all alarm messages are sent as  **`ALARM:X`**, where `X` is an alarm code to tell you exacly what caused the alarm. The table below describes the meaning of each alarm code.

| ID | Alarm Code Description |
|:-------------:|----|
| **`1`** | Hard limit triggered. Machine position is likely lost due to sudden and immediate halt. Re-homing is highly recommended. |
| **`2`** | G-code motion target exceeds machine travel. Machine position safely retained. Alarm may be unlocked. |
| **`3`** | Reset while in motion. Grbl cannot guarantee position. Lost steps are likely. Re-homing is highly recommended. |
| **`4`** | Probe fail. The probe is not in the expected initial state before starting probe cycle, where G38.2 and G38.3 is not triggered and G38.4 and G38.5 is triggered. |
| **`5`** | Probe fail. Probe did not contact the workpiece within the programmed travel for G38.2 and G38.4. |
| **`6`** | Homing fail. Reset during active homing cycle. |
| **`7`** | Homing fail. Safety door was opened during active homing cycle. |
| **`8`** | Homing fail. Cycle failed to clear limit switch when pulling off. Try increasing pull-off setting or check wiring. |
| **`9`** | Homing fail. Could not find limit switch within search distance. Defined as `1.5 * max_travel` on search and `5 * pulloff` on locate phases. |

-------

#### Grbl `$` Settings Message

When a push message starts with a `$`, this indicates Grbl is sending a setting and its configured value. There are only two types of settings messages: a single setting and value `$x=val` and a startup string setting `$Nx=line`. See [Configuring Grbl v1.x] document if you'd like to learn how to write these values for your machine.

- `$x=val` will only appear when the user queries to print all of Grbl's settings via the `$$` print settings command. It does so sequentially and completes with an `ok`.

  - In prior versions of Grbl, the `$` settings included a short description of the setting immediately after the value. However, due to flash restrictions, most human-readable strings were removed to free up flash for the new override features in Grbl v1.1. In short, it was these strings or overrides, and overrides won. Keep in mind that once these values are set, they usually don't change, and GUIs will likely provide the assistance of translating these codes for users.

  - _**NOTE for GUI developers:**_ _As with the error and alarm codes, settings codes are available in an easy to parse CSV file in the `/doc/csv` folder. These are continually updated._

  - The `$$` settings print out is shown below and the following describes each setting.

    ```
$0=10
$1=25
$2=0
$3=0
$4=0
$5=0
$6=0
$10=255
$11=0.010
$12=0.002
$13=0
$20=0
$21=0
$22=0
$23=0
$24=25.000
$25=500.000
$26=250
$27=1.000
$30=1000
$31=0
$32=0
$100=250.000
$101=250.000
$102=250.000
$110=500.000
$111=500.000
$112=500.000
$120=10.000
$121=10.000
$122=10.000
$130=200.000
$131=200.000
$132=200.000
ok
```

	| `$x` Code | Setting Description, Units |
|:-------------:|----|
| **`0`** | Step pulse time, microseconds |
| **`1`** | Step idle delay, milliseconds |
| **`2`** | Step pulse invert, mask |
| **`3`** | Step direction invert, mask |
| **`4`** | Invert step enable pin, boolean |
| **`5`** | Invert limit pins, boolean |
| **`6`** | Invert probe pin, boolean |
| **`10`** | Status report options, mask |
| **`11`** | Junction deviation, millimeters |
| **`12`** | Arc tolerance, millimeters |
| **`13`** | Report in inches, boolean |
| **`20`** | Soft limits enable, boolean |
| **`21`** | Hard limits enable, boolean |
| **`22`** | Homing cycle enable, boolean |
| **`23`** | Homing direction invert, mask |
| **`24`** | Homing locate feed rate, mm/min |
| **`25`** | Homing search seek rate, mm/min  |
| **`26`** | Homing switch debounce delay, milliseconds |
| **`27`** | Homing switch pull-off distance, millimeters |
| **`30`** | Maximum spindle speed, RPM |
| **`31`** | Minimum spindle speed, RPM |
| **`32`** | Laser-mode enable, boolean |
| **`100`** | X-axis steps per millimeter |
| **`101`** | Y-axis steps per millimeter |
| **`102`** | Z-axis steps per millimeter |
| **`110`** | X-axis maximum rate, mm/min |
| **`111`** | Y-axis maximum rate, mm/min |
| **`112`** | Z-axis maximum rate, mm/min |
| **`120`** | X-axis acceleration, mm/sec^2 |
| **`121`** | Y-axis acceleration, mm/sec^2 |
| **`122`** | Z-axis acceleration, mm/sec^2 |
| **`130`** | X-axis maximum travel, millimeters |
| **`131`** | Y-axis maximum travel, millimeters |
| **`132`** | Z-axis maximum travel, millimeters |


- The other `$Nx=line` message is the print-out of a user-defined startup line, where `x` denotes the startup line order and ranges from `0` to `1` by default. The `line` denotes the startup line to be executed by Grbl upon reset or power-up, except during an ALARM.

  - When a user queries for the startup lines via a `$N` command, the following is sent by Grbl and completed by an `ok` response. The first line sets the initial startup work coordinate system to `G54`, while the second line is empty and does not execute.
  ```
  $N0=G54
  $N1=
  ok
  ```


------

#### Feedback Messages

Feedback messages provide non-critical information on what Grbl is doing, what it needs, and/or provide some non-real-time data for the user when queried. Not too complicated. Feedback message are always enclosed in `[]` brackets, except for the startup line execution message which begins with an open chevron character `>`.

- **Non-Queried Feedback Messages:** These feedback messages that may appear at any time and is not part of a query are listed and described below. They are usually sent as an additional helpful acknowledgement of some event or command executed. These always start with a `[MSG:` to denote their type.

  - `[MSG:Reset to continue]` - Critical event message. Reset is required before Grbl accepts any other commands. This prevents ongoing command streaming and risking a motion before the alarm is acknowledged. Only hard or soft limit errors send this message immediately after the ALARM:x code.

  - `[MSG:‘$H’|’$X’ to unlock]`- Alarm state is active at initialization. This message serves as a reminder note on how to cancel the alarm state. All g-code commands and some ‘$’ are blocked until the alarm state is cancelled via homing `$H` or unlocking `$X`. Only appears immediately after the `Grbl` welcome message when initialized with an alarm. Startup lines are not executed at initialization if this message is present and the alarm is active.

  - `[MSG:Caution: Unlocked]` - Appears as an alarm unlock `$X` acknowledgement. An 'ok' still appears immediately after to denote the `$X` was parsed and executed. This message reminds the user that Grbl is operating under an unlock state, where startup lines have still not be executed and should be cautious and mindful of what they do. Grbl may not have retained machine position due to an alarm suddenly halting the machine.  A reset or re-homing Grbl is highly recommended as soon as possible, where any startup lines will be properly executed.

  - `[MSG:Enabled]` - Appears as a check-mode `$C` enabled acknowledgement. An 'ok' still appears immediately after to denote the `$C` was parsed and executed.

  - `[MSG:Disabled]` - Appears as a check-mode `$C` disabled acknowledgement. An 'ok' still appears immediately after to denote the `$C` was parsed and executed. Grbl is automatically reset afterwards to restore all default g-code parser states changed by the check-mode.

  - `[MSG:Check Door]` - This message appears whenever the safety door detected as open. This includes immediately upon a safety door switch detects a pin change or appearing after the welcome message, if the safety door is ajar when Grbl initializes after a power-up/reset.

    - If in motion and the safety door switch is triggered, Grbl will immediately send this message, start a feed hold, and place Grbl into a suspend with the DOOR state.
    - If not in motion and not at startup, the same process occurs without the feed hold.
    - If Grbl is in a DOOR state and already suspended, any detected door switch pin detected will generate this message, including a door close.
    - If this message appears at startup, Grbl will suspended into immediately into the DOOR state. The startup lines are executed immediately after the DOOR state is exited by closing the door and sending Grbl a resume command.

  - `[MSG:Check Limits]` - If Grbl detects a limit switch as triggered after a power-up/reset and hard limits are enabled, this will appear as a courtesy message immediately after the Grbl welcome message.

  - `[MSG:Pgm End]` - M2/30 program end message to denote g-code modes have been restored to defaults according to the M2/30 g-code description.

  - `[MSG:Restoring defaults]` - Appears as an acknowledgement message when restoring EEPROM defaults via a `$RST=` command. An 'ok' still appears immediately after to denote the `$RST=` was parsed and executed.
  
  - `[MSG:Sleeping]` - Appears as an acknowledgement message when Grbl's sleep mode is invoked by issuing a `$SLP` command when in IDLE or ALARM states. Note that Grbl-Mega may invoke this at any time when the sleep timer option has been enabled and the timeout has been exceeded. Grbl may only be exited by a reset in the sleep state and will automatically enter an alarm state since the steppers were disabled.
	  - NOTE: Sleep will also invoke the parking motion, if it's enabled. However, if sleep is commanded during an ALARM, Grbl will not park and will simply de-energize everything and go to sleep.

- **Queried Feedback Messages:**

	- `[GC:]` G-code Parser State Message 
	```
     [GC:G0 G54 G17 G21 G90 G94 M5 M9 T0 F0.0 S0]
     ok
	```
   		- Initiated by the user via a `$G` command. Grbl replies as follows, where the `[GC:` denotes the message type and is followed by a separate `ok` to confirm the `$G` was executed.
     	
     	- The shown g-code are the current modal states of Grbl's g-code parser. This may not correlate to what is executing since there are usually several motions queued in the planner buffer.
     	
     	- NOTE: Program modal state has been fixed and will show `M0`, `M2`, or `M30` when they are active. During a run state, nothing will appear for program modal state.
     	 


  - `[HLP:]` : Initiated by the user via a `$` print help command. The help message is shown below with a separate `ok` to confirm the `$` was executed.
    ```
    [HLP:$$ $# $G $I $N $x=val $Nx=line $J=line $C $X $H ~ ! ? ctrl-x]
    ok
    ```
  - The `$#` print parameter data query produces a large set of data which shown below and completed by an `ok` response message.

    - Each line of the printout is starts with the data type, a `:`, and followed by the data values. If there is more than one, the order is XYZ axes, separated by commas.

      ```
      [G54:0.000,0.000,0.000]
      [G55:0.000,0.000,0.000]
      [G56:0.000,0.000,0.000]
      [G57:0.000,0.000,0.000]
      [G58:0.000,0.000,0.000]
      [G59:0.000,0.000,0.000]
      [G28:0.000,0.000,0.000]
      [G30:0.000,0.000,0.000]
      [G92:0.000,0.000,0.000]
      [TLO:0.000]
      [PRB:0.000,0.000,0.000:0]
      ok
      ```

    - The `PRB:` probe parameter message includes an additional `:` and suffix value is a boolean. It denotes whether the last probe cycle was successful or not.

  - `[VER:]` and `[OPT:]`: Indicates build info data from a `$I` user query. These build info messages are followed by an `ok` to confirm the `$I` was executed, like so:
 
      ```
      [VER:v1.1d.20161014:Some string]
      [OPT:VL]
      ok
      ```
      
  		- The first line `[VER:]` contains the build version and date.
      - A string may appear after the second `:` colon. It is a stored EEPROM string a user via a `$I=line` command or OEM can place there for personal use or tracking purposes.
  		- The `[OPT:]` line follows immediately after and contains character codes for compile-time options that were either enabled or disabled. The codes are defined below and a CSV file is also provided for quick parsing. This is generally only used for quickly diagnosing firmware bugs or compatibility issues.

		| `OPT` Code | Setting Description, Units |
|:-------------:|----|
| **`V`** | Variable spindle enabled |
| **`N`** | Line numbers enabled |
| **`M`** | Mist coolant enabled |
| **`C`** | CoreXY enabled |
| **`P`** | Parking motion enabled |
| **`Z`** | Homing force origin enabled |
| **`H`** | Homing single axis enabled |
| **`L`** | Two limit switches on axis enabled |
| **`A`** | Allow feed rate overrides in probe cycles |
| **`R`** | Classic compatiblity mode enabled |
| **`*`** | Restore all EEPROM disabled |
| **`$`** | Restore EEPROM `$` settings disabled |
| **`#`** | Restore EEPROM parameter data disabled |
| **`I`** | Build info write user string disabled |
| **`E`** | Force sync upon EEPROM write disabled |
| **`W`** | Force sync upon work coordinate offset change disabled |

  - `[echo:]` : Indicates an automated line echo from a command just prior to being parsed and executed. May be enabled only by a config.h option. Often used for debugging communication issues. A typical line echo message is shown below. A separate `ok` will eventually appear to confirm the line has been parsed and executed, but may not be immediate as with any line command containing motions.
      ```
      [echo:G1X0.540Y10.4F100]
      ```
      - NOTE: The echoed line will have been pre-parsed a bit by Grbl. No spaces or comments will appear and all letters will be capitalized.

------

#### Startup Line Execution

  - `>G54G20:ok` : The open chevron indicates a startup line has just executed. The startup line `G54G20` immediately follows the `>` character and is followed by an `:ok` response to indicate it executed successfully.

    - A startup line will execute upon initialization only if a line is present and if Grbl is not in an alarm state.

    - The `:ok` on the same line is intentional. It prevents this `ok` response from being counted as part of a stream, because it is not tied to a sent command, but an internally-generated one.

    - There should always be an appended `:ok` because the startup line is checked for validity before it is stored in EEPROM. In the event that it's not, Grbl will print `>G54G20:error:X`, where `X` is the error code explaining why the startup line failed to execute.

    - In the rare chance that there is an EEPROM read error, the startup line execution will print `>:error:7` with no startup line and a error code `7` for a read fail. Grbl will also automatically wipe and try to restore the problematic EEPROM.


------

#### Real-time Status reports

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

- **Message Construction:**

  - A message is a single line of ascii text, completed by a carriage return and line feed.

  - `< >` Chevrons uniquely enclose reports to indicate message type.

  - `|` Pipe delimiters separate data fields inside the report.

  - The first data field is an exception to the following data field rules. See 'Machine State' description for details.

  - All remaining data fields consist of a data type followed by a `:` colon delimiter and data values. `type:value(s)`

  - Data values are given either as as one or more pre-defined character codes to indicate certain states/conditions or as numeric values, which are separated by a `,` comma delimiter when more than one is present. Numeric values are also in a pre-defined order and units of measure.

  - The first (Machine State) and second (Current Position) data fields are always included in every report.

  - Assume any following data field may or may not exist and can be in any order. The `$10` status report mask setting can alter what data is present and certain data fields can be reported intermittently (see descriptions for details.)

  - The `$13` report inches settings alters the units of some data values. `$13=0` false indicates mm-mode, while `$13=1` true indicates inch-mode reporting. Keep note of this setting and which report values can be altered.

- **Data Field Descriptions:**

    - **Machine State:**

      - Valid states types:  `Idle, Run, Hold, Jog, Alarm, Door, Check, Home, Sleep`

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

        - **NOTE: Grbl v1.1 sends only one position vector because a GUI can easily compute the other position vector with the work coordinate offset `WCO:` data. See WCO description for details.**

        - Three position values are given in the order of X, Y, and Z. A fourth position value may exist in later versions for the A-axis.

        - `$13` report inches user setting effects these values and is given as either mm or inches.

        - This data field is always present as the second field.

    - **Work Coordinate Offset:**

        - `WCO:0.000,1.551,5.664` is the current work coordinate offset of the g-code parser, which is the sum of the current work coordinate system, G92 offsets, and G43.1 tool length offset.

        - Machine position and work position are related by this simple equation per axis: `WPos = MPos - WCO`
        
        	- **GUI Developers:** Simply track and retain the last `WCO:` vector and use the above equation to compute the other position vector for your position readouts. If Grbl's status reports show either `WPos` or `MPos`, just follow the equations below. It's as easy as that!
        		- If `WPos:` is given, use `MPos = WPos + WCO`.
        		- If `MPos:` is given, use `WPos = MPos - WCO`.

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

        - `Bf:15,128`. The first value is the number of available blocks in the planner buffer and the second is number of available bytes in the serial RX buffer.
        
        - The usage of this data is generally for debugging an interface, but is known to be used to control some GUI-specific tasks. While this is disabled by default, GUIs should expect this data field to appear, but they may ignore it, if desired.
        
        - NOTE: The buffer state values changed from showing "in-use" blocks or bytes to "available". This change does not require the GUI knowing how many block/bytes Grbl has been compiled with.

        - This data field appears:
        
          - In every status report when enabled. It is disabled in the settings mask by default.
        
        - This data field will not appear if:

          - It is disabled by the `$` status report mask setting or disabled in the config.h file.

    - **Line Number:**

        - `Ln:99999` indicates line 99999 is currently being executed. This differs from the `$G` line `N` value since the parser is usually queued few blocks behind execution.

        - Compile-time option only because of memory requirements. However, if a GUI passes indicator line numbers onto Grbl, it's very useful to determine when Grbl is executing them.

        - This data field will not appear if:

          - It is disabled in the config.h file. No `$` mask setting available.
          - The line number reporting not enabled in config.h. Different option to reporting data field.
          - No line number or `N0` is passed with the g-code block.
          - Grbl is homing, jogging, parking, or performing a system task/motion.
          - There is no motion in the g-code block like a `G4P1` dwell. (May be fixed in later versions.)

    - **Current Feed and Speed:**

        - There are two versions of this data field that Grbl may respond with. 
        
        	- `F:500` contains real-time feed rate data as the value. This appears only when VARIABLE_SPINDLE is disabled in config.h, because spindle speed is not tracked in this mode.
        	- `FS:500,8000` contains real-time feed rate, followed by spindle speed, data as the values. Note the `FS:`, rather than `F:`, data type name indicates spindle speed data is included.
                
        - The current feed rate value is in mm/min or inches/min, depending on the `$` report inches user setting. 
        - The second value is the current spindle speed in RPM
        
        - These values will often not be the programmed feed rate or spindle speed, because several situations can alter or limit them. For example, overrides directly scale the programmed values to a different running value, while machine settings, acceleration profiles, and even the direction traveled can also limit rates to maximum values allowable.

        - As a operational note, reported rate is typically 30-50 msec behind actual position reported.

        - This data field will always appear, unless it was explicitly disabled in the config.h file.

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

        - Override maximum, minimum, and increment sizes are all configurable within config.h. Assume that a user or OEM will alter these based on customized use-cases. Recommend not hard-coding these values into a GUI, but rather just show the actual override values and generic increment buttons.

        - Override values don't change often during a job once set and only requires intermittent refreshing. This data field appears:

          - After 10 or 20 (configurable 1-255) status reports, depending on is in a motion state or not.
          - If an override value has changed, this data field will appear immediately in the next report. However, if `WCO:` is present, this data field will be delayed one report.
          - In the second report after a reset/power-cycle.

        - This data field will not appear if:

          - It is disabled in the config.h file. No `$` mask setting available.
          - The override refresh counter is in-between intermittent reports.
          - `WCO:` exists in current report during refresh. Automatically set to try again on next report.

	- **Accessory State:**

		- `A:SFM` indicates the current state of accessory machine components, such as the spindle and coolant.

		- Due to the new toggle overrides, these machine components may not be running according to the g-code program. This data is provided to ensure the user knows exactly what Grbl is doing at any given time.
		
		- Each letter after `A:` denotes a particular state. When it appears, the state is enabled. When it does not appear, the state is disabled.

			- `S` indicates spindle is enabled in the CW direction. This does not appear with `C`.
			- `C` indicates spindle is enabled in the CCW direction. This does not appear with `S`.
        	- `F` indicates flood coolant is enabled.
        	- `M` indicates mist coolant is enabled.
		
	   - Assume accessory state letters are presented in no particular order.
      
      - This data field appears:

			- When any accessory state is enabled.
        	- Only with the override values field in the same message. Any accessory state change will trigger the accessory state and override values fields to be shown on the next report.

      - This data field will not appear if:

        	- No accessory state is active.
        	- It is disabled in the config.h file. No `$` mask setting available.
        	- If override refresh counter is in-between intermittent reports.
        	- `WCO:` exists in current report during refresh. Automatically set to try again on next report.


-------
# Message Summary

Grbl v1.1's interface protocol has been tweaked in the attempt to make GUI development cleaner, clearer, and hopefully easier. All messages are designed to be deterministic without needing to know the context of the message. Each can be inferred to a much greater degree than before just by the message type, which are all listed below.

- `ok` / `error:x` : Normal send command and execution response acknowledgement. Used for streaming.

- `< >` : Enclosed chevrons contains status report data.

- `Grbl X.Xx ['$' for help]` : Welcome message indicates initialization.

- `ALARM:x` : Indicates an alarm has been thrown. Grbl is now in an alarm state.

- `$x=val` and `$Nx=line` indicate a settings printout from a `$` and `$N` user query, respectively.

- `[MSG:]` : Indicates a non-queried feedback message.

- `[GC:]` : Indicates a queried `$G` g-code state message.

- `[HLP:]` : Indicates the help message.

- `[G54:]`, `[G55:]`, `[G56:]`, `[G57:]`, `[G58:]`, `[G59:]`, `[G28:]`, `[G30:]`, `[G92:]`, `[TLO:]`, and `[PRB:]` messages indicate the parameter data printout from a `$#` user query.

- `[VER:]` : Indicates build info and string from a `$I` user query.

- `[echo:]` : Indicates an automated line echo from a pre-parsed string prior to g-code parsing. Enabled by config.h option.

- `>G54G20:ok` : The open chevron indicates startup line execution. The `:ok` suffix shows it executed correctly without adding an unmatched `ok` response on a new line.

In addition, all `$x=val` settings, `error:`, and `ALARM:` messages no longer contain human-readable strings, but rather codes that are defined in other documents. The `$` help message is also reduced to just showing the available commands. Doing this saves incredible amounts of flash space. Otherwise, the new overrides features would not have fit.

Other minor changes and bug fixes that may effect GUI parsing include:

- Floating point values printed with zero precision do not show a decimal, or look like an integer. This includes spindle speed RPM and feed rate in mm mode.
- `$G` reports fixed a long time bug with program modal state. It always showed `M0` program pause when running. Now during a normal program run, no program modal state is given until an `M0`, `M2`, or `M30` is active and then the appropriate state will be shown.

On a final note, this interface tweak came about out of necessity, as more data is being sent back from Grbl and it is capable of doing many more things. It's not intended to be altered again in the near future, if at all. This is likely the only and last major change to this. If you have any comments or suggestions before Grbl v1.1 goes to master, please do immediately so we can all vet the new alteration before its installed.
