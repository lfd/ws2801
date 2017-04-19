CFLAGS += -Wall -Wstrict-prototypes -Wtype-limits \
	  -Wmissing-declarations -Wmissing-prototypes
CFLAGS += -O2

INSTALL ?= install
INSTALL_FILE ?= $(INSTALL) -m 644
INSTALL_LIB ?= $(INSTALL_FILE)
INSTALL_INCLUDE ?= $(INSTALL_FILE)

MKDIR ?= mkdir

PREFIX ?= /usr/local
PREFIX_BIN ?= $(PREFIX)/bin
PREFIX_LIB ?= $(PREFIX)/lib
PREFIX_INCLUDE ?= $(PREFIX)/include

$(PREFIX_BIN):
	$(MKDIR) -p $@

$(PREFIX_LIB):
	$(MKDIR) -p $@

$(PREFIX_INCLUDE):
	$(MKDIR) -p $@

