# Makefile for XBee Driver Unit Tests on Win32

# Autodepend methods from http://make.paulandlesley.org/autodep.html

# If you get a "no rule to make target" error for some random .h file, try
# deleting all .d files.

# directory for driver
DRIVER = ../..

# directory for platform support
PORTDIR = $(DRIVER)/ports/win32

# path to include files
INCDIR = $(DRIVER)/include

# path to common source files
SRCDIR = $(DRIVER)/src

# extra defines (in addition to those in platform_config.h)
DEFINE = \
	-DXBEE_PLATFORM_HEADER='"$(PORTDIR)/platform_config.h"' \
	-DXBEE_DEVICE_ENABLE_ATMODE \

# path for Coverity temp directory
COVERITY_TEMP = /c/tmp/coverity

# if PATH isn't set correctly, it may be necessary to do this:
# PATH="/c/Program Files/Coverity/Coverity Integrity Center/static-analysis/bin/:$PATH"

# compiler parameters for building each file
# -MMD generates dependency files automatically, omitting system files
# -MP creates phony targets for each prerequisite in a .d file

# use gcc 4 with MinGW (will not work with Cygwin)
COMPILE_GCC4 = gcc -iquote$(INCDIR)

# use gcc 3 in Cygwin with -mno-cygwin option to build standalone executables
COMPILE_GCC3 = gcc-3 -mno-cygwin -I$(INCDIR)

COMPILE = $(COMPILE_GCC4) -std=gnu99 -g -MMD -MP -Wall $(DEFINE)

EXE = zcl_write \
		zcl_read \
		zcl_24bit \
		zcl_boolean \
		zcl_type_info \
		zcl_encode_structured_values \
		zdo_match_desc_request \
		zdo_simple_desc_respond \
		t_jslong \
		t_packed_struct \
		xbee_timer_compare \
		t_cbuf \
		zcl_type_name \
		t_memcheck \

all : $(EXE)

test : $(EXE)
		./zcl_write \
	&& ./zcl_read \
	&& ./zcl_24bit \
	&& ./zcl_boolean \
	&& ./zcl_type_info \
	&& ./zcl_encode_structured_values \
	&& ./zdo_match_desc_request \
	&& ./zdo_simple_desc_respond \
	&& ./t_jslong \
	&& ./t_packed_struct \
	&& ./xbee_timer_compare \
	&& ./t_cbuf \
	&& ./zcl_type_name \
	&& ./t_memcheck \
	&& echo "ALL PASSED"

clean :
	- rm *.o *.d $(addsuffix .exe, $(EXE)) jsll_gen.exe

SRCS = unittest.c main.c \
		zcl_test_common.c \
		zcl_write.c \
		zcl_read.c \
		zcl_24bit.c \
		zcl_boolean.c \
		zdo_match_desc_request.c \
		zdo_simple_desc_respond.c \
		zdo_tool.c \
		xbee_timer_compare.c \
		t_cbuf.c \
		zcl_type_name.c \
		t_memcheck.c \
		coverity.c \
	wpan_aps.c \
	wpan_types.c \
	xbee_platform_win32.c \
	xbee_serial_win32.c \
	xbee_atcmd.c \
	xbee_atmode.c \
	xbee_cbuf.c \
	xbee_commissioning.c \
	xbee_device.c \
	xbee_discovery.c \
	xbee_firmware.c \
	xbee_io.c \
	xbee_ota_client.c \
	xbee_ota_server.c \
	xbee_reg_descr.c \
	xbee_scan.c \
	xbee_sxa.c \
	xbee_time.c \
	xbee_transparent_serial.c \
	xbee_wifi.c \
	xbee_wpan.c \
	xbee_xmodem.c \
	sep_key_estab.c \
	sep_messaging.c \
	sep_metering.c \
	zcl_basic.c \
	zcl_client.c \
	zcl_commissioning.c \
	zcl_identify.c \
	zcl_onoff.c \
	zcl_time.c \
	zcl_types.c \
	zigbee_zcl.c \
	zigbee_zdo.c \
	xmodem_crc16.c \
	hexdump.c \
	hexstrtobyte.c \
	memcheck.c \
	swapbytes.c \
	swapcpy.c \
	xmodem_crc16.c \
	_atinter.c \
	parse_serial_args.c

