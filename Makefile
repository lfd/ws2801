# ws2801 - WS2801 LED driver running in Linux userspace
#
# Copyright (c) - Ralf Ramsauer, 2017
#
# Authors:
#   Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
#
# This work is licensed under the terms of the GNU GPL, version 2.  See
# the COPYING file in the top-level directory.

all: demos

include include.mk

demos: driver
	$(MAKE) -C $@

driver:
	$(MAKE) -C $@

modules modules_install:
	$(MAKE) -C kernel $@

clean:
	$(MAKE) -C driver $@
	$(MAKE) -C demos $@
	$(MAKE) -C kernel $@

install:
	$(MAKE) -C driver $@
	$(MAKE) -C demos $@

.PHONY: all clean demos driver modules modules_install
