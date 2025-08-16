# DesktopThing
Version 1.0

A project built using Arduino IDE, some TFT screens, and a raspberry pi pico.

Please be nice this is my first time uploading a project.

This was born out of frustration about the end of remote work and heading back into the office after almost 10 years.
I wanted to make a cute little display to show a slideshow and a nice looking clock to go with it.
A desftop thing to sit on my desk and display some images and the time.

Built using an ILI9341 with an i2c capacitive touch screen and a GC9A01 round 1.28" tft display for the clock.
Uses the built in micro sd card slot on the ILI9341 and an external RTC clock with a battery to keep the time.

The .h pixel map files are used by the TFT_eSPI library to create the analog clock movement.  usersetup.h is used with the TFT_eSPI library to configure the clock display.

ClockFace7.h is an example of what your clock face files should look like.  Use one or use a bunch.  Update the .ino file to match.  I used lcd image converter to make the .h clock faces by finding printable clock faces without hands and sizing them to 240x240 to fit the screen.  Then used the converter to create the pixel map.

I used a powershell script to convert my .jpg files to 320x240 bmp files for the slide show.

The adafruit libraries are used to create the slide show and drive the tft displays.

You can set the delay for the slide show to advance near the top of the .ino file.  I also included the ability to just touch the screen to advance to the next picture if desired.

