DESTDIR   ?=
PREFIX    ?= /usr/local
MANPREFIX ?= $(PREFIX)/man

CFLAGS += -std=c99 -Wall

all: brn

clean:
	rm -f brn *.o

install: all
	install -d $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(MANPREFIX)/man1
	install -m755 brn $(DESTDIR)$(PREFIX)/bin/
	install -m644 brn.1 $(DESTDIR)$(MANPREFIX)/man1/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/brn
	rm -f $(DESTDIR)$(MANPREFIX)/man1/brn.1

.PHONY: all clean install uninstall
