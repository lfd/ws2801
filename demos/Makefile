DEMOS = ws2801-demo

all: $(DEMOS)

CFLAGS += -I../

$(DEMOS): ../ws2801.o

clean:
	rm -f *.o
	rm -f $(DEMOS)
