.PHONY all: demos

ws2801.o: ws2801-user.o
	$(LD) -r -o $@ $^

demos: ws2801.o
	$(MAKE) -C $@

clean:
	$(MAKE) -C demos $@
	rm -f *.o
