### Celestron Ultima 8 Drive Corrector Design

#### Introduction

The [Celestron Ultima 8](http://www.telescopebluebook.com/sct/celestron.htm)
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
[Alcor - A Microcontroller-Based Control Circuit for Conventional AC Telescope Drives](http://www.ai.uga.edu/mc/alcor/alcor.pdf)
by Michael A. Covington, which describes a variable frequency, two phase
drive circuit for AC syncronous motors used in telescope clock drives.

In addition to the RA and DEC motor control in Covington's circuit,
my adaptation adds focus motor support, PWM motor speed control,
support for reading all 7 buttons on the stock Celestron handbox,
in-circuit programming, and an I<sup>2</sup>C slave port for interfacing to
other electronics.  The new firmware is programmed in C instead of PIC
assembly. These enhancements are made possible by updating the Alcor's
16F84A microprocessor to a modern part.

![](http://wiki.ultima8drivecorrector.googlecode.com/git/images/v2.png)

#### PIC Microprocessor

The 
[PIC18F14K22](http://www.microchip.com/wwwproducts/Devices.aspx?dDocName=en538160) microprocessor is used in this design.
It is a 20-pin part with 64MHz internal clock, one PWM output,
hardware I<sup>2</sup>C slave, internal pull-ups on PORTA, PORTB inputs,
and several analog inputs.

It can be programmed in-situ via the ICSP header, using the
Microchip [PICkit 2 programmer](http://www.microchipdirect.com/productsearch.aspx?Keywords=pg164120) and free [Hi-TECH C compiler](http://www.htsoft.com/)
(Windows and Linux support for both available).

#### Power Supply

12VDC power is supplied to the base via a 2.1 mm I.D., 5.5 mm O.D.
[coaxial power connector](http://en.wikipedia.org/wiki/Coaxial_DC_Power_Connectors), the type of jack used in the original drive electronics, and by many
other telescopes.

The inner conductor is the positive.  Most panel-mount jacks will allow the
negative outer conductor to become electrically connected to the metal base
housing.  This is OK provided the focus/DEC jacks are insulated from the
panel housing. A series 1N5817 Shottky diode protects the circuit from
accidental polarity reversal.  The 12V source is fused at 1A.

The LM7805 voltage regulator converts 12VDC power to 5VDC required by
the circuit.

5V and 12V power is wired to the I<sup>2</sup>C connector for powering external
encoders, DC motors, etc..

#### LEDs

One LED indicator is wired directly to 12V power and lights when the
base is energized.

The second LED is connected to a PIC digital output.
Software blinks this LED for a few seconds on startup so that it is
apparent if the PIC manages to reset itself.  (Inductive spikes are
a potential problem here).  The LED also lights when one of the handbox
buttons is pressed.

#### AC Motor

The hardware design is covered in detail in Covington's paper.

The drive corrector employs a common 120V primary, 12VCT secondary power
transformer connected in reverse to convert the two-phase, low voltage
signal produced by the pair of
[IRL530N](http://www.irf.com/product-info/datasheets/data/irl530n.pdf)
logic level input HEXFET's to a high voltage signal that approximates a
sine wave.  A 6.0 VA transformer such as the
[Triad FS12-500](http://www.netsuite.com/core/media/media.nl?id=2256&c=ACCT126831&h=993caf6ef1288106eda6&_xt=.pdf) can be used.

An on-chip timer is used to generate the AC signal rather
than counting assembly language instructions as was done in the original Alcor.

#### DC Motors

Apparently Celestron resold the
[JMI Motodec](http://www.jimsmobile.com/buy_motodec.htm) with a Celstron
label for this telescope.  Aftermarket electric focusers such as the JMI
Motofocus are also available for this telescope.  Both are based on 12V
gearhead motors.

Motors attach to the base via 3.5 mm
[TRS jacks](http://en.wikipedia.org/wiki/TRS_connector)
with a right-angle adapter.  Since the polarity of the motors
is reversed depending on the direction of adjustment, the jacks should be
completely insulated from the metal base housing which as described above
may be electrically connected to the outer (negative) pole of the 12VDC
power jack.  I bought plastic jacks and epoxied them to the faceplate.

Half of a Texas Instruments
[SN754410 quad half H-bridge](http://www.ti.com/lit/ds/symlink/sn754410.pdf)
drives each motor.  The enable input of the H-bridge chip is connected to
the PIC PWM output, so speed can be adjusted without losing torque by
altering the duty cycle.  The _A_ inputs are connected to PIC digital outputs.

| 1A | 2A | 3A | 4A | focus    | declination |
|----|----|----|----|----------|-------------|
| 0  | 0  | 0  | 1  | off      | forward     |
| 0  | 0  | 1  | 0  | off      | reverse     |
| 0  | 1  | 0  | 0  | forward  | off         |
| 1  | 0  | 0  | 0  | reverse  | off         |

Braking is achieved by leaving the enable
line active when setting both motor lines to the same value.

To reduce inductive kickback, clamping diodes were added to the motor
outputs in the v2 design.

#### Handbox/Autoguiding Jack

The base is provisioned with a
[RJ-12 jack](http://en.wikipedia.org/wiki/Registered_jack)
for a handbox control input.  The stock Celestron handbox is wired as follows:

![](https://github.com/garlick/ultima8/blob/master/base/images/handbox.png)

The handbox includes a common for focus/lamp and a common for ra/dec.
The ra/dec common is grounded.
The focus/lamp common is attached to the 2.5V output of a voltage divider
consisting of two 2.2K resistors.
The shared lines, RA+/FOCUS-, DEC-/FOCUS+, and RA+/LAMP are pulled high
with 2.2K resistors and wired to ADC inputs.

The ADC inputs read 5V when no buttons are pressed,
0V when the ra/dec button is pressed, and

V<sub>out</sub> = (R<sub>2</sub> / (R<sub>1</sub> + R<sub>2</sub>)) x V<sub>in</sub> = (2.2K / (1.1K + 2.2K)) x 5V = 3.33V

when the focus/lamp button is pressed.

The handbox jack pinout is ST-4 compatible and thus is suitable for
autoguiding.  For more on this see this
[shoestring astronomy forum discussion](http://forum.shoestringastronomy.com/viewtopic.php?f=3&t=288).

== I<sup>2</sup>C Slave Port ==

The microprocessor's I<sup>2</sup>C and base power lines are brought out to
a female Mini-DIN8 jack mounted in the Ultima base.  Although the base can
operate standalone, this allows the Ultima base to optionally
be part of a microprocessor controlled system that interfaces to a
smart handbox or computer.

![](https://github.com/garlick/ultima8/blob/master/base/images/i2c.png)

The I<sup>2</sup>C protocol implemented in firmware is based on single
byte registers.  To write _value_ into _regnum_, the master performs
```
WRITE 2 regnum value
```

To read _value_ from _regnum_, the master performs
```
WRITE 1 regnum
READ value
```

After a READ or a WRITE, subsequent READ ops sample the previously
addressed _regnum_.  The registers are assigned as follows:

| regnum | name     | op    | description |
|--------|----------|-------|-------------|
| 0      | buttons  | read  | reads handbox buttons (see below) |
| 0      | ibuttons | write | write virtual handbox buttons (see below) |

Handbox buttons are encoded within reg 0 as follows:

| bit  | name  |
|------|-------|
| 0x01 | NORTH |
| 0x02 | SOUTH |
| 0x04 | EAST  |
| 0x08 | WEST  |
| 0x10 | FOCIN |
| 0x20 | FOCOUT|
| 0x40 | LAMP  |
