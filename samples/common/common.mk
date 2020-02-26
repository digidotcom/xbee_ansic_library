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
	remote_at \
	sms_client \
	socket_test \
	transparent_client \
	user_data_relay \
	xbee_netcat \
	xbee_term \
	xbee3_ota_tool \
	xbee3_secure_session \
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
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

eblinfo : eblinfo.o xbee_ebl_file.o swapbytes.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

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
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

gpm : $(zigbee_OBJECTS) $(atinter_OBJECTS) xbee_gpm.o gpm.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

network_scan_OBJECTS = $(xbee_OBJECTS) $(atinter_OBJECTS) network_scan.o xbee_scan.o xbee_wifi.o
network_scan : $(network_scan_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

apply_profile : $(xbee_OBJECTS) apply_profile.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

xbee_term_OBJECTS = $(base_OBJECTS) xbee_term.o xbee_term_$(PORT).o xbee_readline.o _xbee_term.o
xbee_term : $(xbee_term_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

ipv4_client_OBJECTS = $(xbee_OBJECTS)  ipv4_client.o xbee_tx_status.o _xbee_term.o xbee_term_$(PORT).o $(atinter_OBJECTS) xbee_ipv4.o
ipv4_client : $(ipv4_client_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

user_data_relay_OBJECTS = $(xbee_OBJECTS) xbee_readline.o xbee_user_data.o \
		user_data_relay.o
user_data_relay : $(user_data_relay_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

remote_at_OBJECTS = $(xbee_OBJECTS) $(atinter_OBJECTS) \
	_nodetable.o xbee_discovery.o sample_cli.o remote_at.o
remote_at : $(remote_at_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

sms_client_OBJECTS = $(xbee_OBJECTS) sms_client.o $(atinter_OBJECTS) xbee_sms.o
sms_client : $(sms_client_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

socket_OBJECTS = $(xbee_OBJECTS) xbee_socket_frames.o xbee_socket.o
socket_test_OBJECTS = $(socket_OBJECTS) socket_test.o $(atinter_OBJECTS) \
            xbee_ipv4.o xbee_delivery_status.o sample_cli.o
socket_test : $(socket_test_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

xbee_netcat_OBJECTS = $(socket_OBJECTS) xbee_netcat.o xbee_delivery_status.o \
            xbee_readline.o xbee_term_$(PORT).o xbee_ipv4.o
xbee_netcat : $(xbee_netcat_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

transparent_client_OBJECTS = $(zigbee_OBJECTS) transparent_client.o \
	$(atinter_OBJECTS) _nodetable.o \
	xbee_discovery.o \
	xbee_transparent_serial.o

transparent_client : $(transparent_client_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

install_ebin_OBJECTS += $(xbee_OBJECTS) install_ebin.o \
	xbee_bl_gen3.o \
	crc16buypass.o

install_ebin: $(install_ebin_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

install_ebl_OBJECTS += $(xbee_OBJECTS) install_ebl.o \
	xbee_atmode.o \
	xbee_firmware.o \
	xbee_xmodem.o xmodem_crc16.o

install_ebl: $(install_ebl_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

xbee3_ota_tool_OBJECTS = $(zigbee_OBJECTS) $(atinter_OBJECTS) \
            _nodetable.o xbee_discovery.o zcl_ota_upgrade.o zcl_ota_server.o \
            sample_cli.o xbee3_ota_tool.o
xbee3_ota_tool : $(xbee3_ota_tool_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

xbee3_secure_session_OBJECTS = $(xbee_OBJECTS) $(atinter_OBJECTS) \
            _nodetable.o sample_cli.o xbee_discovery.o xbee_ext_modem_status.o \
            xbee_secure_session.o xbee3_secure_session.o
xbee3_secure_session : $(xbee3_secure_session_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

mbedtls_OBJECTS = aes.o bignum.o ctr_drbg.o entropy.o entropy_poll.o sha256.o \
            mbedtls_util.o xbee_random_mbedtls.o
xbee3_srp_verifier_OBJECTS = $(mbedtls_OBJECTS) $(xbee_OBJECTS) $(atinter_OBJECTS) \
            srp.o xbee3_srp_verifier.o sample_cli.o
xbee3_srp_verifier : $(xbee3_srp_verifier_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# Zigbee samples

# Info on files used with over-the-air (OTA) upgrade cluster
zigbee_ota_info_OBJECTS = swapbytes.o swapcpy.o hexstrtobyte.o \
                zcl_ota_upgrade.o zigbee_ota_info.o wpan_types.o
zigbee_ota_info : $(zigbee_ota_info_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

zigbee_register_device_OBJECTS = $(xbee_OBJECTS) $(atinter_OBJECTS) \
            xbee_register_device.o zigbee_register_device.o \
            _nodetable.o xbee_discovery.o sample_cli.o
zigbee_register_device : $(zigbee_register_device_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

zigbee_walker_OBJECTS = $(zigbee_OBJECTS) \
	_zigbee_walker.o zigbee_walker.o xbee_time.o zcl_client.o
zigbee_walker : $(zigbee_walker_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

commissioning_client_OBJECTS = $(zigbee_OBJECTS) \
		commissioning_client.o \
		$(atinter_OBJECTS) _commission_client.o \
	zcl_commissioning.o \
	zcl_client.o zcl_identify.o xbee_time.o

commissioning_client : $(commissioning_client_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

commissioning_server_OBJECTS = $(zigbee_OBJECTS) \
		commissioning_server.o \
		$(atinter_OBJECTS) _commission_server.o \
	zcl_commissioning.o xbee_commissioning.o \
	zcl_client.o zcl_identify.o xbee_time.o

commissioning_server : $(commissioning_server_OBJECTS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# Use the dependency files created by the -MD option to gcc.
-include $(SRCS:.c=.d)

# to build a .o file, use the .c file in the current dir...
.c.o :
	$(CC) $(CFLAGS) -c $<

# ...or in the port support directory...
%.o : $(PORTDIR)/%.c
	$(CC) $(CFLAGS) -c $<

# ...or in common samples directory...
%.o : ../common/%.c
	$(CC) $(CFLAGS) -c $<

# ...or in a subdirectory of SRCDIR...
%.o : $(SRCDIR)/*/%.c
	$(CC) $(CFLAGS) -c $<
