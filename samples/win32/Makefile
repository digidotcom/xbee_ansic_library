# Makefile for XBee Driver on Win32

# Uses MinGW or Cygwin's gcc with "-mno-cygwin" option.

# Autodepend methods from http://make.paulandlesley.org/autodep.html

# If you get a "no rule to make target" error for some random .h file, try
# deleting all .d files.

# extra defines (in addition to those in platform_win32.h)
DEFINE = -DZCL_ENABLE_TIME_SERVER \
	-DXBEE_DEVICE_ENABLE_ATMODE \
	-DXBEE_XMODEM_TESTING

# directory for driver
DRIVER = ../..

# path to include files
INCDIR = $(DRIVER)/include

# path to common source files
SRCDIR = $(DRIVER)/src

# libs for samples using GetOpenFileName
DIALOG_LIBS = -lkernel32 -lcomdlg32

# compiler parameters for building each file
# -MMD generates dependency files automatically, omitting system files
# -MP creates phony targets for each prerequisite in a .d file

# use gcc 4 with MinGW (will not work with Cygwin)
COMPILE_GCC4 = gcc -iquote$(INCDIR)

# use gcc 3 in Cygwin with -mno-cygwin option to build standalone executables
COMPILE_GCC3 = gcc-3 -mno-cygwin -I$(INCDIR)

COMPILE = $(COMPILE_GCC4) -g -MMD -MP -Wall $(DEFINE)

EXE = atinter.exe install_ebl.exe comports.exe \
		pxbee_ota_update.exe zcltime.exe \
		commissioning_client.exe commissioning_server.exe \
		apply_profile.exe xbee_term.exe eblinfo.exe \
		zigbee_walker.exe transparent_client.exe pxbee_update.exe

all : $(EXE)

# strip debug information from executables
strip :
	strip $(EXE)

comports.exe : comports.c
	$(COMPILE) -o $@ $^

zcltime.exe : zcltime.o
	$(COMPILE) -o $@ $^

eblinfo.exe : eblinfo.o xbee_ebl_file.o swapbytes.o
	$(COMPILE) -o $@ $^

clean :
	- rm -f *.o *.d $(EXE)

SRCS = \
		atinter.c \
		eblinfo.c \
		install_ebl.c \
		transparent_client.c \
		pxbee_ota_update.c \
		xbee_term.c _xbee_term.c xbee_term_win32.c \
		zcltime.c \
		parse_serial_args.c \
	_atinter.c _pxbee_ota_update.c \
	_commission_client.c \
	_commission_server.c _nodetable.c _zigbee_walker.c \
	xbee_platform_win32.c \
	xbee_serial_win32.c \
	xbee_atcmd.c \
	xbee_atmode.c \
	xbee_cbuf.c \
	xbee_device.c \
	xbee_discovery.c \
	xbee_ebl_file.c \
	xbee_firmware.c \
	xbee_ota_client.c \
	xbee_platform_win32.c \
	xbee_serial_win32.c \
	xbee_time.c \
	xbee_transparent_serial.c \
	xbee_xmodem.c \
	zcl_basic.c \
	zcl_client.c \
	zcl_identify.c \
	zcl_time.c \
	zcl_types.c \
	zigbee_zcl.c \
	zigbee_zdo.c \
	wpan_types.c \
	xmodem_crc16.c \
	hexstrtobyte.c \
	swapbytes.c \
	memcheck.c \
	hexdump.c

base_OBJECTS = xbee_platform_win32.o xbee_serial_win32.o hexstrtobyte.o \
					memcheck.o swapbytes.o swapcpy.o hexdump.o parse_serial_args.o

xbee_OBJECTS = $(base_OBJECTS) xbee_device.o xbee_atcmd.o wpan_types.o

wpan_OBJECTS = $(xbee_OBJECTS) wpan_aps.o xbee_wpan.o

zigbee_OBJECTS = $(wpan_OBJECTS) zigbee_zcl.o zigbee_zdo.o zcl_types.o

atinter_OBJECTS = _atinter.o

