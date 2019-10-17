# Makefile for sample programs on Win32

# Uses MinGW or Cygwin's gcc with "-mno-cygwin" option.

PORT = win32

# libs for samples using GetOpenFileName
DIALOG_LIBS = -lkernel32 -lcomdlg32

# samples in addition to those in samples/common
EXE = comports pxbee_ota_update pxbee_update

# compiler parameters for building each file
# -MMD generates dependency files automatically, omitting system files
# -MP creates phony targets for each prerequisite in a .d file

# use gcc 4 with MinGW (will not work with Cygwin)
COMPILE_GCC4 = gcc -iquote$(INCDIR)

# use gcc 3 in Cygwin with -mno-cygwin option to build standalone executables
COMPILE_GCC3 = gcc-3 -mno-cygwin -I$(INCDIR)

COMPILE = $(COMPILE_GCC4) -std=gnu99 -g -MMD -MP -Wall $(DEFINE) $(CFLAGS)

# extra objects for some samples
install_ebl_OBJECTS = win32_select_file.o
install_ebin_OBJECTS = win32_select_file.o

include ../common/common.mk

clean :
	- rm -f *.o *.d $(addsuffix .exe, $(EXE))

# Samples unique to this platform
comports : comports.c
	$(COMPILE) -o $@ $^

pxbee_update_OBJECTS = $(xbee_OBJECTS) pxbee_update.o \
	win32_select_file.o \
	xbee_atmode.o \
	xbee_firmware.o \
	xbee_xmodem.o xmodem_crc16.o
pxbee_update: $(pxbee_update_OBJECTS)
	$(COMPILE) -o $@ $^ $(DIALOG_LIBS)

pxbee_ota_update_OBJECTS = $(zigbee_OBJECTS) \
	win32_select_file.o \
	pxbee_ota_update.o \
	$(atinter_OBJECTS) _pxbee_ota_update.o \
	xbee_cbuf.o \
	pxbee_ota_client.o \
	xbee_transparent_serial.o \
	xbee_xmodem.o xmodem_crc16.o \
	zcl_client.o zcl_identify.o xbee_time.o

pxbee_ota_update : $(pxbee_ota_update_OBJECTS)
	$(COMPILE) -o $@ $^ $(DIALOG_LIBS)
