This directory contains 32-bit Windows command-line sample programs that use
the XBee/ZigBee Network Stack.  The included Makefile assumes you are using
MinGW/MSYS (www.mingw.org) or Cygwin (www.cygwin.com) with the "-mno-cygwin"
option to gcc.

Sample Programs
---------------
atinter.exe
Simple sample that demonstrates the xbee_atcmd API for reading and setting
AT parameters on the XBee radio.

commissioning_client.exe and commissioning_server.exe
Client and server samples demonstrating the use of the ZCL Commissioning
Cluster to configure network settings on a ZigBee device.

comports.exe
This sample doesn't use the Network Stack, but it's useful for determining
what COM ports are available.

install_ebl.exe
Sends firmware updates (.ebl files) to XBee modules with Ember processors
(e.g., ZigBee and Smart Energy firmware to XBee S2, S2B and S2C).

pxbee_ota_update.exe
Test tool for sending OTA (over-the-air) firmware updates to Programmable
XBee modules.

pxbee_update.exe
Install firmware update to a Programmable XBee module over a wired serial
connection.

transparent_client.exe
Demonstrate the "transparent serial" cluster used by XBee modules in
"AT mode", along with node discovery using the ATND command.  Send strings
between XBee modules on a network.

walker.exe
Uses ZDO requests to enumerate a device's endpoints and clusters, and then
ZCL requests to list the attribute ID, type and value for each cluster.

zcltime.exe
This utility converts a ZCL UTCTime value (32-bit number of seconds since
1/1/2000) to a "struct tm" and human-readable date string.


Building samples with MinGW/MSYS
--------------------------------
Follow the instructions at http://www.mingw.org/wiki/Getting_Started, and
install mingw-get-inst GUI installer.  Use it to install the MinGW C compiler
and MSYS.  Remember to install them to a directory path without any spaces.

Navigate to the directory with the XBee/ZigBee Network Stack and type
"make all" to make all of the samples.  Use "make strip" to strip symbols
from the EXE files and make them smaller.


Building samples with Cygwin
----------------------------
Follow the Cygwin installation instructions from the samples/posix directory.
Edit the Makefile and change the COMPILE variable to use $(COMPILE_GCC3)
instead of $(COMPILE_GCC4).  The "-mno-cygwin" option was deprecated in
gcc version 4, so you must use version 3 to compile standalone executables.
