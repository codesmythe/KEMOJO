
# KEMOJO (Keyboard / Mouse / Joystick)

![KEMOJO](docs/KEMOJO_V1A.png)

This board provides PS/2 keyboard, PS/2 mouse and Atari or MSX (single
button only) joystick support for an RCBus system. It uses the 40-pin
RCBus interface. 

The keyboard, mouse and joystick inputs are read by an ATmega 328P
microcontoller. (This is the same MCU that is in the Arduino UNO.) The
ATmega implements the [Atari ST IKBD
protcol](https://archive.org/details/Intelligent_Keyboard_ikbd_Protocol_Feb_26_1985/)
and communicates that protocol serially to an RCBus system via a
16C550 UART.

## Acknowledgements

The low-level PS/2 code watches the PS/2 keyboard and mouse clock
lines for transitions and signals an interrupt the ATmega. This code
then reads in PS/2 bytes and buffers them. It was written by Daniel
Cliche (@danodus) and is made available by him under an MIT license
from [here](https://github.com/danodus/rosco_m68k_io).

The higher level code that implements the Atari ST IKDB protocol is
borrowed from the [Hatari](https://www.hatari-emu.org) Atari ST
emulator. It is made available under the GNU GPL, version 2 or higher. 

The overall project thus uses the GNU GPL, version 2 or higher.
