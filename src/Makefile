DESTDIR=/usr/local
SBINDIR=$(DESTDIR)/sbin
BINDIR=$(DESTDIR)/bin
SHAREDIR=$(DESTDIR)/share/a2pi
BIN=a2joy a2joymou a2joypad a2mon a2term fusea2pi a2pidcmd dskread dskwrite bload brun nibread dskformat eddread
SBIN=a2pid
# A2PIDEFS=-DSETSERCLK

all: $(BIN) $(SBIN)

a2pid: a2pid.c gpclk.c
	$(CC) $(A2PIDEFS) a2pid.c -o a2pid

fusea2pi: fusea2pi.c a2lib.c
	$(CC) -I/usr/include/fuse -D_FILE_OFFSET_BITS=64 fusea2pi.c -lfuse -o fusea2pi

clean:
	-rm $(BIN) $(SBIN) *~ *.tar.gz

install:
	-mkdir -p $(BINDIR)
	cp $(BIN) a2mount a2setvd $(BINDIR)
	-mkdir -p $(SBINDIR)
	cp $(SBIN) $(SBINDIR)
	-mkdir -p $(SHAREDIR)
	cp -R ../share/* $(SHAREDIR)
	mv $(SHAREDIR)/A2PI-1.7.PO $(SHAREDIR)/A2VD1.PO
	mv $(SHAREDIR)/UTILS.PO $(SHAREDIR)/A2VD2.PO
	systemctl enable --system $(SHAREDIR)/a2pi.service

uninstall:
	cd $(BINDIR); rm $(BIN) a2mount
	cd $(SBINDIR); rm $(SBIN)
