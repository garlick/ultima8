CHIP=18F14K22

all: hotspot.hex

hotspot.hex: hotspot.c
	picc18 -O$@ --chip=$(CHIP) hotspot.c $(CFLAGS)

clean:
	rm -f *.hex *.hxl *.d
	rm -f *.rlf *.obj *.as *.lst *.sym *.sdb *.cof *.pre *.p1 funclist
	rm -f *.o

# -P<device> -F<hexfile>
# -M     erase/program/verify all memory regions
# -R -T  power and release MCLR after operations
pgm: hotspot.hex
	pk2cmd -PPIC$(CHIP) -F$< -M
# verify that pk2cmd sees a programmer
pk2ver:
	sudo chmod 666 /dev/pickit*
	pk2cmd -P -?V
