CC=picc
PICP=picp
PICP_TTY=/dev/ttyUSB0
CHIP=16f84a
FREQ=4000000
SRCS=main.c

all: main.hex

main.hex: $(SRCS)
	$(CC) -D_XTAL_FREQ=$(FREQ) --chip=$(CHIP) $(SRCS)

clean:
	rm -f *.hex *.hxl
	rm -f *.rlf *.obj *.as *.lst *.sym *.sdb *.cof *.pre *.p1 funclist

# erase whole chip, verify program mem is blank, the write program mem
pgm: main.hex
	sudo $(PICP) $(PICP_TTY) $(CHIP) -ef
	sudo $(PICP) $(PICP_TTY) $(CHIP) -bp
	sudo $(PICP) $(PICP_TTY) $(CHIP) -wp $<
