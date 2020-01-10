# Makefile for sample programs on Digi International products using
# libdigiapix as the GPIO interface.

# We expect the build environment to set $(CC) to the cross-compiler.
# CC ?= gcc

PORT = digiapix

DEFINE = -DPOSIX

CFLAGS += $(shell pkg-config --cflags libdigiapix)
LDLIBS += $(shell pkg-config --libs libdigiapix)

# Samples for this platform, in addition to samples in common.mk
EXE =

# compiler parameters for building each file
# -MMD generates dependency files automatically, omitting system files
# -MP creates phony targets for each prerequisite in a .d file
CFLAGS += $(DEFINE) -iquote$(INCDIR) -std=gnu99 -g -MMD -MP -Wall

include ../common/common.mk

clean :
	- rm -f *.o *.d $(EXE)
