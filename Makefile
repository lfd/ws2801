.PHONY: all clean demos driver

all: demos

demos: driver
	$(MAKE) -C $@

driver:
	$(MAKE) -C $@

clean:
	$(MAKE) -C driver $@
	$(MAKE) -C demos $@
