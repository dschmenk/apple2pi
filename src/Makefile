PACKAGE=a2pi
VERSION=0.9.0
PREFIX=/usr/local
BIN= a2serclk a2pid a2joy a2joymou a2joypad a2mon a2term dskread dskwrite bintomon bload brun text2merlin merlin2text
BINDIR=$(PREFIX)/bin
DIST=$(PACKAGE)-$(VERSION)
DISTDIR=./$(DIST)

all: $(BIN) fusea2pi

fusea2pi: fusea2pi.c a2lib.c
	$(CC) -I/usr/include/fuse -D_FILE_OFFSET_BITS=64 fusea2pi.c -lfuse -o fusea2pi

clean:
	-rm $(BIN) *~ fusea2pi

install:
	cp $(BIN) fusea2pi a2mount $(BINDIR)
	$(BASH) ./a2pi-setup.sh

uninstall:
	cd $(BINDIR); rm $(BIN) fusea2pi a2mount

dist:
	-rm -rf $(DISTDIR)
	mkdir $(DISTDIR)
	-chmod 777 $(DISTDIR)
	cp ../LICENSE.txt $(DISTDIR)
	cp ../README.md $(DISTDIR)
	mkdir $(DISTDIR)/docs
	cp ../docs/* $(DISTDIR)/docs
	mkdir $(DISTDIR)/src
	cp *.c $(DISTDIR)/src
	cp *.sh $(DISTDIR)/src
	cp *.conf $(DISTDIR)/src
	cp *.bin $(DISTDIR)/src
	cp *.mon $(DISTDIR)/src
	cp *.PO $(DISTDIR)/src
	cp a2mount execfile Makefile $(DISTDIR)/src
	-chmod -R a+r $(DISTDIR)
	tar czf $(DIST).tar.gz $(DISTDIR)
	rm -rf $(DISTDIR)