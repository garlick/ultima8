#summary Celestron Ultima 8 Aux I2C Interface Module

This document describes an I^2^C module which adds support for
digital setting circle encoders and an encoder-equipped focus motor
to the I^2^C based Ultima 8 electronics system.

The module uses the same
[http://www.microchip.com/wwwproducts/Devices.aspx?dDocName=en538160 PIC18F14K22] microprocessor as the base.  The focus motor driver uses the same
[http://www.ti.com/lit/ds/symlink/sn754410.pdf SN754410 quad half H-bridge]
used in the base.

Three two-phase quadrature encoder inputs are added.

The module is controlled via I^2^C and derives +5V and +12V power from the
Mini-DIN8 I^2^C connector on the base.

The module has two Mini-DIN8 connectors to allow daisy-chaining.

http://wiki.ultima8drivecorrector.googlecode.com/git/images/aux.png

== I^2^C Protocol ==

The I^2^C protocol implemented in firmware is based on two-byte registers.
To write _value_ into _regnum_, the master performs
 WRITE 2 _regnum_ _lsb-value_ _msb-value_

To read _value_ from _regnum_, the master performs
 WRITE 1 _regnum_

 READ _lsb-value_ _msb-value_

After a READ or a WRITE, subsequent READ ops sample the previously addressed _regnum_.  The registers are assigned as follows:

|| *regnum* || *name* || *description* ||
|| 0 || ra-enc || relative RA encoder position ||
|| 1 || dec-enc || relative DEC encoder position ||
|| 2 || focus-enc || relative Focus encoder position ||
|| 3 || focus-motor || motor enable (_lsb_: 0=off, 1=in, 2=out; _msb_: PWM value||
