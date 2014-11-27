#summary Wifi hotspot for Ultima 8 Drive Corrector

_Note: this design is preliminary and has not yet been fully prototyped._

### Introduction

An [ASUS WL-520GU router]
(http://mightyohm.com/blog/2008/10/building-a-wifi-radio-part-3-hacking-the-asus-wl-520gu/)
running [OpenWRT](http://openwrt.org) (an embeddded Linux distro)
is packaged with a homebrew microcontroller board in a project box.
The two internal boards communicate over the router's serial port.

The microcontroller board implements a real time clock, LCD, pushbuttons,
and interfaces for RA/DEC encoders, focuser, and ultima base as depicted
in the following diagram.

![Hot spot architecture]
(http://github.com/garlick/ultima8/blob/master/hotspot/images/hotspotarch.png)

The ASUS router runs a small program that allows the telescope to be
controlled (push-to) by an iphone or ipad running Sky Safari.
The program emulates NGC Max/Tangent/BBox protocol, and presents it
on port 4030.

#### Prelim Design

![Schematic 1]
(http://github.com/garlick/ultima8/blob/master/hotspot/images/hotspot1.png)

![Schematic 2]
(http://github.com/garlick/ultima8/blob/master/hotspot/images/hotspot2.png)

![Schematic 3]
(http://github.com/garlick/ultima8/blob/master/hotspot/images/hotspot3.png)

#### Early prototype

The ASUS board is unboxed and mounted on a
[sintra foam board]([http://www.solarbotics.com/products/sin3mm/)
cut to fit a 7.3in x 5.3in x 3.0in ABS enclosure (Jameco part no. 141859).
A [PICO-PSU wide range input ATX power supply]
(http://www.mini-box.com/PicoPSU-120-WI-25-12-25V-DC-DC-ATX-power-supply)
is glued to the sintra.
A 10-pin wire wrap header is glued to the ASUS board, and power supply
and ASUS serial pins are jumpered to it.  

The enclosure is cut to allow the router RJ45's, pushbuttons, and SMA
antenna connector to be accessed on one side.  Note that the board needs
to be pressed right up against the enclosure in order to allow enough
SMA connector to protrude to make good contact with the antenna.

![Enclosure]
(http://github.com/garlick/ultima8/blob/master/hotspot/images/hotspotbox.png)

Connectors for 
* [JMI RA/DEC encoders](http://www.jimsmobile.com/pdf_docs/sgt_documentation.pdf) (RJ-45)
* a JMI NGF focuser with DC motor and encoder (RJ-12) 
* Ultima base I2C (MINI-DIN8, Jameco part no. 2076771)

are epoxied to the case and wired to a 16 pin wire wrap header.

![Enclosure 2]
(http://github.com/garlick/ultima8/blob/master/hotspot/images/hotspotbox2.png)

The main board is attached to the ABS case lid and wired to the other electronics with two ribbon cables.
