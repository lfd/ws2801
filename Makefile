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

.PHONY: all clean demos driver modules modules_install
