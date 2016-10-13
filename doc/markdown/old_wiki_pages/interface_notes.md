***

# Writing an Interface for Grbl

_**FOR DEVELOPERS ONLY: This section outlines the recommended ways to setup a communications and streaming protocol with Grbl for a GUI.**_

The general interface for Grbl has been described above, but what's missing is how to run an entire G-code program on Grbl, when it doesn't seem to have an upload feature. Or, how to build a decent GUI with real-time feedback. This is where this section fits in. Early on, users fiercely requested for flash drive, external RAM, LCD support, joysticks, or network support so they can upload a g-code program and run it directly on Grbl. The general answer to that is, good ideas, but Grbl doesn't need them. Grbl already has nearly all of the tools and features to reliably communicate with a simple graphical user interface (GUI). Plus, we want to minimize as much as we can on what Grbl should be doing, because, in the end, Grbl needs to be concentrating on producing clean, reliable motion. That's it.


## Streaming a G-Code Program to Grbl

Here we will describe three different streaming methods for Grbl GUIs, but really there's only two that we recommend using. One of the main problems with streaming to Grbl is the USB port itself. Arduinos and most all micro controllers use a USB-to-serial converter chip that, at times, behaves strangely and not typically how you'd expect, like USB packet buffering and delays that can wreak havoc to a streaming protocol. Another problem is how to deal with some of the latency and oddities of the PCs themselves, because none of them are truly real-time and always create micro-delays when executing other tasks. Regardless, we've come up with ways to ensure the G-code stream is reliable and simple.

#### Streaming Protocol: Simple Send-Response _[Recommended for Grbl v0.9+]_
The send-response streaming protocol is the most fool-proof and simplest method to stream a G-code program to Grbl. The host PC interface simply sends a line of G-code to Grbl and waits for an `ok` or `error:` response before sending the next line of G-code. So, no matter if Grbl needs to wait for room in the look-ahead planner buffer to finish parsing and executing the last line of G-code or if the the host computer is busy doing something, this guarantees both to the host PC and Grbl, the programmed G-code has been sent and received properly. An example of this protocol is published in our `simple_stream.py` script in our repository.

However, it's also the slowest of three outlined streaming protocols. Grbl essentially has two buffers between the execution of steps and the host PC interface. One of them is the serial receive buffer. This briefly stores up to 127 characters of data received from the host PC until Grbl has time to fetch and parse the line of G-code. The other buffer is the look-ahead planner buffer. This buffer stores up to 17 line motions that are acceleration-planned and optimized for step execution. Since the send-response protocol receives a line of G-code while the host PC waits for a response, Grbl's serial receive buffer is usually empty and under-utilized. If Grbl is actively running and executing steps, Grbl will immediately begin to execute and empty the look-ahead planner buffer, while it sends the response to the host PC, waits for the next line from the host PC, upon receiving it, parse and plan it, and add it to the end of the look-ahead buffer.

Although this communication lag may take only a fraction of a second, there is a cumulative effect, because there is a lag with every G-code block sent to Grbl. In certain scenarios, like a G-code program containing lots of sequential, very short, line segments with high feed rates, the cumulative lag can be large enough to empty and starve the look-ahead planner buffer within this time. This could lead to start-stop motion when the streaming can't keep up with G-code program execution. Also, since Grbl can only plan and optimize what's in the look-ahead planner buffer, the performance through these types of motions will never be full-speed, because look-ahead buffer will always be partially full when using this streaming method. If your expected application doesn't contain a lot of these short line segments with high feed rates, this streaming protocol should be more than adequate for a vast majority of applications, is very robust, and is a quick way to get started. However, we do not recommend using this method for Grbl versions v0.8 or prior due to some performance issues with these versions.

#### Streaming Protocol: Via Flow Control (XON/XOFF)

