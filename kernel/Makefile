# ws2801 - WS2801 LED driver running in Linux userspace
#
# Copyright (c) - Ralf Ramsauer, 2017
#
# Authors:
#   Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
#
# This work is licensed under the terms of the GNU GPL, version 2.  See
# the COPYING file in the top-level directory.

KDIR ?= /lib/modules/`uname -r`/build
kbuild = -C $(KDIR) M=$$PWD $@
kmake = $(Q)$(MAKE) $(kbuild)

all: modules

obj-m := ws2801.o

ccflags-y := -Wall -Wstrict-prototypes -Wtype-limits -Wmissing-declarations \
	     -Wmissing-prototypes

clean modules:
	$(kmake)

modules_install: modules
	$(kmake)
