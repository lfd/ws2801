libws2801.a: ws2801.o
	$(AR) rcs $@ $^

include ../include.mk

ws2801.o: ws2801-user.o ws2801-kernel.o ws2801-common.o
	$(LD) -r -o $@ $^

clean:
	rm -f *.o
