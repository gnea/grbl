# GRBL-CRPT 1.0.0 Commands

In general, GRBL-CRPT assumes all characters and streaming data sent to it is g-code and will parse and try to execute it as soon as it can. However, GRBL-CRPT also has two separate system command types that are outside of the normal g-code streaming. One system command type is streamed to GRBL-CRPT like g-code, but starts with a `$` character to tell GRBL-CRPT it's not g-code. The other is composed of a special set of characters that will immediately command GRBL-CRPT to do a task in real-time. It's not part of the g-code stream. GRBL-CRPT's system commands do things like control machine state, report saved parameters or what GRBL-CRPT is doing, save or print machine settings, run a homing cycle, or make the machine move faster or slower than programmed. This document describes these "internal" system GRBL-CRPT commands, what they do, how they work, and how to use them.

## Getting Started

First, connect to GRBL-CRPT using the serial terminal of your choice.

Set the baud rate  to **115200** as 8-N-1 (8-bits, no parity, and 1-stop bit.)

Once connected
 you should get the GRBL-CRPT prompt, which looks like this:

```
GRBL-CRPT 1.0.0 ['$' for help]
```

Type $ and press enter to have GRBL-CRPT print a help message. You should not see any local echo of the $ and enter. GRBL-CRPT should respond with:

```
[HLP:$$ $# $G $I $N $x=val $Nx=line $J=line $SLP $C $X $H ~ ! ? ctrl-x]
```

The ‘$’-commands are GRBL-CRPT system commands used to tweak the settings, view or change GRBL-CRPT's states and running modes, and start a homing cycle. The last four **non**-'$' commands are realtime control commands that can be sent at anytime, no matter what GRBL-CRPT is doing. These either immediately change GRBL-CRPT's running behavior or immediately print a report of the important realtime data like current position (aka DRO). There are over a dozen more realtime control commands, but they are not user type-able. See realtime command section for details.

***

## GRBL-CRPT '$' Commands

The `$` system commands provide additional controls for the user, such as printing feedback on the current G-code parser modal state or running the homing cycle. This section explains what these commands are and how to use them.

