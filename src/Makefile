BIN=a2serclk a2pid a2mon a2term dskread dskwrite bintomon bload brun

all: $(BIN)

clean:
	rm $(BIN) *~

install:
	$(BASH) ./install.sh
	cp $(BIN) /usr/local/bin
