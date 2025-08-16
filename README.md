# DesktopThing
Version 1.0
Please be nice this is my first time uploading a project.

A project built using Arduino IDE, some TFT screens, an RTC module, and a raspberry pi pico.

This was born out of frustration about the end of remote work and heading back into the office after almost 10 years.
I wanted to make a cute little display to show a slideshow and a nice looking clock to go with it.
A desktop thing to sit on my desk and display some images and the time.

Built using an ILI9341 with an i2c capacitive touch screen and a GC9A01 round 1.28" tft display for the clock.
Uses the built in micro sd card slot on the ILI9341 and an external RTC clock with a battery to keep the time.

The .h pixel map files are used by the TFT_eSPI library to create the analog clock movement.  usersetup.h is used with the TFT_eSPI library to configure the clock display.

ClockFace7.h is an example of what your clock face files should look like.  Use one or use a bunch.  Update the .ino file to match.  I used lcd image converter to make the .h clock faces by finding printable clock faces without hands and sizing them to 240x240 to fit the screen.  Then used the converter to create the pixel map.

I used a powershell script to convert my .jpg files to 320x240 bmp files for the slide show.

The adafruit libraries are used to create the slide show and drive the tft displays.

You can set the delay for the slide show to advance near the top of the .ino file.  I also included the ability to just touch the screen to advance to the next picture if desired.

I crerated a housing in FreeCAD to hold my custom setup.  I used a prototype board and soldered the pico and some connectors to the board along with the RTC.  Cad files can be found in the cad files folder.

Full list of hardware I used:

2.8 inch 240x320 IPS Capacitive Touch Screen SPI Serial ILI9341V Driver LCD Display Module for Arduino
https://www.amazon.com/dp/B0CMGF4Q68?ref_=ppx_hzsearch_conn_dt_b_fed_asin_title_1&th=1

1.28 inch TFT LCD Display Module Round RGB 240 * 240 GC9A01 Driver 4 Wire SPI
https://www.amazon.com/dp/B0BL76Q6WS?ref_=ppx_hzsearch_conn_dt_b_fed_asin_title_1

Raspberry Pi Pico2 (also works with earlier pico) I did solder pins to it as it made it easier to prototype and solder tot he PCB.
https://www.amazon.com/dp/B0DCLF3QYB?ref_=ppx_hzsearch_conn_dt_b_fed_asin_title_2&th=1

SanDisk Ultra SDSQUNS-016G-GN3MN 16GB 80MB/s UHS-I Class 10 microSDHC Card
https://www.amazon.com/dp/B074B4P7KD?ref=ppx_yo2ov_dt_b_fed_asin_title

Monk Makes prototype PCD board.
https://monkmakes.com/pico_proto

FFC FPC Connector Board 8 Pins 0.5mm Socket to 2.54mm Double Row Male Pin Header Strip Adapter
https://www.amazon.com/dp/B0D79F5MWJ?ref_=ppx_hzsearch_conn_dt_b_fed_asin_title_2&th=1

FPC FFC PCB Converter Board 14P 0.5mm on Socket Side, Back 1.0mm, to DIP 2.54mm
https://www.amazon.com/dp/B0CLH5FWKN?ref=ppx_yo2ov_dt_b_fed_asin_title

Micro USB 2.0 5Pin Male to Female M to F Extension Connector
https://www.amazon.com/dp/B0CSJW27KX?ref_=ppx_hzsearch_conn_dt_b_fed_asin_title_1&th=1

I also used brass heat set insets and various screws including m2 and m3.
