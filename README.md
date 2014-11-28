=== Ultima 8 Drive Corrector

I decided to part out my Celestron Ultima 8 so this is no longer
being worked on.  What is here might be useful to someone:

* Rework of Michael Covington's ALCOR drive corrector design for
  a more modern PIC microprocessor and fit in the Ultima 8 drive base.
  This was working for RA but I had problems with the PIC rebooting
  when operating a noisey add-on MotoDEC declination motor.

* PIC encoder interface and Linux software that emulates Tangent/BBox/JMI
  setting circles.  This was run on an OpenWRT box in the field, and
  used with Sky Safari.
