# Makefile for XBee Driver Unit Tests on POSIX platform

# Autodepend methods from http://make.paulandlesley.org/autodep.html

# If you get a "no rule to make target" error for some random .h file, try
# deleting all .d files.

# directory for driver
DRIVER = ../..

# directory for platform support
PORT = posix
PORTDIR = $(DRIVER)/ports/$(PORT)

# path to include files
INCDIR = $(DRIVER)/include

# path to common source files
SRCDIR = $(DRIVER)/src

# extra defines (in addition to those in platform_config.h)
DEFINE = \
	-DPOSIX \
	-DXBEE_PLATFORM_HEADER='"$(PORTDIR)/platform_config.h"' \
	-DXBEE_DEVICE_ENABLE_ATMODE \

# path for Coverity temp directory
COVERITY_TEMP = /tmp/coverity

# compiler parameters for building each file
# -MMD generates dependency files automatically, omitting system files
# -MP creates phony targets for each prerequisite in a .d file

COMPILE = gcc -std=gnu99 -iquote$(INCDIR) -g -MMD -MP -Wall $(DEFINE)

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
		t_srp \

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
	&& ./t_srp \
	&& echo "ALL PASSED"

clean :
	- rm *.o *.d $(EXE) jsll_gen

SRCS = unittest.c main.c \
	$(wildcard $(SRCDIR)/*/*.c) \
	$(wildcard $(PORTDIR)/*.c) \
	$(wildcard $(DRIVER)/test/*/*.c) \
	$(wildcard $(DRIVER)/samples/$(PORT)/*.c) \
	$(wildcard $(DRIVER)/samples/common/*.c) \

all_OBJECTS = \
	wpan_aps.o \
	wpan_types.o \
	xbee_platform_$(PORT).o \
	xbee_serial_$(PORT).o \
	xbee_atcmd.o \
	xbee_atmode.o \
	xbee_cbuf.o \
	xbee_commissioning.o \
	xbee_device.o \
	xbee_discovery.o \
	xbee_firmware.o \
	xbee_io.o \
	pxbee_ota_client.o \
	pxbee_ota_server.o \
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

platform_OBJECTS = xbee_platform_$(PORT).o xbee_serial_$(PORT).o \
	hexdump.o					\
	swapbytes.o					\
	swapcpy.o					\
	hexstrtobyte.o				\
	unittest.o

base_OBJECTS = $(platform_OBJECTS) parse_serial_args.o xbee_readline.o

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
	$(COMPILE) -o $@ $^

zcl_write_OBJECTS = $(zcl_test_OBJECTS) zcl_write.o
zcl_write : $(zcl_write_OBJECTS)
	$(COMPILE) -o $@ $^

zcl_read_OBJECTS = $(zcl_test_OBJECTS) zcl_read.o
zcl_read : $(zcl_read_OBJECTS)
	$(COMPILE) -o $@ $^

zcl_24bit_OBJECTS = $(zcl_test_OBJECTS) zcl_24bit.o
zcl_24bit : $(zcl_24bit_OBJECTS)
	$(COMPILE) -o $@ $^

zcl_boolean_OBJECTS = $(zcl_test_OBJECTS) zcl_boolean.o
zcl_boolean : $(zcl_boolean_OBJECTS)
	$(COMPILE) -o $@ $^

zcl_encode_structured_values_OBJECTS = $(zcl_common_OBJECTS) \
	zcl_encode_structured_values.o
zcl_encode_structured_values : $(zcl_encode_structured_values_OBJECTS)
	$(COMPILE) -o $@ $^

zdo_match_desc_request_OBJECTS = $(zcl_test_OBJECTS) zdo_match_desc_request.o
zdo_match_desc_request : $(zdo_match_desc_request_OBJECTS)
	$(COMPILE) -o $@ $^

zdo_simple_desc_respond_OBJECTS = $(zcl_test_OBJECTS) \
	zdo_simple_desc_respond.o
zdo_simple_desc_respond : $(zdo_simple_desc_respond_OBJECTS)
	$(COMPILE) -o $@ $^

# testing for jslong
jsll_gen : ../util/jsll_gen.c
	gcc -o $@ ../util/jsll_gen.c
../util/jsll_vectors.h : jsll_gen
	./jsll_gen > ../util/jsll_vectors.h
t_jslong : ../util/jsll_vectors.h $(platform_OBJECTS) t_jslong.o main.o
	$(COMPILE) -o $@ $(platform_OBJECTS) t_jslong.o main.o

t_memcheck_OBJECTS = $(platform_OBJECTS) t_memcheck.o memcheck.o
t_memcheck : $(t_memcheck_OBJECTS)
	$(COMPILE) -o $@ $^

mbedtls_OBJECTS = aes.o bignum.o ctr_drbg.o entropy.o entropy_poll.o sha256.o \
            mbedtls_util.o xbee_random_mbedtls.o
t_srp_OBJECTS = $(mbedtls_OBJECTS) srp.o t_srp.o unittest.o
t_srp : $(t_srp_OBJECTS)
	$(COMPILE) -o $@ $^

t_packed_struct : t_packed_struct.o
	$(COMPILE) -o $@ $^

xbee_timer_compare_OBJECTS = $(platform_OBJECTS) xbee_timer_compare.o
xbee_timer_compare : $(xbee_timer_compare_OBJECTS)
	$(COMPILE) -o $@ $^

t_cbuf_OBJECTS = $(platform_OBJECTS) $(cbuf_OBJECTS) t_cbuf.o
t_cbuf : $(t_cbuf_OBJECTS)
	$(COMPILE) -o $@ $^

zcl_type_name_OBJECTS = zcl_type_name.o zcl_types.o unittest.o
zcl_type_name: $(zcl_type_name_OBJECTS)
	$(COMPILE) -o $@ $^

zdo_tool_OBJECTS = $(zigbee_OBJECTS) $(atinter_OBJECTS) zdo_tool.o
zdo_tool : $(zdo_tool_OBJECTS)
	$(COMPILE) -o $@ $^

serial-passthru: serial-passthru.o $(base_OBJECTS)
	$(COMPILE) -o $@ $^

coverity_OBJECTS = $(all_OBJECTS) coverity.o
coverity : $(coverity_OBJECTS)
	$(COMPILE) -o $@ $^

coverity-clean :
	- rm *.o *.d coverity
	# multiple `rm` below are safer than just $(COVERITY_TEMP)/*
	- rm -rf $(COVERITY_TEMP)/BUILD.metrics.xml
	- rm -rf $(COVERITY_TEMP)/build-log.txt
	- rm -rf $(COVERITY_TEMP)/c

coverity-build:
	mkdir -p $(COVERITY_TEMP)
	cov-build --dir $(COVERITY_TEMP) make coverity
	rm coverity
	cov-analyze --dir $(COVERITY_TEMP) \
		--aggressiveness-level medium \
		--enable-fnptr \
		--all --hfa
	cov-format-errors --dir $(COVERITY_TEMP) -x

coverity-push:
	cov-commit-defects --dir $(COVERITY_TEMP) \
		--target posix \
		--strip-path `pwd | sed -e "s|test/posix||"` \
		--host coverity.digi.com \
		--stream host_ansic_driver

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

# ...or in the port's samples (for parse_serial_args.o)
%.o : ../../samples/$(PORT)/%.c
	$(COMPILE) -c $<

# ...or in a samples/common (for _atinter.o)
%.o : ../../samples/common/%.c
	$(COMPILE) -c $<

