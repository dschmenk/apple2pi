apple2pi
========

Apple II client/server for Raspberry Pi
---------------------------------------

Apple II Pi works by connecting an Apple II to a Raspberry Pi using a RS232 serial connection.  In order to get the Raspberry Pi to talk RS232 from it's 3.3V GPIO serial port, you will need to build or buy a converter.  They are very cheap on eBay, so I would recommend going that route. Alternatively, you can use a USB serial port converter, but you will need to know its tty device name. To ensure you've hooked the converter up correctly, try loggin into the Raspberry Pi from another modern-ish computer.  Raspbian, the default Debian based Linux for the Raspberry Pi, opens up a login (getty) session on the serial port at 115.2K baud.  You will probably need a null modem or cross-over cable to login from another computer.  Once it all checks out, time to connect your Apple II.  All the 3.3V converters and USB serial ports I see have a DB-9 connector and many of the Apple II era connectors are DB-25 so you may need a DB-9 to DB-25 converter.

Installing and configuring the Apple II:  You will need an Apple //c or Apple //e w/ SuperSerial Card.  An Apple II Mouse is recommended for that full-on retro feel, but not required.  Download and install the A2PI.PO disk image onto a 5 1/4 floppy.  ADTPro would be the recommended tool for that operation although once you have the latest apple2pi version running, you can use the included dskwrite and dskread utilities for writing and reading ProDOS floppies.

Installing and configuring the Raspberry Pi:  Download the apple2pi project to your Raspberry Pi.  Enter the apple2pi/src directory.  Compile the daemon and tools with 'make' and copy the results to /usr/local/bin with 'sudo make install'.  To build the FUSE driver needed to mount ProDOS devices under Linux, you will need the libfuse-dev package installed.  Get thris from apt-get, aptitude, or whichever package manager you like.  Build with 'make fusea2pi' and install with 'sudo make fuse-install'.

The following is no longer neessary as the install script carefully makes all the following adjustments automatically.  I left this here so you know what the script is doing.

<comment>

You will need to disable the Raspbain serial login by editing /etc/inittab and commenting out the line (probably at the very bottom) to look like:<br>
<code>
\#T0:23:respawn:/sbin/getty -L ttyAMA0 115200 vt100
</code>
<br>
You will also want to disable the console messages that go out to the serial port in the /boot/cmdline.txt file.  Remove the "console=" clause and the "kgdboc=" clause from the /boot/cmdline.txt file.  Mine looks like:<br>
<code>
dwc_otg.lpm_enable=0 console=tty1 root=/dev/mmcblk0p2 rootfstype=ext4 elevator=deadline rootwait
</code>

If you are using an HDMI port to display video, skip the overscan settings.  This was for my monitor and your values may vary (a lot). I adjust the NTSC output so it fits nicely on my //c monitor, I edited the setting in /boot/config.txt such that:

<code>
overscan_left=26<br>
overscan_right=-8<br>
overscan_top=-8<br>
\#overscan_bottom=16<br>
</code>

To run the a2pid daemon automatically at boot time, edit /etc/rc.local and add:

<code>
/usr/local/bin/a2pid --daemon
</code>

right before the line:

<code>
exit 0
</code>

</comment>

followed by rebooting the Raspberry Pi.

NOTE - For USB serial port users and non-Raspberry Pi owners:  This isn't actually tied to the Raspberry Pi in any way, except for the default serial port used to connect the Pi to the Apple II.  On most up-to-date Linux distributions, you should be able to build all the files.  To run the daemon on a specific serial port, just add it as a command line option i.e. a2pid --daemon /dev/ttyUSB0

Reboot the Apple II with the newly created floppy in the start-up drive.  If everything is configured correctly, you should be able to login to the Raspberry Pi with your Apple II keyboard.  If you have an Apple II Mouse, that should control the cursor in X, or in the console if you have gdm installed.

Using a2pi: The Apple //c and //e keyboards are pretty minimal compared to modern keyboards, and the Apple II Mouse only has one button.  In order to provide most of the funcitonality required of modern OSes, the Open-Apple and Closed-Apple keys are used as modifiers to enhance the keyboard and mouse.  On the keyboard, Open-Apple acts just like the Alt key.  The Closed-Apple key acts like a Fn key, changing the actual key codes.  Currently, the Closed-Apple key will modify the number keys 1-0 as funciton keys F1-F10 and the arrow keys as Left-Arrow=Home, Right-Arrow=End, Up-Arrow=PgUp, Down-Arrow=PgDn.  For the mouse, when you click the mouse button by itself, that is the left(default)-click.  Open-Apple along with the mouse button will return the right-click, and Closed-Apple along with the mouse button will return the middle-click.  If you should ever need to exit a2pi, press Closed-Apple ESC on the Apple II keyboard.  This will exit both the code on the Apple II and the Raspberry Pi.  This is useful when developing and debugging the drivers/daemons.  a2pid can be run directly (not as a daemon) by leaving off the '--daemon' option.  Enabling printf's in the code allows one to watch the packets arrive and get processed when run from a network ssh session.

Theory of operation:  Apple II Pi works by running code on the Apple II and the Raspberry Pi, talking to each other with a simple protocol.  The Apple II basically appears to the Raspberry Pi as an external peripheral, not unlike a USB keyboard and mouse.  The Apple II floppy boots into ProDOS and runs a simple machine language program that scans the keyboard, and mouse if present, sending the events out the serial port to the Raspberry Pi.  It is a very simple protocol and the serial port is running at 115.2K baud, so it is fast and low overhead.  On the Raspberry Pi, a little daemon runs, waiting for packets on the serial port, converts the Apple II events into Linux compatible events, and inserts them into the input subsystem.  This daemon also has a socket interface (port 6502) that can be used to access the Apple II memory and execute arbitrary code.  Look at a2lib.c for implementation.

Enjoy,

Dave Schmenk
