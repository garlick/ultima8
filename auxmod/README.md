### Aux I2C Interface Module

This document describes an I<sup>2</sup>C module which adds support for
digital setting circle encoders and an encoder-equipped focus motor
to the I<sup>2</sup>C based Ultima 8 electronics system.

The module uses the same
[PIC18F14K22](http://www.microchip.com/wwwproducts/Devices.aspx?dDocName=en538160)
microprocessor as the base.  The focus motor driver uses the same
[SN754410 quad half H-bridge](http://www.ti.com/lit/ds/symlink/sn754410.pdf)
used in the base.

Three two-phase quadrature encoder inputs are added.

The module is controlled via I<sup>2</sup>C and derives +5V and +12V power from the
Mini-DIN8 I<sup>2</sup>C connector on the base.

The module has two Mini-DIN8 connectors to allow daisy-chaining.

![](https://github.com/garlick/ultima8/blob/master/auxmod/schem/aux.png)

#### I<sup>2</sup>C Protocol

The I<sup>2</sup>C protocol implemented in firmware is based on two-byte registers.
To write _value_ into _regnum_, the master performs
```
WRITE 2 regnum lsb-value msb-value
```
To read _value_ from _regnum_, the master performs
```
WRITE 1 regnum
READ lsb-value msb-value
```
After a READ or a WRITE, subsequent READ ops sample the previously addressed _regnum_.  The registers are assigned as follows:

| regnum | name        | description |
|--------|-------------|-------------|
| 0      | ra-enc      | relative RA encoder position |
| 1      | dec-enc     | relative DEC encoder position |
| 2      | focus-enc   | relative Focus encoder position |
| 3      | focus-motor | motor enable (lsb: 0=off, 1=in, 2=out; msb: PWM value)|
