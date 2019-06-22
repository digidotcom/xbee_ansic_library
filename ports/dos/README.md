Platform Support: DOS
=====================

Overview
--------
Not actively maintained.  Serial driver only supports COM1.

Originally created for use on an embedded handheld device running MS-DOS
6.2.  Use [Open Watcom C 1.9] compiler to build a library that you can
link into your programs.

When installing Open Watcom, be sure to include the 16-bit compiler.
By default, `xbee_driver.wpj` builds a 16-bit, large memory model,
80386-targeted library.

[Open Watcom C 1.9]: https://sourceforge.net/projects/openwatcom/files/open-watcom-1.9/


Building xbee_driver.lib
------------------------
Open `ports/dos/xbee_driver.wpj` in the Open Watcom IDE and choose "Make
All (F5)" from the Actions menu (or the rightmost icon in the toolbar) to
create `xbee_driver.lib` and `xbee_driver.lb1` in `lib/dos/`.  You can
then link that library into your programs.

Be sure the compilation options (processor and memory model) for your
programs match the ones for the library (currently 80386/large).  You can
probably compile many of the POSIX samples in Open Watcom with minimal
changes.


Building Sample Programs
------------------------
Open `samples/dos/samples.wpj` in Open Watcom and "Make All" to create
`serial.exe`, a simple program for testing the serial port, and
`atinter.exe`, a console for sending AT commands in API frames to an
XBee module connected to the serial port.  Note that the serial driver
was written for COM1 and will require additional effort to support
COM2 through COM4.