#### `$$`and `$x=val` - View and write GRBL-CRPT settings
See [Grbl v1.1 Configuration](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Configuration#grbl-settings) for more details on how to view and write setting and learn what they are.

#### `$#` - View gcode parameters

G-code parameters store the coordinate offset values for G54-G59 work coordinates, G28/G30 pre-defined positions, G92 coordinate offset, and tool length offsets. Most of these parameters are directly written to EEPROM anytime they are changed and are persistent. The non-persistent parameters, which are not retained when reset or power-cycled, are G92 and G43.1 tool length offsets.

G54-G59 work coordinates can be changed via the `G10 L2 Px` or `G10 L20 Px` command defined by the NIST gcode standard and the EMC2 (linuxcnc.org) standard. G28/G30 pre-defined positions can be changed via the `G28.1` and the `G30.1` commands, respectively.

When `$#` is called, GRBL-CRPT responds with the stored offsets from machine coordinates for each system as follows. `TLO` denotes the tool length offset.

```
[G54:4.000,0.000,0.000]
[G55:4.000,6.000,7.000]
[G56:0.000,0.000,0.000]
[G57:0.000,0.000,0.000]
[G58:0.000,0.000,0.000]
[G59:0.000,0.000,0.000]
[G28:1.000,2.000,0.000]
[G30:4.000,6.000,0.000]
[G92:0.000,0.000,0.000]
[TLO:0.000]
```

#### `$G` - View gcode parser state

This command prints all of the active gcode modes in GRBL-CRPT's G-code parser. When sending this command to GRBL-CRPT, it will reply with a message starting with an `[GC:` indicator like: 

```
[GC:G0 G54 G17 G21 G90 G94 M0 T0 F500.0]
```

These active modes determine how the next G-code block or command will be interpreted by GRBL-CRPT's G-code parser. Modes set the parser into a particular state so you do not have to repeatedly specify it. These modes are organized into sets called "modal groups" that cannot be logically active at the same time. For example, the units modal group sets whether G-code is interpreted in inches or in millimeters.

A short list of the modal groups, supported by Grbl, is shown below, but more complete and detailed descriptions can be found at LinuxCNC's [website](http://www.linuxcnc.org/docs/2.4/html/gcode_overview.html#sec:Modal-Groups). The G-code commands in **bold** indicate the default modes upon powering-up Grbl or resetting it. The commands in _italics_ indicate a special Grbl-only command.

| Modal Group Meaning	|  Member Words |
|:----:|:----:|
| Motion Mode | **G0**, G1, G2, G3, G80 |
|Coordinate System Select	| **G54**, G55, G56, G57, G58, G59|
|Plane Select	| **G17**, G18, G19|
|Distance Mode	| **G90**, G91|
|Arc IJK Distance Mode | **G91.1** |
|Feed Rate Mode	| G93, **G94**|
|Units Mode	| G20, **G21**|
|Cutter Radius Compensation | **G40** |
|Tool Length Offset |G43.1, **G49**|
|Program Mode | **M0**, M1, M2, M30|
|Override Control | _M56_ |

GRBL-CRPT supports a special _M56_ override control command, where this enables and disables GRBL-CRPT's parking motion when a `P1` or a `P0` is passed with `M56`, respectively. This command is only available when both parking and this particular option is enabled.

In addition to the G-code parser modes, GRBL-CRPT will report the active `T` tool number and `F` feed rate, which both default to 0 upon a reset. For those that are curious, these don't quite fit into nice modal groups, but are just as important for determining the parser state.

#### `$I` - View build info
This prints the GRBL-CRPT version and source code build date. Optionally, `$I` can also store a short string to identify the robot or firmware configuration. To set this string, send GRBL-CRPT `$I=xxx`, where `xxx` is your customization string that is less than 80 characters. The next time you query GRBL-CRPT with `$I`, GRBL-CRPT prints this string after the version and build date.

NOTE: Some OEMs may block access to over-writing the build info string so they can store product information and codes there.

#### $N - View startup blocks

`$Nx` are the startup blocks that GRBL-CRPT runs every time you power on GRBL-CRPT or reset GRBL-CRPT. In other words, a startup block is a line of G-code that you can have GRBL-CRPT auto-magically run to set your G-code modal defaults, or anything else you need GRBL-CRPT to do everytime you start up your machine. GRBL-CRPT can store two blocks of G-code as a system default.

So, when connected to GRBL-CRPT, type `$N` and then enter. GRBL-CRPT should respond with something short like:
```
$N0=
$N1=
ok
```
Not much to go on, but this just means that there is no G-code block stored in line `$N0` for GRBL-CRPT to run upon startup. `$N1` is the next line to be run.

#### $Nx=line - Save startup block

**IMPORTANT: Be very careful when storing any motion (G0/1,G2/3,G28/30) commands in the startup blocks. These motion commands will run everytime you reset or power up GRBL-CRPT, so if you have an emergency situation and have to e-stop and reset, a startup block move can and will likely make things worse quickly. Also, do not place any commands that save data to EEPROM, such as G10/G28.1/G30.1. This will cause GRBL-CRPT to constantly re-write this data upon every startup and reset, which will eventually wear out your Arduino's EEPROM.**

**Typical usage for a startup block is simply to set your preferred modal states, such as G20 inches mode, always default to a different work coordinate system, or, to provide a way for a user to run some user-written unique feature that they need for their crazy project.**

To set a startup block, type `$N0=` followed by a valid G-code block and an enter. GRBL-CRPT will run the block to check if it's valid and then reply with an `ok` or an `error:` to tell you if it's successful or something went wrong. If there is an error, GRBL-CRPT will not save it.

For example, say that you want to use your first startup block `$N0` to set your G-code parser modes like G54 work coordinate, G20 inches mode, G17 XY-plane. You would type `$N0=G20 G54 G17` with an enter and you should see an `ok` response. You can then check if it got stored by typing `$N` and you should now see a response like `$N0=G20G54G17`.

Once you have a startup block stored in GRBL-CRPT's EEPROM, everytime you startup or reset you will see your startup block printed back to you, starting with an open-chevron `>`, and a `:ok` response from GRBL-CRPT to indicate if it ran okay. So for the previous example, you'll see:

```
GRBL-CRPT 1.0.0 ['$' for help]
>G20G54G17:ok

```
If you have multiple G-code startup blocks, they will print back to you in order upon every startup. And if you'd like to clear one of the startup blocks, (e.g., block 0) type `$N0=` without anything following the equal sign.

NOTE: There are two variations on when startup blocks with run. First, it will not run if GRBL-CRPT initializes up in an ALARM state or exits an ALARM state via an `$X` unlock for safety reasons. Always address and cancel the ALARM and then finish by a reset, where the startup blocks will run at initialization. Second, if you have homing enabled, the startup blocks will execute immediately after a successful homing cycle, not at startup.

#### `$C` - Check gcode mode
This toggles the GRBL-CRPT G-code parser to process incoming blocks as it would in normal operation, but without moving the axes. It ignores dwells and auxiliary outputs. This lets users check a G-code program for parser errors and soft-limit violations.

When toggled off, GRBL-CRPT will perform an automatic soft-reset (^X). This is for two purposes. It simplifies the code management a bit. But, it also prevents users from starting a job when their G-code modes are not what they think they are. A system reset always gives the user a fresh, consistent start.

#### `$X` - Kill alarm lock
GRBL-CRPT's alarm mode is a state when something has gone critically wrong, such as a hard limit or an abort during a cycle, or if GRBL-CRPT doesn't know its position. By default, if you have homing enabled and power-up the Arduino, GRBL-CRPT enters the alarm state, because it does not know its position. The alarm mode will lock all G-code commands until the '$H' homing cycle has been performed. Or if a user needs to override the alarm lock to move their axes off their limit switches, for example, '$X' kill alarm lock will override the locks and allow G-code functions to work again.

But, tread carefully!! This should only be used in emergency situations. The position has likely been lost, and GRBL-CRPT may not be where you think it is. So, it's advised to use G91 incremental mode to make short moves. Then, perform a homing cycle or reset immediately afterwards.

As noted earlier, startup lines do not execute after a `$X` command. Always reset when you have cleared the alarm and fixed the scenario that caused it. When GRBL-CRPT resets to idle, the startup lines will then run as normal.

#### `$H` - Run homing cycle
This command is the only way to perform the homing cycle in GRBL-CRPT. Some other motion controllers designate a special G-code command to run a homing cycle, but this is incorrect according to the G-code standards. Homing is a completely separate command handled by the controller.

TIP: After running a homing cycle, rather than jogging manually each time, set a G28 or G30 pre-defined position as the post-homing destination within your workspace. Jog the machine to the desired location and send `G28.1` or `G30.1` to store it. After `$H`, sending `G28` or `G30` returns the machine to that position. For example, move the XY axes toward the center and leave Z raised to provide clearance.

#### `$Jx=line` - Run jogging motion

New to GRBL-CRPT 1.0.0, this command will execute a special jogging motion. There are three main differences between a jogging motion and a motion commanded by a g-code line.

- Like normal g-code commands, several jog motions may be queued into the planner buffer, but the jogging can be easily canceled by a jog-cancel or feed-hold real-time command. GRBL-CRPT will immediately hold the current jog and then automatically purge the buffers of any remaining commands. 
- Jog commands are completely independent of the g-code parser state. It will not change any modes like `G91` incremental distance mode. So, you no longer have to make sure that you change it back to `G90` absolute distance mode afterwards. This helps reduce the chance of starting with the wrong g-code modes enabled.
- If soft-limits are enabled, any jog command that exceeds a soft-limit will simply return an error. It will not throw an alarm as it would with a normal g-code command. This allows for a much more enjoyable and fluid GUI or joystick interaction.

Executing a jog requires a specific command structure, as described below:

 - The first three characters must be '$J=' to indicate the jog.
 - The jog command follows immediate after the '=' and works like a normal G1 command.
 - Feed rate is only interpreted in G94 units per minute. A prior G93 state is ignored during jog.
 - Required words:
   - XYZ: One or more axis words with target value.
   - F - Feed rate value. NOTE: Each jog requires this value and is not treated as modal.
 - Optional words: Jog executes based on current G20/G21 and G90/G91 g-code parser state. If one of the following optional words is passed, that state is overridden for one command only.
   - G20 or G21 - Inch and millimeter mode
   - G90 or G91 - Absolute and incremental distances
   - G53 - Move in machine coordinates
 - All other g-codes, m-codes, and value words are not accepted in the jog command.
 - Spaces and comments are allowed in the command. These are removed by the pre-parser.

 - Example: G21 and G90 are active modal states prior to jogging. These are sequential commands.
    - `$J=X10.0 Y-1.5` will move to X=10.0mm and Y=-1.5mm in work coordinate frame (WPos).
    - `$J=G91 G20 X0.5` will move +0.5 inches (12.7mm) to X=22.7mm (WPos). Note that G91 and G20 are only applied to this jog command.
    - `$J=G53 Y5.0` will move the machine to Y=5.0mm in the machine coordinate frame (MPos). If the work coordinate offset for the y-axis is 2.0mm, then Y is 3.0mm in (WPos).

Jog commands behave almost identically to normal g-code streaming. Every jog command will
return an 'ok' when the jogging motion has been parsed and is setup for execution. If a
command is not valid or exceeds a soft-limit, GRBL-CRPT will return an 'error:'. Multiple jogging commands may be queued in sequence.

NOTE: See additional jogging documentation for details on using this command to create a low-latency joystick or rotary dial interface.


#### `$RST=$`, `$RST=#`, and `$RST=*`- Restore GRBL-CRPT settings and data to defaults
These commands are not listed in the main GRBL-CRPT `$` help message, but are available to allow users to restore parts of or all of GRBL-CRPT's EEPROM data. Note: GRBL-CRPT will automatically reset after executing one of these commands to ensure the system is initialized correctly.

- `$RST=$` : Erases and restores the `$$` GRBL-CRPT settings back to defaults, which is defined by the default settings file used when compiling GRBL-CRPT. Often OEMs will build their GRBL-CRPT firmwares with their machine-specific recommended settings. This provides users and OEMs a quick way to get back to square-one, if something went awry or if a user wants to start over.
- `$RST=#` : Erases and zeros all G54-G59 work coordinate offsets and G28/30 positions stored in EEPROM. These are generally the values seen in the `$#` parameters printout. This provides an easy way to clear these without having to do it manually for each set with a `G20 L2/20` or `G28.1/30.1` command.
- `$RST=*` : This clears and restores all of the EEPROM data used by GRBL-CRPT. This includes `$$` settings, `$#` parameters, `$N` startup lines, and `$I` build info string. Note that this doesn't wipe the entire EEPROM, only the data areas GRBL-CRPT uses. To do a complete wipe, please use the Arduino IDE's EEPROM clear example project.

NOTE: Some OEMs may restrict some or all of these commands to prevent certain data they use from being wiped. 

#### `$SLP` - Enable Sleep Mode

This command places GRBL-CRPT into a de-powered sleep state, disabling auxiliary outputs and stepper drivers and blocking other commands. It may only be exited by a soft-reset or power-cycle. Once re-initialized, GRBL-CRPT automatically enters an ALARM state because it cannot determine its position after the steppers have been disabled.

This feature can automatically disable the stepper drivers at the end of a program. Before using it, move the robot to a safe position and account for loss of position after the drivers are disabled. Do not rely on firmware as an emergency stop or leave the robot unattended.


***

## GRBL-CRPT 1.0.0 Realtime commands

Realtime commands are single control characters that may be sent to GRBL-CRPT to command and perform an action in real-time. This means that they can be sent at anytime, anywhere, and GRBL-CRPT will immediately respond, regardless of what it is doing at the time. These commands include a reset, feed hold, resume, status report query, and overrides (as introduced in upstream Grbl v1.1).

A realtime command:

- Will execute within tens of milliseconds.

- Is a single character that may be sent to GRBL-CRPT at any time.

- Does not require a line feed or carriage return after them.

- Is not considered a part of the streaming protocol. 

- Are intercepted when they are received and never placed in a buffer to be parsed by GRBL-CRPT.

- Will ignore multiple commands until it has executed the first received command.

- May be tied to an input pin and may be operated with a button or switch.

- Actions depends on state or what GRBL-CRPT is doing. It may not do anything.

- Descriptions explain how they work and what to expect.

#### ASCII Realtime Command Descriptions
Four realtime commands are type-able by users on a keyboard and shown in the `$` GRBL-CRPT help message. These realtime command characters control some of GRBL-CRPT's basic functions.

- `0x18` (ctrl-x) : Soft-Reset

  - Immediately halts and safely resets GRBL-CRPT without a power-cycle.
  - Accepts and executes this command at any time.
  - If reset while in motion, GRBL-CRPT will throw an alarm to indicate position may be lost from the motion halt.
  - If reset while in not motion, position is retained and re-homing is not required.
  - An input pin is available to connect a button or switch.


- `?` : Status Report Query

  - Immediately generates and sends back runtime data with a status report.
  - Accepts and executes this command at any time, except during a homing cycle and when critical alarm (hard/soft limit error) is thrown.


- `~` : Cycle Start / Resume

  - Resumes a feed hold, a safety door/parking state when the door is closed, and the M0 program pause states.
  - Command is otherwise ignored.
  - If the parking compile-time option is enabled and the safety door state is ready to resume, GRBL-CRPT will move back into position and then resume.
  - An input pin is available to connect a button or switch.


- `!` : Feed Hold

  - Places GRBL-CRPT into a suspend or HOLD state. If in motion, the machine will decelerate to a stop and then be suspended.
  - Command executes when GRBL-CRPT is in an IDLE, RUN, or JOG state. It is otherwise ignored.
  - If jogging, a feed hold will cancel the jog motion and flush all remaining jog motions in the planner buffer. The state will return from JOG to IDLE or DOOR, if was detected as ajar during the active hold.
  - A feed hold stops motion but does not disable the auxiliary outputs.
  - An input pin is available to connect a button or switch.


#### Extended-ASCII Realtime Command Descriptions

GRBL-CRPT 1.0.0 includes extended-ASCII realtime commands to control feed and rapid overrides. To help prevent users from inadvertently altering overrides with a keystroke and allow for more commands later on, the newer control characters have been moved to the extended ASCII character set. These are not easily type-able on a keyboard, but, depending on the OS, they may be entered using specific keystroke and code. GUI developers will need to be able to send extended ASCII characters, values `128 (0x80)` to `255 (0xFF)`, to GRBL-CRPT to take advantage of these features.

- `0x84` : Safety Door

  - Although typically connected to an input pin to detect the opening of a safety door, this command allows a GUI to enact the safety door behavior with this command.
  - Immediately suspends into a DOOR state. If in motion, the robot will decelerate to a stop and then be suspended.
  - If executed during homing, GRBL-CRPT will instead halt motion and throw a homing alarm.
  - If already in a suspend state or HOLD, the DOOR state supersedes it.
  - If the parking compile-time option is enabled, GRBL-CRPT will park the robot at the configured location.
  - Command executes when GRBL-CRPT is in an IDLE, HOLD, RUN, HOMING, or JOG state. It is otherwise ignored.
  - If jogging, a safety door will cancel the jog and all queued motions in the planner buffer. When the safety door is closed and resumed, GRBL-CRPT will return to an IDLE state.
  - An input pin is available to connect a button or switch, if enabled with a compile-time option.
  - Some builds of GRBL-CRPT v0.9 used the `@` character for this command, but it was undocumented. Moved to extended-ASCII to prevent accidental commanding.


- `0x85` : Jog Cancel

  - Immediately cancels the current jog state by a feed hold and automatically flushing any remaining jog commands in the buffer.
  - Command is ignored, if not in a JOG state or if jog cancel is already invoked and in-process.
  - GRBL-CRPT will return to the IDLE state or the DOOR state, if the safety door was detected as ajar during the cancel.
  

- Feed Overrides

  - Immediately alters the feed override value. An active feed motion is altered within tens of milliseconds.
  - Does not alter rapid rates, which include G0, G28, and G30, or jog motions.
  - Feed override value can not be 10% or greater than 200%.
  - If feed override value does not change, the command is ignored.
  - Feed override range and increments may be changed in config.h.
  - The commands are:
    - `0x90` : Set 100% of programmed rate.
    - `0x91` : Increase 10%
    - `0x92` : Decrease 10%
    - `0x93` : Increase 1%
    - `0x94` : Decrease 1%


- Rapid Overrides

  - Immediately alters the rapid override value. An active rapid motion is altered within tens of milliseconds.
  - Only effects rapid motions, which include G0, G28, and G30.
  - If rapid override value does not change, the command is ignored.
  - Rapid override set values may be changed in config.h.
  - The commands are:
    - `0x95` : Set to 100% full rapid rate.
    - `0x96` : Set to 50% of rapid rate.
    - `0x97` : Set to 25% of rapid rate.
