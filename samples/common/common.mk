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
	install_ebin \
	install_ebl \
	ipv4_client \
	network_scan \
	sms_client \
	socket_test \
	transparent_client \
	user_data_relay \
	xbee_netcat \
	xbee_term \
	xbee3_ota_tool \
	xbee3_srp_verifier \
	zcltime \
	zigbee_ota_info \
	zigbee_register_device \
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
	$(wildcard $(SRCDIR)/*/*.c) \
	$(wildcard $(PORTDIR)/*.c) \
	$(wildcard $(DRIVER)/samples/$(PORT)/*.c) \
	$(wildcard $(DRIVER)/samples/common/*.c) \

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

socket_OBJECTS = $(xbee_OBJECTS) xbee_socket_frames.o xbee_socket.o
socket_test_OBJECTS = $(socket_OBJECTS) socket_test.o $(atinter_OBJECTS) \
            xbee_ipv4.o xbee_delivery_status.o
socket_test : $(socket_test_OBJECTS)
	$(COMPILE) -o $@ $^

xbee_netcat_OBJECTS = $(socket_OBJECTS) xbee_netcat.o xbee_delivery_status.o \
            xbee_readline.o xbee_term_$(PORT).o xbee_ipv4.o
xbee_netcat : $(xbee_netcat_OBJECTS)
	$(COMPILE) -o $@ $^

transparent_client_OBJECTS = $(zigbee_OBJECTS) transparent_client.o \
	$(atinter_OBJECTS) _nodetable.o \
	xbee_discovery.o \
	xbee_transparent_serial.o

transparent_client : $(transparent_client_OBJECTS)
	$(COMPILE) -o $@ $^

install_ebin_OBJECTS += $(xbee_OBJECTS) install_ebin.o \
	xbee_bl_gen3.o \
	crc16buypass.o

install_ebin: $(install_ebin_OBJECTS)
	$(COMPILE) -o $@ $^ $(DIALOG_LIBS)

install_ebl_OBJECTS += $(xbee_OBJECTS) install_ebl.o \
	xbee_atmode.o \
	xbee_firmware.o \
	xbee_xmodem.o xmodem_crc16.o

install_ebl: $(install_ebl_OBJECTS)
	$(COMPILE) -o $@ $^ $(DIALOG_LIBS)

xbee3_ota_tool_OBJECTS = $(zigbee_OBJECTS) $(atinter_OBJECTS) \
            _nodetable.o xbee_discovery.o zcl_ota_upgrade.o zcl_ota_server.o \
            xbee3_ota_tool.o
xbee3_ota_tool : $(xbee3_ota_tool_OBJECTS)

mbedtls_OBJECTS = aes.o bignum.o ctr_drbg.o entropy.o entropy_poll.o sha256.o \
            mbedtls_util.o xbee_random_mbedtls.o
xbee3_srp_verifier_OBJECTS = $(mbedtls_OBJECTS) $(xbee_OBJECTS) $(atinter_OBJECTS) \
            srp.o xbee3_srp_verifier.o
xbee3_srp_verifier : $(xbee3_srp_verifier_OBJECTS)
	$(COMPILE) -o $@ $^

# Zigbee samples

# Info on files used with over-the-air (OTA) upgrade cluster
zigbee_ota_info_OBJECTS = swapbytes.o swapcpy.o hexstrtobyte.o \
                zcl_ota_upgrade.o zigbee_ota_info.o wpan_types.o
zigbee_ota_info : $(zigbee_ota_info_OBJECTS)
	$(COMPILE) -o $@ $^

zigbee_register_device_OBJECTS = $(xbee_OBJECTS) $(atinter_OBJECTS) \
            xbee_register_device.o zigbee_register_device.o \
            _nodetable.o xbee_discovery.o
zigbee_register_device : $(zigbee_register_device_OBJECTS)
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

# ...or in a subdirectory of SRCDIR...
%.o : $(SRCDIR)/*/%.c
	$(COMPILE) -c $<
