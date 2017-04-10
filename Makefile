.PHONY all: demos

demos: ws2801.o
	$(MAKE) -C $@

clean:
	$(MAKE) -C demos $@
	rm *.o
