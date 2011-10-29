CHIP=18F14K22
CFLAGS=-DMOTOR_HZ=60
SRCS=main.c

all: main.hex

main.hex: $(SRCS) freq.h
	picc18 -O$@ --chip=$(CHIP) $(SRCS) $(CFLAGS)

freq.h: genfreq
	./genfreq >$@

clean:
	rm -f *.hex *.hxl *.d
	rm -f *.rlf *.obj *.as *.lst *.sym *.sdb *.cof *.pre *.p1 funclist
	rm -f *.o genfreq freq.h

# -P<device> -F<hexfile>
# -M     erase/program/verify all memory regions
# -R -T  power and release MCLR after operations
pgm: main.hex
	pk2cmd -PPIC$(CHIP) -F$< -M
# verify that pk2cmd sees a programmer
pk2ver:
	sudo chmod 666 /dev/pickit*
	pk2cmd -P -?V