To avoid the risk of starving the look-ahead planner buffer, a flow control streaming protocol can be used to try to keep Grbl's serial receive buffer full, so that Grbl has immediate access to the next g-code line to parse and plan without having to wait for the host PC to send it. Flow control, also known as XON/XOFF software flow control, uses two special characters to tell the host PC when it has or doesn't have room in the serial receive buffer to receive more data. When there is room, usually at 20% full, the special character is sent to the host PC indicating ready-to-receive. The host PC will begin to send data until it receives the other stop-receive special character, usually at 80% full. Grbl's XON/XOFF software flow control feature may be enabled through the config.h, but is not officially supported for the following reasons.

While sound in logic, software flow control has a number of problems. The timing between Grbl and the host PC is almost never perfectly in sync, in large part due to the USB protocol and the USB-serial converter chips on every Arduino. This poses a big problem when sending and receiving these special flow-control characters. When Grbl's serial receive buffer is low, the time between when it sends the ready-to-receive character and when the host PC sends more data all depends everything in between. If the host PC is busy or the Arduino USB-serial converter is not sending the character on time, this lag can cause Grbl to wait for more serial data to come in before parsing and executing the next line of G-code. Even worse though, if the serial receive buffer is nearing full and the stop-receive character is sent, the host PC may not receive the signal in time to stop the data transfer and over-flow Grbl's serial buffer. This is bad and will corrupt the data stream.

Because the software flow-control method is dependent on the performance of the USB-serial converter on the Arduino and the host PC, the low and high watermarks for the ready-to-receive and stop-receive characters must be tuned for each case. Thus, it's not really a robust solution. In our experience with XON/XOFF software flow control, it absolutely **DOES NOT** work with Arduinos with the Atmega8U/16U USB-serial converter chips (on all current Arduinos from the Uno to Mega2560). For some reason, there are USB packet delays that are out of Grbl's control and almost always led to data corruption. However, XON/XOFF worked, but only on older Arduinos or micro controllers that featured an FTDI RS232 USB-serial converter chip, such as the Duemilanove or controllers with an FTDI break-out board. The FTDI's firmware reliably sent the XON/XOFF special characters in time and on time. We're not sure why there is such a difference between them.

If you decide to use XON/XOFF software flow control for your GUI, keep in mind that, at the moment, it'll only really works with FTDI USB-serial converters. But, the great thing about this method is that you can connect with Grbl over a serial emulator program like Coolterm, enable XON/XOFF flow control, cut-and-paste an entire g-code program into it, and Grbl will execute it completely. (Nice but not really necessary.)

#### Streaming Protocol: Character-Counting _[**Recommended with Reservations**]_

To get the best of both worlds, the simplicity and reliability of the send-response method and assurance of maximum performance with software flow control, we came up with a simple character-counting protocol for streaming a G-code program to Grbl. It works like the send-response method, where the host PC sends a line of G-code for Grbl to execute and waits for a response, but, rather than needing special XON/XOFF characters for flow control, this protocol simply uses Grbl's responses as a way to reliably track how much room there is in Grbl's serial receive buffer. An example of this protocol is outlined in the `stream.py` streaming script in our repo.

The main difference between this protocol and the others is the host PC needs to maintain a standing count of how many characters it has sent to Grbl and then subtract the number of characters corresponding to the line executed with each Grbl response. Suppose there is a short G-code program that has 5 lines with 25, 40, 31, 58, and 20 characters (counting the line feed and carriage return characters too). We know Grbl has a 127 character serial receive buffer, and the host PC can send up to 127 characters without overflowing the buffer. If we let the host PC send as many complete lines as we can without over flowing Grbl's serial receive buffer, the first three lines of 25, 40, and 31 characters can be sent for a total of 96 characters. When Grbl responds, we know the first line has been processed and is no longer in the serial read buffer. As it stands, the serial read buffer now has the 40 and 31 character lines in it for a total of 71 characters. The host PC needs to then determine if it's safe to send the next line without overflowing the buffer. With the next line at 58 characters and the serial buffer at 71 for a total of 129 characters, the host PC will need to wait until more room has cleared from the serial buffer. When the next Grbl response comes in, the second line has been processed and only the third 31 character line remains in the serial buffer. At this point, it's safe to send the remaining last two 58 and 20 character lines of the g-code program for a total of 109.