all_OBJECTS = \
	wpan_aps.o \
	wpan_types.o \
	xbee_platform_win32.o \
	xbee_serial_win32.o \
	xbee_atcmd.o \
	xbee_atmode.o \
	xbee_cbuf.o \
	xbee_commissioning.o \
	xbee_device.o \
	xbee_discovery.o \
	xbee_firmware.o \
	xbee_io.o \
	xbee_ota_client.o \
	xbee_ota_server.o \
	xbee_reg_descr.o \
	xbee_sxa.o \
	xbee_time.o \
	xbee_transparent_serial.o \
	xbee_wpan.o \
	xbee_xmodem.o \
	sep_key_estab.o \
	sep_messaging.o \
	sep_metering.o \
	zcl_basic.o \
	zcl_client.o \
	zcl_commissioning.o \
	zcl_identify.o \
	zcl_onoff.o \
	zcl_time.o \
	zcl_types.o \
	zigbee_zcl.o \
	zigbee_zdo.o \
	hexdump.o \
	hexstrtobyte.o \
	memcheck.o \
	swapbytes.o \
	swapcpy.o \
	xmodem_crc16.o

platform_OBJECTS = xbee_platform_win32.o xbee_serial_win32.o \
	hexdump.o					\
	swapbytes.o					\
	swapcpy.o					\
	hexstrtobyte.o				\
	unittest.o

base_OBJECTS = $(platform_OBJECTS) parse_serial_args.o

cbuf_OBJECTS =  xbee_cbuf.o

xbee_OBJECTS = $(base_OBJECTS) xbee_device.o xbee_atcmd.o wpan_types.o

wpan_OBJECTS = $(xbee_OBJECTS) wpan_aps.o xbee_wpan.o

zigbee_OBJECTS = $(wpan_OBJECTS) zigbee_zcl.o zigbee_zdo.o zcl_types.o

atinter_OBJECTS = _atinter.o

zcl_common_OBJECTS =	\
	$(platform_OBJECTS)		\
	wpan_aps.o					\
	wpan_types.o				\
	zcl_types.o					\
	zigbee_zcl.o				\
	zigbee_zdo.o

zcl_test_OBJECTS = $(zcl_common_OBJECTS) zcl_test_common.o

zcl_type_info_OBJECTS = unittest.o zcl_types.o zcl_type_info.o
zcl_type_info : $(zcl_type_info_OBJECTS)
	$(COMPILE) -o $@ $(zcl_type_info_OBJECTS)

zcl_write_OBJECTS = $(zcl_test_OBJECTS) zcl_write.o
zcl_write : $(zcl_write_OBJECTS)
	$(COMPILE) -o $@ $(zcl_write_OBJECTS)

zcl_read_OBJECTS = $(zcl_test_OBJECTS) zcl_read.o
zcl_read : $(zcl_read_OBJECTS)
	$(COMPILE) -o $@ $(zcl_read_OBJECTS)

zcl_24bit_OBJECTS = $(zcl_test_OBJECTS) zcl_24bit.o
zcl_24bit : $(zcl_24bit_OBJECTS)
	$(COMPILE) -o $@ $(zcl_24bit_OBJECTS)

zcl_boolean_OBJECTS = $(zcl_test_OBJECTS) zcl_boolean.o
zcl_boolean : $(zcl_boolean_OBJECTS)
	$(COMPILE) -o $@ $(zcl_boolean_OBJECTS)

zcl_encode_structured_values_OBJECTS = $(zcl_common_OBJECTS) \
	zcl_encode_structured_values.o
zcl_encode_structured_values : $(zcl_encode_structured_values_OBJECTS)
	$(COMPILE) -o $@ $(zcl_encode_structured_values_OBJECTS)

zdo_match_desc_request_OBJECTS = $(zcl_test_OBJECTS) zdo_match_desc_request.o
zdo_match_desc_request : $(zdo_match_desc_request_OBJECTS)
	$(COMPILE) -o $@ $(zdo_match_desc_request_OBJECTS)

zdo_simple_desc_respond_OBJECTS = $(zcl_test_OBJECTS) \
	zdo_simple_desc_respond.o
