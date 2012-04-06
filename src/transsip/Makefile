#
# Makefile for transsip
# Author: Daniel Borkmann
#

include ../definitions.mk

INCLUDE     = -I.. -I./include/
LIBS        = -lspeexdsp -lasound -lcelt0 -lm -lreadline -lpthread

CFLAGS += -DPROGNAME_STRING="\"transsip\"" -DVERSION_STRING="\"0.5.0\""

core-objs   = transsip.o
lib-objs    = xmalloc.o		\
	      alsa.o		\
	      engine.o		\
	      notifier.o	\
	      call-notifier.o	\
	      xutils.o		\
	      stun.o		\
	      cli.o

target      = transsip

all: clean build 
debug: all
develop: all

gen-version:
	@cd .. && cat ../VERSION | xargs ./gen_build_info.pl

build: $(lib-objs) $(core-objs)
	@$(LD) $(target) $(core-objs) $(lib-objs) $(LIBS)
	@strip $(target)

%.o: ../%.c
	@$(CC) $(CFLAGS) $(INCLUDE) $<

clean:
	@rm *.o *~ $(target) || true

