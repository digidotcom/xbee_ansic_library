This directory contains POSIX command-line sample programs that use
the XBee/ZigBee Network Stack.  They have been tested on Mac OS X and
Cygwin (Windows).

Sample Programs
---------------
atinter
Simple sample that demonstrates the xbee_atcmd API for reading and setting
AT parameters on the XBee radio.

commissioning_client and commissioning_server
Client and server samples demonstrating the use of the ZCL Commissioning
Cluster to configure network settings on a ZigBee device.

install_ebl
Sends firmware updates (.ebl files) to XBee modules with Ember processors
(e.g., ZigBee and Smart Energy firmware to XBee S2, S2B and S2C).

pxbee_ota_update
Test tool for sending OTA (over-the-air) firmware updates to Programmable
XBee modules.

pxbee_update
Install firmware update to a Programmable XBee module over a wired serial
connection.

transparent_client
Demonstrate the "transparent serial" cluster used by XBee modules in
"AT mode", along with node discovery using the ATND command.  Send strings
between XBee modules on a network.

walker
Uses ZDO requests to enumerate a device's endpoints and clusters, and then
ZCL requests to list the attribute ID, type and value for each cluster.

zcltime
This utility converts a ZCL UTCTime value (32-bit number of seconds since
1/1/2000) to a "struct tm" and human-readable date string.


Installing Cygwin for 32-bit Windows development
------------------------------------------------
Cygwin is a POSIX environment for Windows systems. It provides a bash
shell and supports many familiar packages from Unix-like operating systems
(like Mac OS X, Linux and OpenBSD). Here are instructions for installing
Cygwin on your PC in order to build and run the XBee sample applications.

Go to http://cygwin.com/install.html and download setup.exe.

Launch setup.exe and accept the default settings:

- Install from Internet
- Install to c:\cygwin for all users
- Use c:\Windows\Temp (or some other temp directory) as the Local Package
  Directory
- Choose a few mirrors for downloading

Select the following packages:

gcc, gcc-core, gcc-g++, gcc4, gcc4-core, gcc4-g++, binutils, gdb, make

Allow cygwin to install any dependencies for those packages.

Once installed, launch "Cygwin Bash Shell" and navigate to the directory
where you installed xbee_driver.zip. Note that "/cygdrive/c" is the path
to get to your C drive. Go into the "samples/win32" directory and type
"make all". If you've installed everything correctly, it should build
all of the sample programs.

Note that the executable files you build require support DLL files to
run on systems without Cygwin installed. At minimum, you'll need
cygwin1.dll in the same directory as the executable, and possibly
cyggcc_s-1.dll. If you're missing a DLL, you'll get a warning with the
missing DLL file's name when you try to launch the program.

To build standalone, Win32-native executables, use the Win32 platform with
MinGW and MSYS (see samples/win32/readme.txt).
