## Grbl v1.0 Jogging

Executing a jog requires a specific command structure, as described below:

 - The first three characters must be '$J=' to indicate the jog.
 - The jog command follows immediate after the '=' and works like a normal G1 command.
 - Feed rate is only interpreted in G94 units per minute. A prior G93 state is ignored during jog.
 - Required words:
   - XYZ: One or more axis words with target value.
   - F - Feed rate value. NOTE: Each jog requires this value and is not treated as modal.
 - Optional words: Jog executes based on current G20/G21 and G90/G91 g-code parser state. If one
   of the following optional words is passed, that state is overridden for one command only.
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
command is not valid, Grbl will return an 'error:'. Multiple jogging commands may be
queued in sequence.

The main differences are:
- During a jog, Grbl will report a 'Jog' state while executing the jog.
- A jog command will only be accepted when Grbl is in either the 'Idle' or 'Jog' states.
- Jogging motions may not be mixed with g-code commands while executing, which will return
  a lockout error, if attempted.
- All jogging motion(s) may be cancelled at anytime with a simple feed hold command. Grbl
  will automatically flush Grbl's internal buffers of any queued jogging motions and return
  to the 'Idle' state. No soft-reset required.
- IMPORTANT: Jogging does not alter the g-code parser state. Hence, no g-code modes need to
  be explicitly managed, unlike previous ways of implementing jogs with commands like
  'G91G1X1F100'. Since G91, G1, and F feed rates are modal and if they are not changed
  back prior to resuming/starting a job, a job may not run how its was intended and result
  in a crash.
