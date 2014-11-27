#summary Wifi hotspot for Ultima 8 Drive Corrector

Note: this design is preliminary and has not yet been fully prototyped.

== Introduction ==

An [http://mightyohm.com/blog/2008/10/building-a-wifi-radio-part-3-hacking-the-asus-wl-520gu/ ASUS WL-520GU router] running [http://openwrt.org OpenWRT]
(an embeddded Linux distro) is packaged with a homebrew microcontroller
board in a project box.  The two internal boards communicate over the
router's (undocumented) serial port.

The microcontroller board implements a real time clock, LCD, pushbuttons,
and interfaces for RA/DEC encoders, focuser, and ultima base as depicted
in the following diagram.

http://wiki.ultima8drivecorrector.googlecode.com/git/images/hotspotarch.png

The ASUS router runs
[http://www.indilib.org/index.php?title=INDI_Server INDI server]
and custom software to allow a laptop running Linux and
[http://edu.kde.org/kstars/ Kstars]
to control the telescope (push-to) wirelessly.

In addition, the telescope can be controlled by an iphone or ipad running Starmap Pro if we can emulate LX200 protocol on a telnet port as
described on the Starmap Pro [http://albireoplanetarium.com/?telescope telescope page].

Finally the router has a USB 2.0 port so it should be possible to
attach a USB camera to INDI and use INDI-enabled Linux imaging and autoguiding packages such as [http://www.astro.louisville.edu/software/xmccd/ Xmccd] and
[http://code.google.com/p/open-phd-guiding/ Open Phd Guiding].

== Prelim Design ==

http://wiki.ultima8drivecorrector.googlecode.com/git/images/hotspot1.png
http://wiki.ultima8drivecorrector.googlecode.com/git/images/hotspot2.png
http://wiki.ultima8drivecorrector.googlecode.com/git/images/hotspot3.png

== Early prototype ==

The ASUS board is unboxed and mounted on a
[http://www.solarbotics.com/products/sin3mm/ sintra PVC foam board] cut to
fit a 7.3in x 5.3in x 3.0in ABS enclosure (Jameco part no. 141859).
A [http://www.mini-box.com/PicoPSU-120-WI-25-12-25V-DC-DC-ATX-power-supply PICO-PSU wide range input ATX power supply] is glued to 
the sintra.
A 10-pin wire wrap header is glued to the ASUS board, and power supply
and ASUS serial pins are jumpered to it.  

The enclosure is cut to allow the router RJ45's, pushbuttons, and SMA antenna connector to be accessed on one side.  Note that the board needs to be pressed right up against the enclosure in order to allow enough SMA connector to protrude to make good contact with the antenna.

http://wiki.ultima8drivecorrector.googlecode.com/git/images/hotspotbox.png

Connectors for 
  * [http://www.jimsmobile.com/pdf_docs/sgt_documentation.pdf JMI ra/dec encoders] (RJ-45)
  * a JMI NGF focuser with DC motor and encoder (RJ-12)
  * Ultima base I2C (MINI-DIN8, Jameco part no. 2076771)
are epoxied to the case and wired to a 16 pin wire wrap header.

http://wiki.ultima8drivecorrector.googlecode.com/git/images/hotspotbox2.png

The main board will be attached to the ABS case lid and wired to the other electronics with the two ribbon cables.

== Other Crazy Ideas ==

Plenty of board real estate, power supply capacity, and the internal I2C bus leaves room for expansion.  Here are some ideas:
  * Take apart a serial GPS mouse and interface it to the spare serial port on the encoder PIC.
  * Add LX200 serial interface for use in front of a Meade LX200 or compatible.
  * Interface Mitutoyo micrometer and DC motor on Van Slyke focusers.
  * Implement time critical portions of Mel Bartels DOS stepper control program on the PIC and add a parallel port to connect to Bartels hardware.

But first things first.
