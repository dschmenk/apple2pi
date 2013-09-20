DESTDIR=/usr/local/bin
SHAREDIR=/usr/share/a2pi
BIN=a2pid a2joy a2joymou a2joypad a2mon a2term fusea2pi dskread dskwrite bload brun
A2PIDEFS=-DSETSERCLK

all: $(BIN)

a2pid: a2pid.c gpclk.c
	$(CC) $(A2PIDEFS) a2pid.c -o a2pid

fusea2pi: fusea2pi.c a2lib.c
	$(CC) -I/usr/include/fuse -D_FILE_OFFSET_BITS=64 fusea2pi.c -lfuse -o fusea2pi

clean:
	-rm $(BIN) *~ *.tar.gz

install:
	cp $(BIN) a2mount $(DESTDIR)
	-mkdir $(SHAREDIR)
	cp ../share/* $(SHAREDIR)
	$(BASH) ./a2pi-setup.sh

uninstall:
	cd $(DESTDIR); rm $(BIN) a2mount
