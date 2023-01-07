## About

dhteensy is a hardware hack and a new firmware for Datahand keyboards
using the Teensy++ development board to add native USB support and
full layout customization.

## Hardware hack

I got the idea from Glenn Boyer's geekhack.org post (see below) and
decided to implement something very similar. The idea is to use a
Teensy++ board and hook up the VCC, GND and the required data pins to
the CPU socket on the Datahand's main circuit board in place of the
original 8051 clone CPU.

The Teensy++ is a very nice AVR based board similar to Arduino that
makes it easy to implement USB HID devices. It has plenty of I/O pins
and it is the same size as the original CPU used in the Datahand so
it's easy to make a converter.

## Firmware

The firmware is written in C using the AVR gcc port. The Makefile and
the USB HID code came from the examples provided by PJRC for the
Teensy++. The current firmware is very simple and doesn't have all the
functionality of the original and it is somewhat customized to my own
preferences. It has some assumptions about which Teensy pins are used
to access the Datahand, this should be easy to change if you implement
the converter differently.

## Inspiration
http://geekhack.org/showwiki.php?title=Island:12212
http://github.com/JesusFreke/DHFirm
