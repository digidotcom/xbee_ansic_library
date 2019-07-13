# Shared Makefile rules for sample programs on Win32 and POSIX platforms

# Autodepend methods from http://make.paulandlesley.org/autodep.html

# If you get a "no rule to make target" error for some random .h file, try
# deleting all .d files.

# directory for driver
DRIVER = ../..

# directory for platform support
PORTDIR = $(DRIVER)/ports/$(PORT)

# path to include files
INCDIR = $(DRIVER)/include

# path to common source files
SRCDIR = $(DRIVER)/src

# extra defines (in addition to those in platform_config.h)
DEFINE += \
	-DXBEE_PLATFORM_HEADER='"$(PORTDIR)/platform_config.h"' \
	-DZCL_ENABLE_TIME_SERVER \
	-DXBEE_CELLULAR_ENABLED \
	-DXBEE_DEVICE_ENABLE_ATMODE \
	-DXBEE_XMODEM_TESTING

EXE += \
	apply_profile \
	atinter \
	commissioning_client \
	commissioning_server \
	eblinfo \
	gpm \
	install_ebl \
	ipv4_client \
	network_scan \
	sms_client \
	transparent_client \
	user_data_relay \
	xbee_term \
	zcltime \
	zigbee_walker \

all : $(EXE)

# strip debug information from executables
strip :
	strip $(EXE)

zcltime : zcltime.o
	$(COMPILE) -o $@ $^

eblinfo : eblinfo.o xbee_ebl_file.o swapbytes.o
	$(COMPILE) -o $@ $^

SRCS = \
	atinter.c \
	eblinfo.c \
	install_ebl.c \
	ipv4_client.c \
	transparent_client.c \
	pxbee_ota_update.c \
	xbee_term.c _xbee_term.c xbee_term_$(PORT).c \
	zcltime.c \
	parse_serial_args.c \
	_atinter.c _pxbee_ota_update.c \
	_commission_client.c \
	_commission_server.c _nodetable.c _zigbee_walker.c \
	xbee_atcmd.c \
	xbee_atmode.c \
	xbee_cbuf.c \
	xbee_device.c \
	xbee_discovery.c \
	xbee_ebl_file.c \
	xbee_firmware.c \
	xbee_gpm.c \
	xbee_ipv4.c \
	xbee_ota_client.c \
	xbee_platform_$(PORT).c \
	xbee_readline.c \
	xbee_scan.c \
	xbee_serial_$(PORT).c \
	xbee_sms.c \
	xbee_time.c \
	xbee_transparent_serial.c \
	xbee_tx_status.c \
	xbee_user_data.c \
	xbee_wifi.c \
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

base_OBJECTS = xbee_platform_$(PORT).o xbee_serial_$(PORT).o hexstrtobyte.o \
					memcheck.o swapbytes.o swapcpy.o hexdump.o parse_serial_args.o

xbee_OBJECTS = $(base_OBJECTS) xbee_device.o xbee_atcmd.o wpan_types.o

wpan_OBJECTS = $(xbee_OBJECTS) wpan_aps.o xbee_wpan.o

zigbee_OBJECTS = $(wpan_OBJECTS) zigbee_zcl.o zigbee_zdo.o zcl_types.o

atinter_OBJECTS = xbee_readline.o _atinter.o

atinter : $(xbee_OBJECTS) $(atinter_OBJECTS) atinter.o
	$(COMPILE) -o $@ $^

gpm : $(zigbee_OBJECTS) $(atinter_OBJECTS) xbee_gpm.o gpm.o
	$(COMPILE) -o $@ $^

network_scan_OBJECTS = $(xbee_OBJECTS) $(atinter_OBJECTS) network_scan.o xbee_scan.o xbee_wifi.o
network_scan : $(network_scan_OBJECTS)
	$(COMPILE) -o $@ $^

apply_profile : $(xbee_OBJECTS) apply_profile.o
	$(COMPILE) -o $@ $^

xbee_term_OBJECTS = $(base_OBJECTS) xbee_term.o xbee_term_$(PORT).o xbee_readline.o _xbee_term.o 
xbee_term : $(xbee_term_OBJECTS)
	$(COMPILE) -o $@ $^

ipv4_client_OBJECTS = $(xbee_OBJECTS)  ipv4_client.o xbee_tx_status.o _xbee_term.o xbee_term_$(PORT).o $(atinter_OBJECTS) xbee_ipv4.o
ipv4_client : $(ipv4_client_OBJECTS)
	$(COMPILE) -o $@ $^	
	
user_data_relay_OBJECTS = $(xbee_OBJECTS) xbee_readline.o xbee_user_data.o \
		user_data_relay.o
user_data_relay : $(user_data_relay_OBJECTS)
	$(COMPILE) -o $@ $^

sms_client_OBJECTS = $(xbee_OBJECTS) sms_client.o $(atinter_OBJECTS) xbee_sms.o 
sms_client : $(sms_client_OBJECTS)
	$(COMPILE) -o $@ $^	
	
transparent_client_OBJECTS = $(zigbee_OBJECTS) transparent_client.o \
	$(atinter_OBJECTS) _nodetable.o \
	xbee_discovery.o \
	xbee_transparent_serial.o

transparent_client : $(transparent_client_OBJECTS)
	$(COMPILE) -o $@ $^

install_ebl_OBJECTS += $(xbee_OBJECTS) install_ebl.o \
	xbee_atmode.o \
	xbee_firmware.o \
	xbee_xmodem.o xmodem_crc16.o

install_ebl: $(install_ebl_OBJECTS)
	$(COMPILE) -o $@ $^ $(DIALOG_LIBS)

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

# Use the dependency files created by the -MD option to gcc.
-include $(SRCS:.c=.d)

# to build a .o file, use the .c file in the current dir...
.c.o :
	$(COMPILE) -c $<

# ...or in the port support directory...
%.o : $(PORTDIR)/%.c
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
%.o : $(SRCDIR)/util/%.c
	$(COMPILE) -c $<

