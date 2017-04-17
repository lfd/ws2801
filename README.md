WS2801 Linux driver
===================

Many WS2801 libraries are written in Python or are special embedded libraries
for Arduinos or AVRs.  This driver is a Linux driver, and support arbitrary
GPIOs and does not depend on SPI devices.  This makes the driver flexible.

Userspace driver
----------------

The userspace driver uses the gpiolib to access GPIOs.

How to write an application
---------------------------

Have a look at the demo code under 'demos/'.  Code should be pretty self explanatory.

Driver modes
------------

The driver will only commit data on LED stripes if required.  Therefore, the
driver supports two modes: normal mode and auto-commit mode.  In normal mode,
the user has to decide when to commit data to the LED stripes by calling the
commit() method.  In auto-commit mode, the driver automatically commits data on
any change.  This can cause unintended effects in some cases.

LEDs might flicker or go crazy, when no more data arrives, so the driver
implements a refresh-rate parameters.  This parameter forces the driver to
commit data to the LED stripe in case that no other communication is ongoing.

Build & Run
-----------

To compile userspace drivers and demos, just type

    make

in the root directory.  Run the demo:

    ./demos/ws2801-demo -n 40 -c 21 -d 22

This tells the demo that your LED stripe is equipped with 40 LEDs, the clock
line is located at GPIO 21 and the data line at GPIO 22.

TODOs
-----

  - Implement gamma correction (on driver side).
  - Implement kernel driver
  - Add more demo code