While seemingly complicated, this character-counting streaming protocol is extremely effective in practice. It always ensures Grbl's serial read buffer is filled, while never overflowing it. It maximizes Grbl's performance by keeping the look-ahead planner buffer full by better utilizing the bi-directional data flow of the serial port, and it's fairly simple to implement as our `stream.py` script illustrates. We have stress-tested this character-counting protocol to extremes and it has not yet failed. Seemingly, only the speed of the serial connection is the limit.

_UPDATE: Up until recently, we've recommended that Grbl GUIs use this streaming protocol. It is indeed very efficient and effective, but there are a couple of additional things interface writers should aware of. These are issues being worked on for the v1.0 release._
- _Since the GUI is preloading Grbl's serial RX buffer with commands, Grbl will continually execute all of the queued g-code in the RX serial buffer. The first problem is if there is an error at the beginning of the RX buffer, Grbl will continue to execute the remaining buffered g-code and the GUI won't be able to control what happens. The interim solution is to check all of the g-code via the $C check mode, so all errors are vetted prior to streaming._
- _When Grbl stores data to EEPROM, the AVR requires all interrupts to be disabled during this write process, including the serial RX ISR. This means that if a g-code or Grbl `$` command writes to EEPROM, the data sent during the write may be lost. This is usually rare and typically occurs when streaming a G10 command inappropriately inside a program. For robustness, GUIs should track and detect these EEPROM write commands and handle them appropriately by waiting for the queue to finish executing before sending more data. Note that the simple send-response protocol doesn't not suffer from this issue._


## Interacting with Grbl's Systems

Along with streaming a G-code program, there a few more things to consider when writing a GUI for Grbl, such as how to use status reporting, real-time control commands, dealing with EEPROM, and general message handling.

#### Status Reporting
When a `?` character is sent to Grbl (no additional line feed or carriage return character required), it will immediately respond with something like `<Idle,MPos:0.000,0.000,0.000,WPos:0.000,0.000,0.000>` to report its state and current position. The `?` is always picked-off and removed from the serial receive buffer whenever Grbl detects one. So, these can be sent at any time. Also, to make it a little easier for GUIs to pick up on status reports, they are always encased by `<>` chevrons.

Developers can use this data to provide an on-screen position digital-read-out (DRO) for the user and/or to show the user a 3D position in a virtual workspace. We recommend querying Grbl for a `?` real-time status report at no more than 5Hz. 10Hz may be possible, but at some point, there are diminishing returns and you are taxing Grbl's CPU more by asking it to generate and send a lot of position data.

Grbl's status report is fairly simply in organization. It always starts with a word describing the machine state like `IDLE` (descriptions of these are available elsewhere in the Wiki). The following data values are usually in the order listed below and separated by commas, but may not be in the exact order or printed at all. Report output depends on the user's `$10` status report mask setting.

* `MPos:0.000,0.000,0.000` : Machine position listed as `X,Y,Z` coordinates. Units (mm or inch) depends on `$13` Grbl unit reporting setting.
* `WPos:0.000,0.000,0.000` : Work position listed as `X,Y,Z` coordinates. Units (mm or inch) depends on `$13` Grbl unit reporting setting.
* `Buf:0` : Number of motions queued in Grbl's planner buffer.
* `RX:0` : Number of characters queued in Grbl's serial RX receive buffer.

#### Real-Time Control Commands
The real-time control commands, `~` cycle start/resume, `!` feed hold, and `^X` soft-reset, all immediately signal Grbl to change its running state. Just like `?` status reports, these control characters are picked-off and removed from the serial buffer when they are detected and do not require an additional line-feed or carriage-return character to operate.

#### EEPROM Issues
EEPROM access on the Arduino AVR CPUs turns off all of the interrupts while the CPU reads and writes to EEPROM. This poses a problem for certain features in Grbl, particularly if a user is streaming and running a g-code program, since it can pause the main step generator interrupt from executing on time. Most of the EEPROM access is restricted by Grbl when it's in certain states, but there are some things that developers need to know.