atinter.exe : $(xbee_OBJECTS) $(atinter_OBJECTS) atinter.o
	$(COMPILE) -o $@ $^

apply_profile.exe : $(xbee_OBJECTS) apply_profile.o
	$(COMPILE) -o $@ $^

xbee_term_OBJECTS = $(base_OBJECTS) xbee_term.o _xbee_term.o xbee_term_win32.o
xbee_term.exe : $(xbee_term_OBJECTS)
	$(COMPILE) -o $@ $^

transparent_client_OBJECTS = $(zigbee_OBJECTS) transparent_client.o \
	$(atinter_OBJECTS) _nodetable.o \
	xbee_discovery.o \
	xbee_transparent_serial.o

transparent_client.exe : $(transparent_client_OBJECTS)
	$(COMPILE) -o $@ $^

install_ebl_OBJECTS = $(xbee_OBJECTS) install_ebl.o \
	xbee_atmode.o \
	xbee_firmware.o \
	xbee_xmodem.o xmodem_crc16.o

install_ebl.exe: $(install_ebl_OBJECTS)
	$(COMPILE) -o $@ $^ $(DIALOG_LIBS)

pxbee_update_OBJECTS = $(xbee_OBJECTS) pxbee_update.o \
	xbee_atmode.o \
	xbee_firmware.o \
	xbee_xmodem.o xmodem_crc16.o
pxbee_update.exe: $(pxbee_update_OBJECTS)
	$(COMPILE) -o $@ $^ $(DIALOG_LIBS)

pxbee_ota_update_OBJECTS = $(zigbee_OBJECTS) \
		pxbee_ota_update.o \
		$(atinter_OBJECTS) _pxbee_ota_update.o \
	xbee_cbuf.o \
	xbee_ota_client.o \
	xbee_transparent_serial.o \
	xbee_xmodem.o xmodem_crc16.o \
	zcl_client.o zcl_identify.o xbee_time.o

pxbee_ota_update.exe : $(pxbee_ota_update_OBJECTS)
	$(COMPILE) -o $@ $^ $(DIALOG_LIBS)

zigbee_walker_OBJECTS = $(zigbee_OBJECTS) \
	_zigbee_walker.o zigbee_walker.o xbee_time.o zcl_client.o
zigbee_walker.exe : $(zigbee_walker_OBJECTS)
	$(COMPILE) -o $@ $^

commissioning_client_OBJECTS = $(zigbee_OBJECTS) \
		commissioning_client.o \
		$(atinter_OBJECTS) _commission_client.o \
	zcl_commissioning.o \
	zcl_client.o zcl_identify.o xbee_time.o

commissioning_client.exe : $(commissioning_client_OBJECTS)
	$(COMPILE) -o $@ $^

commissioning_server_OBJECTS = $(zigbee_OBJECTS) \
		commissioning_server.o \
		$(atinter_OBJECTS) _commission_server.o \
	zcl_commissioning.o xbee_commissioning.o \
	zcl_client.o zcl_identify.o xbee_time.o

commissioning_server.exe : $(commissioning_server_OBJECTS)
	$(COMPILE) -o $@ $^

# Use the depenency files created by the -MD option to gcc.
-include $(SRCS:.c=.d)

# to build a .o file, use the .c file in the current dir...
.c.o :
	$(COMPILE) -c $<

# ...or in common samples directory...
%.o : ../common/%.c
	$(COMPILE) -c $<

# ...or in SRCDIR...
%.o : $(SRCDIR)/%.c
	$(COMPILE) -c $<

# ...or in a given subdirectory of SRCDIR.
%.o : $(SRCDIR)/xbee/%.c
	$(COMPILE) -c $<
%.o : $(SRCDIR)/wpan/%.c
	$(COMPILE) -c $<
%.o : $(SRCDIR)/zigbee/%.c
	$(COMPILE) -c $<
%.o : $(SRCDIR)/win32/%.c
	$(COMPILE) -c $<
%.o : $(SRCDIR)/util/%.c
	$(COMPILE) -c $<