zdo_simple_desc_respond : $(zdo_simple_desc_respond_OBJECTS)
	$(COMPILE) -o $@ $(zdo_simple_desc_respond_OBJECTS)

# testing for jslong
jsll_gen : ../util/jsll_gen.c
	gcc -o $@ ../util/jsll_gen.c
../util/jsll_vectors.h : jsll_gen
	./jsll_gen > ../util/jsll_vectors.h
t_jslong : ../util/jsll_vectors.h $(platform_OBJECTS) t_jslong.o main.o
	$(COMPILE) -o $@ $(platform_OBJECTS) t_jslong.o main.o

t_memcheck_OBJECTS = $(platform_OBJECTS) t_memcheck.o memcheck.o
t_memcheck : $(t_memcheck_OBJECTS)
	$(COMPILE) -o $@ $(t_memcheck_OBJECTS)

t_packed_struct : t_packed_struct.o
	$(COMPILE) -o $@ $^

xbee_timer_compare_OBJECTS = $(platform_OBJECTS) xbee_timer_compare.o
xbee_timer_compare : $(xbee_timer_compare_OBJECTS)
	$(COMPILE) -o $@ $(xbee_timer_compare_OBJECTS)

t_cbuf_OBJECTS = $(platform_OBJECTS) $(cbuf_OBJECTS) t_cbuf.o
t_cbuf : $(t_cbuf_OBJECTS)
	$(COMPILE) -o $@ $(t_cbuf_OBJECTS)

zcl_type_name_OBJECTS = zcl_type_name.o zcl_types.o unittest.o
zcl_type_name: $(zcl_type_name_OBJECTS)
	$(COMPILE) -o $@ $(zcl_type_name_OBJECTS)

zdo_tool_OBJECTS = $(zigbee_OBJECTS) $(atinter_OBJECTS) zdo_tool.o
zdo_tool : $(zdo_tool_OBJECTS)
	$(COMPILE) -o $@ $(zdo_tool_OBJECTS)

serial-passthru: serial-passthru.o $(base_OBJECTS)
	$(COMPILE) -o $@ serial-passthru.o $(base_OBJECTS)

coverity_OBJECTS = $(all_OBJECTS) coverity.o
coverity : $(coverity_OBJECTS)
	$(COMPILE) -o $@ $(coverity_OBJECTS)

coverity-clean :
	- rm *.o *.d coverity.exe
	# multiple `rm` below are safer than just $(COVERITY_TEMP)/*
	- rm -rf $(COVERITY_TEMP)/BUILD.metrics.xml
	- rm -rf $(COVERITY_TEMP)/build-log.txt
	- rm -rf $(COVERITY_TEMP)/c

coverity-build:
	mkdir -p $(COVERITY_TEMP)
	cov-build --dir $(COVERITY_TEMP) make coverity.exe
	rm coverity.exe
	cov-analyze --dir $(COVERITY_TEMP) \
		--aggressiveness-level medium \
		--enable-fnptr \
		--all --hfa
	cov-format-errors --dir $(COVERITY_TEMP) -x

coverity-push:
	cov-commit-defects.exe --dir $(COVERITY_TEMP) \
		--target win32 \
		--strip-path `pwd | sed -e "s|test/win32||"` \
		--host coverity.digi.com \
		--stream host_ansic_driver

# Use the dependency files created by the -MD option to gcc.
-include $(SRCS:.c=.d)

# to build a .o file, use the .c file in the current dir...
.c.o :
	$(COMPILE) -c $<

# ...or up one dir
%.o : ../%.c
	$(COMPILE) -c $<

# ...or up one dir and in the util, xbee or zigbee test directories
%.o : ../zigbee/%.c
	$(COMPILE) -c $<
%.o : ../xbee/%.c
	$(COMPILE) -c $<
%.o : ../util/%.c
	$(COMPILE) -c $<

# ...or in the port support directory...
%.o : $(PORTDIR)/%.c
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

# ...or in a samples/win32 (for parse_serial_args.o)
%.o : ../../samples/win32/%.c
	$(COMPILE) -c $<

# ...or in a samples/common (for _atinter.o)
%.o : ../../samples/common/%.c
	$(COMPILE) -c $<

