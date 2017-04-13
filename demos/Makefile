DEMOS = ws2801-demo cpu-load

DRIVER_DIR = ../driver

all: $(DEMOS)

CFLAGS += -I$(DRIVER_DIR)

$(DEMOS): $(DRIVER_DIR)/ws2801.o common.o

clean:
	rm -f *.o
	rm -f $(DEMOS)
