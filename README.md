### Ultima 8 Drive Corrector

I decided to part out my Celestron Ultima 8 so this is no longer
being worked on.  What is here might be useful to someone:

* Rework of Michael Covington's ALCOR drive corrector design for
  a more modern PIC microprocessor, in C.
  This was working to drive the factory Celestron synchronous AC
  motor in RA, but I never resolved problems with the PIC rebooting when
  operating a noisy add-on Moto declination motor.

* PIC encoder interface and Linux software that emulates Tangent/BBox/JMI
  setting circles.  This was run on an OpenWRT box in the field, and
  used with Sky Safari.
