DEMOS = ws2801-demo cpu-load

all: $(DEMOS)

CFLAGS += -I../

$(DEMOS): ../ws2801.o

clean:
	rm -f *.o
	rm -f $(DEMOS)
