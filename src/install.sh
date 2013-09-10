#!/bin/bash

#
# Update /boot partition files.
#
if [ ! -f /boot/cmdline.bak ] ; then
	mv /boot/cmdline.txt /boot/cmdline.bak
	echo "dwc_otg.lpm_enable=0 console=tty1 root=/dev/mmcblk0p2 rootfstype=ext4 elevator=deadline rootwait" > /boot/cmdline.txt
fi
#
# Disable getty on built-in serial port.
#
gettyline=`grep ttyAMA0 /etc/inittab`
if [ -n "$gettyline" -a ${gettyline:0:1} = T ] ; then
	mv /etc/inittab /etc/inittab.bak
	sed 's/^T0*/#T0*/' /etc/inittab.bak > /etc/inittab
fi
#
# Add a2serclk and a2pid to rc.local
#
if [ -f /etc/rc.local ] ; then
	if ! grep a2pid /etc/rc.local > /dev/null ; then
		mv /etc/rc.local /etc/rc.local.bak
		sed -e '/^exit/i\# Start Apple II Pi' -e '/^exit/i\/usr/local/bin/a2serclk' -e '/^exit/i\/usr/local/bin/a2pid --daemon' /etc/rc.local.bak > /etc/rc.local
		chmod +x /etc/rc.local
	fi
fi
#
# Disable joystick as a mouse in X
#
cp 11-joy.conf /usr/share/X11/xorg.conf.d
#
# Create link to new joystick driver for gsportx
#
if [ ! -e /dev/js0 ] ; then
	ln -s /dev/input/js0 /dev/js0
fi
