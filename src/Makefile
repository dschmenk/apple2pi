BIN=a2serclk a2pid a2mon a2term dskread dskwrite bintomon bload brun
all: $(BIN)

fusea2pi: fusea2pi.c a2lib.c
	$(CC) -I/usr/include/fuse -D_FILE_OFFSET_BITS=64 fusea2pi.c -lfuse -o fusea2pi

clean:
	rm $(BIN) *~ fusea2pi

install:
	$(BASH) ./install.sh
	cp $(BIN) /usr/local/bin

fuse-install: fusea2pi
	cp fusea2pi /usr/local/bin
	cp a2mount /usr/local/bin
	chmod +x /usr/local/bin
