.PHONY: all clean demos driver

export CFLAGS += -Wall -Wstrict-prototypes -Wtype-limits \
		 -Wmissing-declarations -Wmissing-prototypes
export CFLAGS += -O2

all: demos

demos: driver
	$(MAKE) -C $@

driver:
	$(MAKE) -C $@

clean:
	$(MAKE) -C driver $@
	$(MAKE) -C demos $@
