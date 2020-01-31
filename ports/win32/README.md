Platform Support: Windows (Win32)                     {#platform_win32}
=================================

Overview
--------
This port targets Microsoft Windows with [MinGW] and gcc.  Users of
[Cygwin] can make use of the POSIX port or modify the `Makefile` in
`samples/win32` to use `gcc` with the `-mno-cygwin` option.

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


Building samples with Cygwin
----------------------------
Follow the Cygwin installation instructions from `ports/posix/README.md`.

If you want standalone executables that run without Cygwin DLLs, you need
to use `gcc` version 3 with the `-mno-cygwin` command-line option (which
was deprecated in version 4).  Edit `samples/win32/Makefile` and change
the `COMPILE` variable to use `$(COMPILE_GCC3)` instead of
`$(COMPILE_GCC4)`.
