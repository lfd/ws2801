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
