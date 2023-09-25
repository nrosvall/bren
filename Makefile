DESTDIR   ?=
PREFIX    ?= /usr/local
MANPREFIX ?= $(PREFIX)/man
DEBUG ?=
CFLAGS += $(DEBUG) -std=c99 -Wall -lguile-3.0 -lgmp -lm -I/usr/include/guile/3.0

all: bren

clean:
	rm -f bren *.o

install: all
	install -d $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(MANPREFIX)/man1
	install -m755 brn $(DESTDIR)$(PREFIX)/bin/
	install -m644 brn.1 $(DESTDIR)$(MANPREFIX)/man1/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/bren
	rm -f $(DESTDIR)$(MANPREFIX)/man1/bren.1

.PHONY: all clean install uninstall
