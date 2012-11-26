# Makefile for XBee Driver on POSIX platforms

# Autodepend methods from http://make.paulandlesley.org/autodep.html

# If you get a "no rule to make target" error for some random .h file, try
# deleting all .d files.

# extra defines
DEFINE = -DPOSIX \
	-DZCL_ENABLE_TIME_SERVER \
	-DXBEE_DEVICE_ENABLE_ATMODE \
	-DXBEE_XMODEM_TESTING

# directory for driver
DRIVER = ../..

# path to include files
INCDIR = $(DRIVER)/include

# path to common source files
SRCDIR = $(DRIVER)/src

# compiler parameters for building each file
# -MMD generates dependency files automatically, omitting system files
# -MP creates phony targets for each prerequisite in a .d file

COMPILE = gcc -iquote$(INCDIR) -g -MMD -MP -Wall $(DEFINE)

EXE = atinter install_ebl zcltime \
		commissioning_client commissioning_server transparent_client zigbee_walker \
		apply_profile xbee_term eblinfo

all : $(EXE)

# strip debug information from executables
strip :
	strip $(EXE)

zcltime : zcltime.o
	$(COMPILE) -o $@ $^

eblinfo : eblinfo.o xbee_ebl_file.o swapbytes.o
	$(COMPILE) -o $@ $^

clean :
	- rm -f *.o *.d $(EXE)

SRCS = \
		atinter.c \
		eblinfo.c \
		install_ebl.c \
		transparent_client.c \
		ota_test.c \
		xbee_term.c _xbee_term.c xbee_term_posix.c \
		zcltime.c \
		parse_serial_args.c \
	_atinter.c _pxbee_ota_update.c \
	_commission_client.c \
	_commission_server.c _nodetable.c _zigbee_walker.c \
	xbee_platform_posix.c \
	xbee_serial_posix.c \
	xbee_atcmd.c \
	xbee_atmode.c \
	xbee_cbuf.c \
	xbee_device.c \
	xbee_discovery.c \
	xbee_ebl_file.c \
	xbee_firmware.c \
	xbee_ota_client.c \
	xbee_platform_posix.c \
	xbee_readline.c \
	xbee_serial_posix.c \
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
	hexdump.c \
	xbee_readline.c

base_OBJECTS = xbee_platform_posix.o xbee_serial_posix.o hexstrtobyte.o \
					memcheck.o swapbytes.o swapcpy.o hexdump.o parse_serial_args.o

xbee_OBJECTS = $(base_OBJECTS) xbee_device.o xbee_atcmd.o wpan_types.o

wpan_OBJECTS = $(xbee_OBJECTS) wpan_aps.o xbee_wpan.o

zigbee_OBJECTS = $(wpan_OBJECTS) zigbee_zcl.o zigbee_zdo.o zcl_types.o

atinter_OBJECTS = xbee_readline.o _atinter.o

atinter : $(xbee_OBJECTS) $(atinter_OBJECTS) atinter.o
	$(COMPILE) -o $@ $^

apply_profile : $(xbee_OBJECTS) apply_profile.o
	$(COMPILE) -o $@ $^

xbee_term_OBJECTS = $(base_OBJECTS) xbee_term.o _xbee_term.o xbee_term_posix.o
xbee_term : $(xbee_term_OBJECTS)
	$(COMPILE) -o $@ $^

transparent_client_OBJECTS = $(zigbee_OBJECTS) transparent_client.o \
	$(atinter_OBJECTS) _nodetable.o \
	xbee_discovery.o \
	xbee_transparent_serial.o

transparent_client : $(transparent_client_OBJECTS)
	$(COMPILE) -o $@ $^

install_ebl_OBJECTS = $(xbee_OBJECTS) install_ebl.o \
	xbee_atmode.o \
	xbee_firmware.o \
	xbee_xmodem.o xmodem_crc16.o

install_ebl: $(install_ebl_OBJECTS)
	$(COMPILE) -o $@ $^

pxbee_ota_update_OBJECTS = $(zigbee_OBJECTS) \
		pxbee_ota_update.o \
		$(atinter_OBJECTS) _pxbee_ota_update.o \
	xbee_cbuf.o \
	xbee_ota_client.o \
	xbee_transparent_serial.o \
	xbee_xmodem.o xmodem_crc16.o \
	sep_key_estab.o \
	zcl_client.o zcl_identify.o xbee_time.o

pxbee_ota_update : $(pxbee_ota_update_OBJECTS)
	$(COMPILE) -o $@ $^

zigbee_walker_OBJECTS = $(zigbee_OBJECTS) \
	_zigbee_walker.o zigbee_walker.o xbee_time.o zcl_client.o
zigbee_walker : $(zigbee_walker_OBJECTS)
	$(COMPILE) -o $@ $^

commissioning_client_OBJECTS = $(zigbee_OBJECTS) \
		commissioning_client.o \
		$(atinter_OBJECTS) _commission_client.o \
	zcl_commissioning.o \
	zcl_client.o zcl_identify.o xbee_time.o

commissioning_client : $(commissioning_client_OBJECTS)
	$(COMPILE) -o $@ $^

commissioning_server_OBJECTS = $(zigbee_OBJECTS) \
		commissioning_server.o \
		$(atinter_OBJECTS) _commission_server.o \
	zcl_commissioning.o xbee_commissioning.o \
	zcl_client.o zcl_identify.o xbee_time.o

commissioning_server : $(commissioning_server_OBJECTS)
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
%.o : $(SRCDIR)/posix/%.c
	$(COMPILE) -c $<
%.o : $(SRCDIR)/util/%.c
	$(COMPILE) -c $<

