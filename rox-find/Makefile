CC ?= cc
PKG_CONFIG ?= pkg-config
CFLAGS ?= -O2 -Wall -Wextra -Wformat=2
CPPFLAGS += -DGETTEXT_PACKAGE=\"rox-find\" -DLOCALEDIR=\"/usr/share/locale\"
GTK_CFLAGS := $(shell $(PKG_CONFIG) --cflags gtk+-3.0 gio-2.0)
GTK_LIBS := $(shell $(PKG_CONFIG) --libs gtk+-3.0 gio-2.0)

all: rox-find

rox-find: rox-find.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(GTK_CFLAGS) -o $@ $< $(GTK_LIBS)

clean:
	rm -f rox-find

.PHONY: all clean
