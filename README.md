# AudioSwitch1
> What is it

Audioswitch is an Arduino project to switch three audio inputs into one output.
All of inputs are switched with buttons or with infrared remote control too.
Audioswitch can learn to respond to the codes of different types of remote controls.

> Microcontroler

The program is written for microcontroller ATtiny1624. It is necessary to install support for MegaTinyCore family microcontrollers in the Arduino IDE: https://github.com/SpenceKonde/megaTinyCore

> Used Libraries

In the file **library.zip** there is a library IRRemote for controlling signal decoding from IRC, which must be unpacked into the  Arduino library folder, e.g. Arduino/library
I made modifications in the library, added correct support for ATtiny3226 and ATtiny1624 microcontroller. 
Original source of library: https://github.com/Arduino-IRremote/Arduino-IRremote
