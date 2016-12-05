## Grbl v1.1 Realtime commands

Realtime commands are single control characters that may be sent to Grbl to command and perform an action in real-time. This means that they can be sent at anytime, anywhere, and Grbl will immediately respond, regardless of what it is doing at the time. These commands include a reset, feed hold, resume, status report query, and overrides (in v1.1).

A realtime command:

- Will execute within tens of milliseconds.

- Is a single character that may be sent to Grbl at any time.

- Does not require a line feed or carriage return after them.

- Is not considered a part of the streaming protocol. 

- Are intercepted when they are received and never placed in a buffer to be parsed by Grbl.

- Will ignore multiple commands until it has executed the first received command.

- May be tied to an input pin and may be operated with a button or switch.

- Actions depends on state or what Grbl is doing. It may not do anything.

- Descriptions explain how they work and what to expect.

#### ASCII Realtime Command Descriptions
Four realtime commands are type-able by users on a keyboard and shown in the `$` Grbl help message. These realtime command characters control some of Grbl's basic functions.

- `0x18` (ctrl-x) : Soft-Reset

  - Immediately halts and safely resets Grbl without a power-cycle.
  - Accepts and executes this command at any time.
  - If reset while in motion, Grbl will throw an alarm to indicate position may be lost from the motion halt.
  - If reset while in not motion, position is retained and re-homing is not required.
  - An input pin is available to connect a button or switch.


- `?` : Status Report Query

  - Immediately generates and sends back runtime data with a status report.
  - Accepts and executes this command at any time, except during a homing cycle and when critical alarm (hard/soft limit error) is thrown.


- `~` : Cycle Start / Resume

  - Resumes a feed hold, a safety door/parking state when the door is closed, and the M0 program pause states.
  - Command is otherwise ignored.
  - If the parking compile-time option is enabled and the safety door state is ready to resume, Grbl will re-enable the spindle and coolant, move back into position, and then resume.
  - An input pin is available to connect a button or switch.


- `!` : Feed Hold

  - Places Grbl into a suspend or HOLD state. If in motion, the machine will decelerate to a stop and then be suspended.
  - Command executes when Grbl is in an IDLE, RUN, or JOG state. It is otherwise ignored.
  - If jogging, a feed hold will cancel the jog motion and flush all remaining jog motions in the planner buffer. The state will return from JOG to IDLE or DOOR, if was detected as ajar during the active hold.
  - By machine control definition, a feed hold does not disable the spindle or coolant. Only motion.
  - An input pin is available to connect a button or switch.


#### Extended-ASCII Realtime Command Descriptions

Grbl v1.1 installed more than a dozen new realtime commands to control feed, rapid, and spindle overrides. To help prevent users from inadvertently altering overrides with a keystroke and allow for more commands later on, all of the new control characters have been moved to the extended ASCII character set. These are not easily type-able on a keyboard, but, depending on the OS, they may be entered using specific keystroke and code. GUI developers will need to be able to send extended ASCII characters, values `128 (0x80)` to `255 (0xFF)`, to Grbl to take advantage of these new features.

- `0x84` : Safety Door

  - Although typically connected to an input pin to detect the opening of a safety door, this command allows a GUI to enact the safety door behavior with this command.
  - Immediately suspends into a DOOR state and disables the spindle and coolant. If in motion, the machine will decelerate to a stop and then be suspended.
  - If executed during homing, Grbl will instead halt motion and throw a homing alarm.
  - If already in a suspend state or HOLD, the DOOR state supersedes it.
  - If the parking compile-time option is enabled, Grbl will park the spindle to a specified location.
  - Command executes when Grbl is in an IDLE, HOLD, RUN, HOMING, or JOG state. It is otherwise ignored.
  - If jogging, a safety door will cancel the jog and all queued motions in the planner buffer. When the safety door is closed and resumed, Grbl will return to an IDLE state.
  - An input pin is available to connect a button or switch, if enabled with a compile-time option.
  - Some builds of Grbl v0.9 used the `@` character for this command, but it was undocumented. Moved to extended-ASCII to prevent accidental commanding.


- `0x85` : Jog Cancel

  - Immediately cancels the current jog state by a feed hold and automatically flushing any remaining jog commands in the buffer.
  - Command is ignored, if not in a JOG state or if jog cancel is already invoked and in-process.
  - Grbl will return to the IDLE state or the DOOR state, if the safety door was detected as ajar during the cancel.
  

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


- Spindle Speed Overrides

  - Immediately alters the spindle speed override value. An active spindle speed is altered within tens of milliseconds.
  - Override values may be changed at any time, regardless of if the spindle is enabled or disabled.
  - Spindle override value can not be 10% or greater than 200%
  - If spindle override value does not change, the command is ignored.
  - Spindle override range and increments may be altered in config.h.
  - The commands are:
    - `0x99` : Set 100% of programmed spindle speed
    - `0x9A` : Increase 10%
    - `0x9B` : Decrease 10%
    - `0x9C` : Increase 1%
    - `0x9D` : Decrease 1%


- `0x9E` : Toggle Spindle Stop

  - Toggles spindle enable or disable state immediately, but only while in the HOLD state.
  - The command is otherwise ignored, especially while in motion. This prevents accidental disabling during a job that can either destroy the part/machine or cause personal injury. Industrial machines handle the spindle stop override similarly.
  - When motion restarts via cycle start, the last spindle state will be restored and wait 4.0 seconds (configurable) before resuming the tool path. This ensures the user doesn't forget to turn it back on.
  - While disabled, spindle speed override values may still be altered and will be in effect once the spindle is re-enabled.
  - If a safety door is opened, the DOOR state will supersede the spindle stop override, where it will manage the spindle re-energizing itself upon closing the door and resuming. The prior spindle stop override state is cleared and reset.


- `0xA0` : Toggle Flood Coolant

  - Toggles flood coolant state and output pin until the next toggle or g-code command alters it.
  - May be commanded at any time while in IDLE, RUN, or HOLD states. It is otherwise ignored.
  - This override directly changes the coolant modal state in the g-code parser. Grbl will continue to operate normally like it received and executed an `M8` or `M9` g-code command.
  - When `$G` g-code parser state is queried, the toggle override change will be reflected by an `M8` enabled or disabled with an `M9` or not appearing when `M7` is present.


- `0xA1` : Toggle Mist Coolant

  - Enabled by `ENABLE_M7` compile-time option. Default is disabled.
  - Toggles mist coolant state and output pin until the next toggle or g-code command alters it.
  - May be commanded at any time while in IDLE, RUN, or HOLD states. It is otherwise ignored.
  - This override directly changes the coolant modal state in the g-code parser. Grbl will continue to operate normally like it received and executed an `M7` or `M9` g-code command.
  - When `$G` g-code parser state is queried, the toggle override change will be reflected by an `M7` enabled or disabled with an `M9` or not appearing when `M8` is present.
