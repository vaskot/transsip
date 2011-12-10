#
# Makefile for transsip
# Author: Daniel Borkmann
#

include ../definitions.mk

INCLUDE     = -I.. -I./include/
LIBS        = ./nacl/libnacl.a -lspeexdsp -lasound -lcelt0 -lm -lreadline -lpthread

core-objs   = transsip.o
lib-objs    = xmalloc.o		\
	      xutils.o		\
	      stun.o		\
	      alsa.o		\
	      cli.o		\
	      curve.o		\
	      mtrand.o

#     dht.o		\

target      = transsip

all: clean gen-version build-nacl build doc
debug: all
develop: all

gen-version:
	@cd .. && cat ../VERSION | xargs ./gen_build_info.pl

build-nacl:
	@sh ./nacl.sh

build: $(lib-objs) $(core-objs)
	@$(LD) $(target) $(core-objs) $(lib-objs) $(LIBS)
	@strip $(target)

doc:
#	@cd ../../Documentation && make clean
#	@cd ../../Documentation && make transsip
#	@cp ../../Documentation/man/* .
#	@cp ../../Documentation/pdf/* .
#	@cp ../../Documentation/html/* .

%.o: ../%.c
	@$(CC) $(CFLAGS) $(INCLUDE) $<

clean-nacl:
	@rm -rf ./nacl/ ./tmp/ ./include/

clean:
	@rm *.o *~ $(target) ../version.h *.gz *.pdf *.html *.8 *.css  \
	../parser.tab.c ../parser.tab.h ../lex.yy.c *.rc               \
	$(DIR_SND)/*.o $(DIR_COD)/*.o $(DIR_CRY)/*.o > /dev/null 2>&1 || true

install:
	@install -D $(target) $(DESTDIR)/$(BINDIR)/$(target)
	@install -d $(DESTDIR)/$(ETCDIR)/$(target)/
	@install -D ../sounds/*.wav $(DESTDIR)/$(ETCDIR)/$(target)/
	@install -D $(target).8.gz $(DESTDIR)/usr/share/man/man8/$(target).8.gz

uninstall:
	@rm $(DESTDIR)/$(BINDIR)/$(target) || true
	@rm -rf $(DESTDIR)/$(ETCDIR)/$(target)/
	@rm $(DESTDIR)/usr/share/man/man8/$(target).8.gz || true

help:
	@echo 'Cleaning targets:'
	@echo '  clean           - Remove generated files'
	@echo 'Building targets:'
	@echo '  all             - Build transsip (default)'
	@echo 'Setup targets:'
	@echo '  install         - Install transsip'
	@echo '  uninstall       - Uninstall transsip'
	@echo 'Other targets:'
	@echo '  help            - Print this help'

