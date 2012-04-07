#
# Makefile definitions for netyack
# Author: Daniel Borkmann
#

# For mem debugging add -D_DEBUG_
LD_NORM      = echo "LD        $(target)";    \
               gcc
CC_NORM      = echo "CC        $<";           \
               gcc
CC_DEBUG     = echo "DC        $<";           \
               gcc
A2X_NORM     = echo "A2X   $<";               \
               a2x
FL_NORM      = echo "FL        $(flex-obj)";  \
               flex
BI_NORM      = echo "BI        $(bison-obj)"; \
               bison

MAKEFLAGS   += --no-print-directory --silent

BINDIR       = usr/sbin
ETCDIR       = etc

define eq
	$(if $(1:$(2)=),,$(if $(2:$(1)=),,T))
endef

FL = $(FL_NORM)
BI = $(BI_NORM) -d

ifneq ($(or $(call eq,$(MAKECMDGOALS),"all"), $(call eq,$(MAKECMDGOALS),"")),)
	LD      = $(LD_NORM) -o
	CC      = $(CC_NORM) -c
	CFLAGS  = -O2 -fomit-frame-pointer -fno-strict-aliasing -fno-common  \
		  -fno-delete-null-pointer-checks -std=gnu99                 \
		  -fstack-protector -D_FORTIFY_SOURCE=2 -fPIE                \
		  -fno-strict-overflow -D_REENTRANT
	CFLAGS += -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs   \
		  -Werror-implicit-function-declaration -Wno-format-security \
		  -Wcomments -Wendif-labels -Wno-long-long -Wuninitialized   \
		  -Wstrict-overflow
endif

.PHONY: all

