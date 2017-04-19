DEMOS = ws2801-demo cpu-load rgb-demo

DRIVER_DIR = ../driver

all: $(DEMOS)

include ../include.mk

CFLAGS += -I$(DRIVER_DIR)
LDFLAGS = -pthread

$(DEMOS): $(DRIVER_DIR)/ws2801.o common.o

clean:
	rm -f *.o
	rm -f $(DEMOS)
