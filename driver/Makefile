# ws2801 - WS2801 LED driver running in Linux userspace
#
# Copyright (c) - Ralf Ramsauer, 2017
#
# Authors:
#   Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
#
# This work is licensed under the terms of the GNU GPL, version 2.  See
# the COPYING file in the top-level directory.

libws2801.a: ws2801.o
	$(AR) rcs $@ $^

include ../include.mk

ws2801.o: ws2801-user.o ws2801-kernel.o ws2801-common.o
	$(LD) -r -o $@ $^

clean:
	rm -f *.o

install_lib: libws2801.a $(PREFIX_LIB)
	$(INSTALL_LIB) $^

install_header: ws2801.h $(PREFIX_INCLUDE)
	$(INSTALL_INCLUDE) $^

install: install_lib install_header
