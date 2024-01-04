# Apple II Pi

See the [upstream repo](https://github.com/dschmenk/apple2pi) for the original, full info.

## Installing

1. Flash [Apple-II-Pi.uf2](https://github.com/oliverschmidt/apple2pi/releases/latest/download/Apple-II-Pi.uf2) to the Raspberry Pi Pico.

2. Install [Raspberry Pi OS](https://www.raspberrypi.org/software/).

3. Execute
   ```
   sudo apt install git libfuse-dev -y
   git clone https://github.com/oliverschmidt/apple2pi.git
   cd apple2pi
   make
   sudo make install
   ```

4. Execute `sudo systemctl start a2pi.service` to run the Apple II Pi Daemon right away.

## Using

1. Start Apple II Pi either via cold boot or `PR#n`.

2. Make sure to check out `a2term` in a Raspberry Pi shell.

## Building

1. Build the project in `/pipico` (requires the [Raspberry Pico C/C++ SDK](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html)).

2. Execute `make` in `/src` (requires [libfuse-dev](https://packages.debian.org/en/sid/libfuse-dev)).
