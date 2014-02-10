.SUFFIXES	=
AFLAGS          = -o $@
ROM = ROM\#062000
DRVR = PIDRIVE\#062000
#
# Image filetypes for CiderPress
#
PLATYPE	= \#ed0000
BINTYPE	= \#060000
SYSTYPE	= \#ff0000
TXTTYPE	= \#040000

all: $(ROM) $(DRVR)

$(ROM): rom.s
	ca65 rom.s -o rom.o
	ld65 -o $(ROM) -C rom.cfg rom.o

$(DRVR): pidrive.s
	ca65 pidrive.s -o pidrive.o
	ld65 -o $(DRVR) -C drvr.cfg pidrive.o
