# AudioSwitch1
Firmware for AudioSwitch v.1.

Audioswitch is a Arduino project to switch three audio inputs into one output.
All of inputs are switched with buttons or with infrared remote control too.
Audioswitch can learn to respond to the codes of different types of remote controls.

The program is written for ATtiny1624. It is necessary to install support for MegaTinyCore microcontrollers in the Arduino IDE: https://github.com/SpenceKonde/megaTinyCore

In the file library.zip there is a library for controlling signal decoding from IRC, which must be unpacked into the library folder for Arduino, e.g. Arduino/library
