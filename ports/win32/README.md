Platform Support: Windows (Win32)                     {#platform_win32}
=================================

Overview
--------
This port targets Microsoft Windows with [MinGW] or [Mingw-w64] and gcc.
Users of [Cygwin] or [MSYS2] (without Mingw-w64) should use the POSIX
platform files.

[MinGW]: http://www.mingw.org/
[Mingw-w64]: http://mingw-w64.org/
[Cygwin]: http://www.cygwin.org/
[MSYS2]: https://www.msys2.org/


Building Sample Programs
------------------------
Navigate to `samples/win32` and type `make all` to build all available
sample programs.  Use `make strip` to strip debug symbols from the EXE
files and make them smaller.  Some samples make use of shared source code
in `samples/common`.  Read the `Makefile` to see which source files go
into each sample.


Building samples with MSYS2/Mingw-w64
-------------------------------------
It should be possible to just install Mingw-w64 without MSYS2, but these
instructions assume an MSYS2 installation and use of a `mintty` console to
a 32-bit or 64-bit MinGW environment with a `bash` shell.

Follow the [directions to install MSYS2 64-bit][1], and then install
multiple packages using `pacman` from an MSYS2 shell:

```
$ pacman -S msys2-w32api-headers git make gcc
$ pacman -S mingw-w64-x86_64-headers-git mingw-w64-i686-headers-git
$ pacman -S --needed base-devel mingw-w64-i686-toolchain
$ pacman -S --needed base-devel mingw-w64-x86_64-toolchain
$ pacman -S winpty
```

If using an MSYS2 mintty connection, work from `samples/posix` and refer
to serial ports as `/dev/ttySnn` (where nn is COM port number minus one).
For example, `./atinter /dev/ttyS3`.  Programs built this way will only
work from a Windows Command Prompt if `msys-2.0.dll` is available.

If using a MinGW 32-bit or 64-bit mintty console, work from `samples/win32`
but run programs with `winpty`.  For example, `winpty ./atinter COM4`.
Programs that don't reference `<conio.h>` or link `xbee_readline.o` should
work without `winpty`.  Programs built this way will work standalone in a
Windows Command Prompt without any DLL dependencies (e.g.,
`C:\>atinter COM4`).

[1]: https://www.msys2.org


Building samples with MSYS/MinGW
--------------------------------
We recommend MSYS2/Mingw-w64, but here's how to use MSYS/MinGW.

Follow the [MinGW/MSYS installation instructions][2], to install the
`mingw-get-inst` GUI installer.  Use it to install the MinGW C compiler
and MSYS.  Remember to install them to a directory path without any
spaces.

[2]: http://www.mingw.org/wiki/Getting_Started
