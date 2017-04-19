export CFLAGS += -Wall -Wstrict-prototypes -Wtype-limits \
		 -Wmissing-declarations -Wmissing-prototypes
export CFLAGS += -O2

all: demos

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
