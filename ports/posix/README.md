Platform Support: POSIX                               {#platform_posix}
=======================

Overview
--------
This port targets [POSIX] operating systems (Windows/Cygwin, macOS, Linux,
BSD, etc.) with `gcc`.  This overview assumes you are already familiar
with compiling command-line programs from a shell prompt.

[POSIX]: https://en.wikipedia.org/wiki/POSIX


Building with Linux
-------------------
Install the necessary packages for gcc, gdb and make.  We have tested
this code on an Ubuntu desktop and a Raspberry Pi Zero W V1.1.


Building for Digi International's ConnectCore Products
------------------------------------------------------
Look at `ports/digiapix`, for support of Digi International's embedded
products using libdigiapix for GPIO control.


Building with macOS
-------------------
Install Xcode to get LLVM with a gcc wrapper and GNU Make.


Installing Cygwin for Windows development
-----------------------------------------
To build standalone, Win32-native executables, use the Win32 platform with
MinGW and MSYS (see `ports/win32/README.md`).  This is the recommended
development path for Windows, but you can alternatively use the POSIX
platform by installing Cygwin.

Cygwin is a POSIX environment for Windows systems. It provides a bash
shell and supports many familiar packages from Unix-like operating systems
(like macOS, Linux and OpenBSD). Here are instructions for installing
Cygwin on your PC in order to build and run the XBee sample applications.

Go to <http://cygwin.com/install.html> and download `setup-x86_64.exe`
for 64-bit Windows intallations (or `setup-x86.exe` for 32-bit).

Launch the setup executable and accept the default settings:

- Install from Internet
- Install to c:\cygwin64 (64-bit) or c:\cygwin (32-bit) for all users
- Use c:\Windows\Temp (or some other temp directory) as the Local Package
  Directory
- Choose a few mirrors for downloading

Select the following packages:

    gcc-core, gcc-g++, binutils, gdb, make

Allow Cygwin to install any dependencies for those packages.

Once installed, launch "Cygwin Bash Shell" and navigate to the directory
where you installed the library. Note that `/cygdrive/c` is the path
to get to your C drive. Go into the `samples/posix` directory and type
`make all`. If you've installed everything correctly, it should build
all of the sample programs.

To access your serial ports, reference /dev/ttySXXX where XXX is the COM
port number minus 1 (e.g., COM31 is /dev/ttyS30).

Note that the executable files you build require support DLL files to
run on systems without Cygwin installed. At minimum, you'll need
`cygwin1.dll` in the same directory as the executable, and possibly
`cyggcc_s-1.dll`. If you're missing a DLL, you'll get a warning with the
missing DLL file's name when you try to launch the program.
