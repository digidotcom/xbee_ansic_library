Platform Support: Windows (Win32)                     {#platform_win32}
=================================

Overview
--------
This port targets Microsoft Windows with [MinGW] and gcc.  Users of
[Cygwin] should use the POSIX platform files.

[MinGW]: http://www.mingw.org/
[Cygwin]: http://www.cygwin.org/


Building Sample Programs
------------------------
Navigate to `samples/win32` and type `make all` to build all available
sample programs.  Use `make strip` to strip debug symbols from the EXE
files and make them smaller.  Some samples make use of shared source code
in `samples/common`.  Read the `Makefile` to see which source files go
into each sample.


Building samples with MinGW/MSYS
--------------------------------
Follow the [MinGW/MSYS installation instructions][1], to install the
`mingw-get-inst` GUI installer.  Use it to install the MinGW C compiler
and MSYS.  Remember to install them to a directory path without any
spaces.

[1]: http://www.mingw.org/wiki/Getting_Started
