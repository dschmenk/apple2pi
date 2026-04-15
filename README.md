# Apple II Pi

1. See the [upstream repo](https://github.com/dschmenk/apple2pi) for the original, full info.

2. This fork is based on [A2Pico](https://github.com/oliverschmidt/a2pico).

## Installing

1. Flash the correct [.uf2 file](https://github.com/oliverschmidt/apple2pi/releases/latest/) to the A2Pico or A2Pico2Lite.

2. Connect the [Raspberry Pi Zero 2 W](https://www.raspberrypi.com/products/raspberry-pi-zero-2-w/) to the A2Pico or A2Pico2Lite via a USB OTG cable.
What is the Apple II Pi?
------------------------
Basically, the Apple II Pi is the integration of an Apple II with a Raspberry Pi (http://www.raspberrypi.org), or any modern Linux computer to create a hybrid computer combining the input devices and storage mediums (even downloadable code) of the Apple with the CPU, GPU (graphical processing unit), USB, network capabilities, etc. of Linux.  The concept is to create an updated version of the Apple II using some imagination, low-level drivers, off-the-shelf hardware, and a closely coupled communications channel; thus bringing modern compute and Linux software to the Apple II platform.  The Apple II is running as a dedicated I/O processor for Linux under ProDOS.  Much like the PC Transporter card brought MS-DOS and the Z-80 card brought CP/M, the Apple II Pi brings Linux to the Apple II using the Apple’s input devices and the Linux machine’s video output.  As such, knowledge and familiarity with Linux is required to get the most out of its environment.  Under Linux, the Apple II Pi can read and write the Apple’s storage devices (floppies/harddrives/CFFA) and also run the GSport Apple IIgs emulator (http://gsport.sourceforge.net).  Together, GSport and Apple II Pi provide an immersive environment providing access to most of the Apple II hardware you own plus an accelerated 65816 with up to 8 MB RAM, and all the disk images you can fit on your Linux machine.

Apple II server for the Raspberry Pi
------------------------------------

Apple II Pi works by connecting an Apple II to the Raspberry Pi using a RS232 serial connection.  In order to get the Raspberry Pi to talk RS232 from it's 3.3V GPIO serial port, you will need to build or buy a converter.  They are very cheap on eBay, so I would recommend going that route. Alternatively, you can use a USB serial port converter, but you will need to know its tty device name. To ensure you've hooked the converter up correctly, try logging into the Raspberry Pi from another modern-ish computer.  RaspberryPi OS, the default Debian based Linux for the Raspberry Pi, opens up a login (getty) session on the serial port at 115.2K baud.  You will probably need a null modem or cross-over cable to login from another computer.  Once it all checks out, time to connect your Apple II.  All the 3.3V converters and USB serial ports I see have a DB-9 connector and many of the Apple II era connectors are DB-25 so you may need a DB-9 to DB-25 converter.

Apple II client
---------------

Installing and configuring the Apple II:  You will need an Apple //c or Apple ][, //e, IIgs with a SuperSerial Card.  An Apple ][ requires the SHIFT key mod.  An Apple II Mouse is recommended for that full-on retro feel, but not required.  Download and install the A2PI.PO disk image onto a 5 1/4 floppy.  ADTPro would be the recommended tool for that operation although once you have the latest apple2pi version running, you can use the included dskwrite and dskread utilities for writing and reading ProDOS floppies.

Building DEB file from source
-----------------------------

Clone or download package ZIP file onto your Debian based Linux distro. Enter `apple2pi` directory.

Install required build packages: `sudo apt install libfuse-dev pbuilder debhelper`

Build A2Pi package: `make deb`

Install A2Pi DEB package: `sudo dpkg -i a2pi-xxx.xxx.deb` (replace xxx.xxx to whatever the current build version is)

The default tty device that the a2pid deamon connects to is now set to be `/dev/ttyUSB0`. This can be changed in the `/etc/defaults/a2pi` file. This is rarely needed unless connecting to the Pi's GPIO serial port or USB<->RS232 dongles that show up as something like ttyAMA0.

You may have to reboot for changes to take effect.

Installing and configuring the Raspberry Pi the hard way
--------------------------------------------------------

Download the apple2pi project to your Raspberry Pi.  Enter the apple2pi/src directory.  Compile the daemon and tools with 'make' and copy the results to /usr/local/bin with 'sudo make install'.  To build the FUSE driver needed to mount ProDOS devices under Linux, you will need the libfuse-dev package installed.  Get this from apt-get, aptitude, or whichever package manager you like.  Build with 'make fusea2pi' and install with 'sudo make fuse-install'.

3. Activate the USB Power on the A2Pico.

4. Connect the HMDI monitor to the Raspberry Pi Zero 2 W:

![Setup](https://github.com/oliverschmidt/apple2pi/assets/2664009/ac5e954a-3c80-4ab0-974b-b3e2394cd747)

5. Install [Raspberry Pi OS](https://www.raspberrypi.org/software/).

6. Execute
   ```
   sudo apt install git libfuse-dev -y
   git clone https://github.com/oliverschmidt/apple2pi.git
   cd apple2pi
   make
   sudo make install
   ```

7. Execute `sudo systemctl start a2pi.service` to run the Apple II Pi Daemon right away.

## Using

1. Start Apple II Pi either via cold boot or `PR#n`.

2. Make sure to check out `a2term` in a Raspberry Pi shell.

## Building

1. Build the project in `/pipico` (requires the [Raspberry Pico C/C++ SDK](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html)).

2. Execute `make` in `/src` (requires [libfuse-dev](https://packages.debian.org/en/sid/libfuse-dev)).
Reboot the Apple II with the newly created floppy in the start-up drive.  If everything is configured correctly, you should be able to login to the Raspberry Pi with your Apple II keyboard.  If you have an Apple II Mouse, that should control the cursor in X, or in the console if you have gdm installed.


Using A2Pi
----------

The Apple //c and //e keyboards are pretty minimal compared to modern keyboards, and the Apple II Mouse only has one button.  In order to provide most of the funcitonality required of modern OSes, the Open-Apple and Closed-Apple keys are used as modifiers to enhance the keyboard and mouse.  On the keyboard, Open-Apple acts just like the Alt key.  The Closed-Apple key acts like a Fn key, changing the actual key codes.  Currently, the Closed-Apple key will modify the number keys 1-0 as funciton keys F1-F10 and the arrow keys as Left-Arrow=Home, Right-Arrow=End, Up-Arrow=PgUp, Down-Arrow=PgDn.  For the mouse, when you click the mouse button by itself, that is the left(default)-click.  Open-Apple along with the mouse button will return the right-click, and Closed-Apple along with the mouse button will return the middle-click.  a2pid can be run directly (not as a daemon) by leaving off the '--daemon' option.  Enabling printf's in the code allows one to watch the packets arrive and get processed when run from a network ssh session.

Theory of operation
-------------------
Apple II Pi works by running code on the Apple II and the Raspberry Pi, talking to each other with a simple protocol.  The Apple II basically appears to the Raspberry Pi as an external peripheral, not unlike a USB keyboard and mouse.  The Apple II floppy boots into ProDOS and runs a simple machine language program that scans the keyboard, and mouse if present, sending the events out the serial port to the Raspberry Pi.  It is a very simple protocol and the serial port is running at 115.2K baud, so it is fast and low overhead.  On the Raspberry Pi, a little daemon runs, waiting for packets on the serial port, converts the Apple II events into Linux compatible events, and inserts them into the input subsystem.  This daemon also has a socket interface (port 6551) that can be used to access the Apple II memory and execute arbitrary code.  Look at a2lib.c for implementation.

Enjoy,

Dave Schmenk
