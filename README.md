# Apple II Pi

1. See the [upstream repo](https://github.com/dschmenk/apple2pi) for the original, full info.

2. This fork is based on [A2Pico](https://github.com/oliverschmidt/a2pico).

## Installing

1. Flash [Apple-II-Pi.uf2](https://github.com/oliverschmidt/apple2pi/releases/latest/download/Apple-II-Pi.uf2) to the A2Pico.

2. Connect the [Raspberry Pi Zero 2 W](https://www.raspberrypi.com/products/raspberry-pi-zero-2-w/) to the A2Pico via a USB OTG cable.

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
