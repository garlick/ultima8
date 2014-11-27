#summary Celestron Ultima 8 Drive Corrector Design (current v2)

== Introduction ==


The [http://www.telescopebluebook.com/sct/celestron.htm Celestron Ultima 8]
is an 8-inch SCT telescope with single-axis drive. In the non-PEC variant
of the Ultima, the base contains an inverter circuit used to convert 12VDC
to 120VAC used by the AC synchronous motor that clocks the telescope in
right ascension. A plug-in hand paddle is used to alter the frequency of
the 120VAC supply to correct the motor speed during astrophotography.
Provision is also made to drive an optional/aftermarket DC gearhead motor
for adjustments in declination.

My drive electronics failed.   An alternative to repairing the original
electronics is to remove the lead acid battery and the old drive electronics
and replace them with an enhanced, microprocessor-based drive corrector
circuit presented here.

The new drive corrector is based upon a design presented in this paper:
[http://www.ai.uga.edu/mc/alcor/alcor.pdf Alcor - A Microcontroller-Based Control Circuit for Conventional AC Telescope Drives]
by Michael A. Covington, which describes a variable frequency, two phase
drive circuit for AC syncronous motors used in telescope clock drives.

In addition to the RA and DEC motor control in Covington's circuit,
my adaptation adds focus motor support, PWM motor speed control,
support for reading all 7 buttons on the stock Celestron handbox,
in-circuit programming, and an I^2^C slave port for interfacing to
other electronics.  The new firmware is programmed in C instead of PIC
assembly. These enhancements are made possible by updating the Alcor's 16F84A microprocessor to a modern part.

http://wiki.ultima8drivecorrector.googlecode.com/git/images/v2.png

== PIC Microprocessor ==

The [http://www.microchip.com/wwwproducts/Devices.aspx?dDocName=en538160 PIC18F14K22] microprocessor is used in this design.
It is a 20-pin part with 64MHz internal clock, one PWM output,
hardware I^2^C slave, internal pull-ups on PORTA, PORTB inputs,
and several analog inputs.

It can be programmed in-situ via the ICSP header, using the
Microchip [http://www.microchipdirect.com/productsearch.aspx?Keywords=pg164120 PICkit 2 programmer]
and free 
[http://www.htsoft.com/ Hi-TECH C compiler] (Windows and Linux
support for both available).

== Power Supply ==

12VDC power is supplied to the base via a 2.1 mm I.D., 5.5 mm O.D.
[http://en.wikipedia.org/wiki/Coaxial_DC_Power_Connectors coaxial power connector],
the type of jack used in the original drive electronics, and by many other
telescopes.

The inner conductor is the positive.  Most panel-mount jacks will allow the
negative outer conductor to become electrically connected to the metal base
housing.  This is OK provided the focus/DEC jacks are insulated from the
panel housing. A series 1N5817 Shottky diode protects the circuit from
accidental polarity reversal.  The 12V source is fused at 1A.

The LM7805 voltage regulator converts 12VDC power to 5VDC required by
the circuit.

5V and 12V power is wired to the I^2^C connector for powering external
encoders, DC motors, etc..

== LEDs ==

One LED indicator is wired directly to 12V power and lights when the
base is energized.

The second LED is connected to a PIC digital output.
Software blinks this LED for a few seconds on startup so that it is
apparent if the PIC manages to reset itself.  (Inductive spikes are
a potential problem here).  The LED also lights when one of the handbox
buttons is pressed.

== AC Motor ==

The hardware design is covered in detail in Covington's paper.

The drive corrector employs a common 120V primary, 12VCT secondary power
transformer connected in reverse to convert the two-phase, low voltage
signal produced by the pair of
[http://www.irf.com/product-info/datasheets/data/irl530n.pdf IRL530N]
logic level input HEXFET's to a high voltage signal that approximates a sine wave.
A 6.0 VA transformer such as the [http://www.netsuite.com/core/media/media.nl?id=2256&c=ACCT126831&h=993caf6ef1288106eda6&_xt=.pdf Triad FS12-500] can be used.

An on-chip timer is used to generate the AC signal rather
than counting assembly language instructions as was done in the original Alcor.

== DC Motors ==

Apparently Celestron resold the
[http://www.jimsmobile.com/buy_motodec.htm JMI Motodec] with a Celstron
label for this telescope.  Aftermarket electric focusers such as the JMI
Motofocus are also available for this telescope.  Both are based on 12V
gearhead motors.

Motors attach to the base via 3.5 mm [http://en.wikipedia.org/wiki/TRS_connector TRS jacks] with a right-angle adapter.  Since the polarity of the motors
is reversed depending on the direction of adjustment, the jacks should be
completely insulated from the metal base housing which as described above
may be electrically connected to the outer (negative) pole of the 12VDC
power jack.  I bought plastic jacks and epoxied them to the faceplate.

Half of a Texas Instruments
[http://www.ti.com/lit/ds/symlink/sn754410.pdf SN754410 quad half H-bridge]
drives each motor.  The enable input of the H-bridge chip is connected to
the PIC PWM output, so speed can be adjusted without losing torque by
altering the duty cycle.  The _A_ inputs are connected to PIC digital outputs.

|| *1A* || *2A* || *3A* || *4A* || *focus* || *declination* ||
|| 0  || 0  || 0  || 1  || off         || forward ||
|| 0  || 0  || 1  || 0  || off         || reverse ||
|| 0  || 1  || 0  || 0  || forward     || off ||
|| 1  || 0  || 0  || 0  || reverse     || off ||

Braking is achieved by leaving the enable
line active when setting both motor lines to the same value.

To reduce inductive kickback, clamping diodes were added to the motor
outputs in the v2 design.

== Handbox/Autoguiding Jack ==

The base is provisioned with a
[http://en.wikipedia.org/wiki/Registered_jack RJ-12 jack] for a handbox
control input.  The stock Celestron handbox is wired as follows:

http://wiki.ultima8drivecorrector.googlecode.com/git/images/handbox.png

The handbox includes a common for focus/lamp and a common for ra/dec.
The ra/dec common is grounded.
The focus/lamp common is attached to the 2.5V output of a voltage divider
consisting of two 2.2K resistors.
The shared lines, RA+/FOCUS-, DEC-/FOCUS+, and RA+/LAMP are pulled high
with 2.2K resistors and wired to ADC inputs.

The ADC inputs read 5V when no buttons are pressed,
0V when the ra/dec button is pressed, and

 V,,out,, = (R,,2,, / (R,,1,, + R,,2,,)) x V,,in,, = (2.2K / (1.1K + 2.2K)) x 5V = 3.33V

when the focus/lamp button is pressed.

The handbox jack pinout is ST-4 compatible and thus is suitable for
autoguiding.  For more on this see this
[http://forum.shoestringastronomy.com/viewtopic.php?f=3&t=288 shoestring astronomy forum discussion].

== I^2^C Slave Port ==

The microprocessor's I^2^C and base power lines are brought out to a female
Mini-DIN8 jack mounted in the Ultima base.  Although the base can
operate standalone, this allows the Ultima base to optionally
be part of a microprocessor controlled system that interfaces to a smart handbox or computer.

http://wiki.ultima8drivecorrector.googlecode.com/git/images/i2c.png

The I^2^C protocol implemented in firmware is based on single byte registers.
To write _value_ into _regnum_, the master performs
 WRITE 2 _regnum_ _value_

To read _value_ from _regnum_, the master performs
 WRITE 1 _regnum_

 READ _value_

After a READ or a WRITE, subsequent READ ops sample the previously addressed _regnum_.  The registers are assigned as follows:

|| *regnum* || *name* || *op* || *description* ||
|| 0 || buttons || read ||  reads handbox buttons (see below) ||
|| 0 || ibuttons || write || write virtual handbox buttons (see below) ||

Handbox buttons are encoded within reg 0 as follows:

|| *bit* || *name* ||
|| 0x01 || NORTH ||
|| 0x02 || SOUTH ||
|| 0x04 || EAST ||
|| 0x08 || WEST ||
|| 0x10 || FOCIN ||
|| 0x20 || FOCOUT ||
|| 0x40 || LAMP ||