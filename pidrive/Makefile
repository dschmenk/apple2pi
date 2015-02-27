.SUFFIXES	=
AFLAGS          = -o $@
SIROM = SIROM\#062000
PIROM = PIROM\#062000
VDRV = PIDRIVE\#062000
VCLK = PICLOCK\#062000
#
# Image filetypes for CiderPress
#
PLATYPE	= \#ed0000
BINTYPE	= \#060000
SYSTYPE	= \#ff0000
TXTTYPE	= \#040000

all: $(SIROM) $(PIROM) $(VDRV) $(VCLK)

clean:
	-rm $(SIROM) $(PIROM) $(VDRV) $(VCLK) *.o *~

$(SIROM): sirom.s romincs.s cxrom.s c8rom.s
	ca65 sirom.s -o sirom.o
	ld65 -o $(SIROM) -C sirom.cfg sirom.o

$(PIROM): pirom.s romincs.s cxrom.s c8rom.s
	ca65 pirom.s -o pirom.o
	ld65 -o $(PIROM) -C pirom.cfg pirom.o

$(VDRV): pidrive.s
	ca65 pidrive.s -o pidrive.o
	ld65 -o $(VDRV) -C drvr.cfg pidrive.o

$(VCLK): piclock.s
	ca65 piclock.s -o piclock.o
	ld65 -o $(VCLK) -C drvr.cfg piclock.o
