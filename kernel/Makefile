obj-m += ws2801.o

ccflags-y += -Wall -Wstrict-prototypes -Wtype-limits -Wmissing-declarations \
	     -Wmissing-prototypes

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