* Settings can't be streamed to Grbl with either the XON/XOFF software flow control or the character-counting streaming protocols. Only the simple send-response protocol works. This is because during the EEPROM write, the AVR CPU also shuts-down the serial RX interrupt, which means data can get corrupted or lost. Note that the send-response protocol doesn't send any data until a response comes back.

* When changing work coordinates, printing `$#` parameters, or accessing the `G28`/`G30` predefined positions, Grbl has to fetch them from EEPROM. There is a small chance this access can pause the stepper or serial receive interrupt long enough to cause motion issues, but since it only fetches 12 bytes at a time at 2 cycles per fetch, the chances are very small that this will do anything to how Grbl runs. We just suggest keeping an eye on this and report to us any issues you might think are related to this.

For reference:
* Grbl's EEPROM write commands: `G10 L2`, `G10 L20`, `G28.1`, `G30.1`, `$x=`, `$I=`, `$Nx=`, `$RST=`
* Grbl's EEPROM read commands: `G54-G59`, `G28`, `G30`, `$$`, `$I`, `$N`, `$#`

#### Message Handling
Most of the feedback from Grbl fits into nice categories so GUIs can easily tell what is what. Here's how they are organized:

* `ok`: Standard all-is-good response to a single line sent to Grbl.
* `error:`: Standard error response to a single line sent to Grbl.
* `ALARM:`: A critical error message that occurred. All processes stopped until user acknowledgment.
* `[]`: All feedback messages are sent in brackets. These include parameter and g-code parser state print-outs.
* `<>`: Status reports are sent in chevrons.

There are few things that don't fit neatly into this setup at the moment. In the next version, we'll try to make this more universal, but for now, your GUIs will need to manually account for these:
* The startup message.
* `$` Help print-out.
* `$N` Start-up blocks execution after the startup message.
* The `$$` view Grbl settings print-out.

#### G-code Error Handling

As of Grbl v0.9, the g-code parser is fully standards-compilant with complete error-checking. When a G-code parser detects an error in a G-code block/line, the parser will dump everything in the block from memory and report an `error:` back to the user or GUI. This dump can pose problematic, because the bad G-code block may have contained some valuable positioning commands or feed rate settings.

It's highly recommended to do what all professional CNC controllers do when they detect an error in the G-code program, _**halt**_. Don't do anything further until the user has modified the G-code and fixed the error in their program. Otherwise, bad things could happen.

As a service to GUIs, Grbl has a "check G-code" mode, enabled by the `$C` system command. GUIs can stream a G-code program to Grbl, where it will parse it, error-check it, and report `ok`'s and `errors:`'s without powering on anything or moving. So GUIs can pre-check the programs before streaming them for real. To disable the "check G-code" mode, send another `$C` system command and Grbl will automatically soft-reset to flush and re-initialize the G-code parser and the rest of the system. This perhaps should be run in the background when a user first loads a program, before a user sets up his machine. This flushing and re-initialization clears `G92`'s by G-code standard, which some users still incorrectly use to set their part zero.

#### Jogging

Unfortunately, Grbl doesn't have a proper jogging interface, at least for now. This was to conserve precious flash space for the development of Grbl v0.9, but may be installed in the next release of Grbl. However, authors of Grbl GUIs have come up with ways to simulate jogging with Grbl by sending incremental motions, such as `G91 X0.1`, with each jogging click or key press. This works pretty well, but, if uncontrolled, a user can easily queue up more motions than they want without realizing it and move well-past their desired location.

The general methodology that has been shown to work is to simply restrict the number of jogging commands sent to Grbl. This can be done by disabling key repeating when held down. The planner buffer queue size can be tracked such that only a handful of motions can be queued and executed.

#### Synchronization

For situations when a GUI needs to run a special set of commands for tool changes, auto-leveling, etc, there often needs to be a way to know when Grbl has completed a task and the planner buffer is empty. The absolute simplest way to do this is to insert a `G4 P0.01` dwell command, where P is in seconds and must be greater than 0.0. This acts as a quick force-synchronization and ensures the planner buffer is completely empty before the GUI sends the next task to execute.
