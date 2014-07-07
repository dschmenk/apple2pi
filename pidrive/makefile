.SUFFIXES	=
AFLAGS          = -o $@
ROM = ROM.BIN
VDRV = PIDRIVE\#062000
VCLK = PICLOCK\#062000
#
# Image filetypes for CiderPress
#
PLATYPE	= \#ed0000
BINTYPE	= \#060000
SYSTYPE	= \#ff0000
TXTTYPE	= \#040000

all: $(ROM) $(VDRV) $(VCLK)

clean:
	-rm $(ROM) $(VDRV) $(VCLK) *.o *~

$(ROM): rom.s
	ca65 rom.s -o rom.o
	ld65 -o $(ROM) -C rom.cfg rom.o

$(VDRV): pidrive.s
	ca65 pidrive.s -o pidrive.o
	ld65 -o $(VDRV) -C drvr.cfg pidrive.o

$(VCLK): piclock.s
	ca65 piclock.s -o piclock.o
	ld65 -o $(VCLK) -C drvr.cfg piclock.o
