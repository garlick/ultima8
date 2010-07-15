PICP=picp
PICP_TTY=/dev/ttyUSB0
CHIP=16f84a
SIMCHIP=pic16f84

alcorF84A.hex: alcorF84A.asm
	gpasm -r HEX -o $@ $<

clean:
	rm -f *.hex *.cod *.lst

pgm: alcorF84A.hex
	sudo $(PICP) $(PICP_TTY) $(CHIP) -ef
	sudo $(PICP) $(PICP_TTY) $(CHIP) -bp
	sudo $(PICP) $(PICP_TTY) $(CHIP) -wp $<

sim: alcorF84A.hex
	gpsim -p$(SIMCHIP) alcorF84A.cod alcorF84A.asm
