DEMOS = ws2801-demo cpu-load rgb-demo

all: $(DEMOS)

include ../include.mk

$(DEMOS): ws2801.o

clean:
	rm -f *.o
	rm -f $(DEMOS)
