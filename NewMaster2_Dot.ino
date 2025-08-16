/***************************************************
Jeff Annin - jeff.annin@gmail.com
Parts of this shamelessly borrowed from other various examples and projects found online.

Used to create a desktop thing that has a clock and displays pictures.
My own version of a custom made digital picture frame with a customized
cloock face.  Future plans might include replacing the clock with a wireless
phone charger and using a larger screen for the slideshow.  I might also
get the touchscreen part working and use it to set the time/date and
possibly brightness and other options.

This uses TFT-eSPI and Adafruit_GFX which might be inefficient and redundant but it worked.
I did this mainly because TFT_eSPI did a good job with the clock using bitmaps and sprites, but
didn't support multiple screens with different drivers.  I also had a workign slideshow project
that used Adafruit_GFX so I just combined them.  It would have been nice to use just one of them.

Built using a Raspberry pi pico because I had them laying around when I got started
working with microcontrollers.  Uses setup/setup1 and loop/loop1 to use both processors
to run each part of the project(clock/slideshow) separately.

I built this project using a breakout/dev board for the pico and 3d printed a housing to hold it.
I did this because I have commitment issues and like the idea that I could just pop the pico out and
use it for somethign else or replace it with a Pico 2 or Pico W in the future without needing
to resolder or change too much.  It also allows me to do the same with the other components.

Hardware used:
ILI9341 2.8 inch tft SPI display with built in micro sd card reader and i2c cap touch
GC9A01A 1.28 inch round tft SPI display
DS3231 RTC clock
Raspberry Pi Pico RP2040 standard setup
A breakout/dev board for the pico that has a socket for the pico and power and GPIO pins
An external 9v power supply for the breakout/dev board.
Bambu A1 mini 3d printer and various brass heat inserts, screws, and standoffs.

Full details can be found here: http://someurl.com
****************************************************/

//Core libraries
#include <SPI.h>                  //Core SPI library
#include <Adafruit_GFX.h>         //Adafruit GFX graphics library
#include <Adafruit_ILI9341.h>     //ILI9341 adafruit Hardware-specific library for slideshow
#include <SD.h>                   //SD card/filesystem library
#include <TFT_eSPI.h>             //TFT_eSPI graphics library for clock sprites
#include <RTClib.h>               //RTC Library
#include <Wire.h>                 //i2c support for RTC/touch module
#include <Adafruit_FT6206.h>      //Adafruit Cap Touch Library

// ili9341 pins for adafruit gfx
#define TFT1_CS 17	//PICO SPI0 CSn
#define TFT1_MISO 16	//PICO SPI0 RX
#define TFT1_MOSI 19	//PICO SPI0 TX
#define TFT1_CLK 18	//PICO SPI0 SCK
#define TFT1_RST 21	//PICO GP1
#define TFT1_DC 20	//PICO GP0
#define SD_CS 22   //GPIO 22

//cap touch pins - using same i2c as rtc
#define CTP_INT 2
#define CTP_SDA 0
#define CTP_RST //to 3.3v
#define CTP_SCL 1

//ili9341 display setup using adafruit gfx
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT1_CS, TFT1_DC, TFT1_RST);

//GC9A01A display setup - pins can be found in user_setup.h under the TFT_eSPI library used for the clock
TFT_eSPI tft2 = TFT_eSPI();

//RTC setup
RTC_DS3231 rtc;

//Cap touch on ili9341 setup
Adafruit_FT6206 touch = Adafruit_FT6206();

//Slideshow stuff
char *Files[1000];
int Filecounter = 0;
int slideshowinterval = 60000;   //ms between images in slideshow
#define BUFFPIXEL 20

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.
uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

//Loading .h bitmaps for clock face and hands
#include "ClockFace1.h"   // File with ClockFace1.h pixel map - BT image
#include "ClockFace2.h"   // File with ClockFace2.h pixel map - Huskers simple red N
#include "ClockFace3.h"   // File with ClockFace3.h pixel map - Huskers Black with red N white numbers
#include "ClockFace4.h"   // File with ClockFace4.h pixel map - Light blue with roman numerals
#include "ClockFace5.h"   // File with ClockFace5.h pixel map - Blue big numbers
#include "ClockFace6.h"   // File with ClockFace5.h pixel map - Huskers Blackshirts
#include "ClockFace7.h"   // File with ClockFace5.h pixel map - Neon 
#include "hourHand.h"     // File with hour hand pixel map - Black
#include "minuteHand.h"   // File with minute hand pixel map - Black
#include "secondHand.h"   // File with seconds hand pixel map
#include "minuteHandW.h"  // File with minutes hand pixel map - White
#include "hourHandW.h"    // File with hour hand pixel map - White

//clock related definitions
int centerX = tft2.width() / 2;  // Adjust for your display size
int centerY = tft2.height() / 2;  // Adjust for your display size
int radius = tft2.width() / 2; // Adjust for your display size
int sDeg,mDeg,hDeg;
unsigned long ms = millis();
unsigned long mms = millis();
byte start = 1;         // initial start flag
byte display_sHand = 1; // display seconds hand 0=no, 1=yes
volatile int hh = 0;    // hours variable
volatile int mm = 0;    // minutes variable
volatile int ss = 0;    // seconds variable
volatile int MM = 00;    // month variable
volatile int DD = 00;    // day variable
volatile int YYYY = 0000;  // year variable
int ClockFaceChoice = 1;
bool initial = 1;
char buffer[20];
char d[32];
char t[32];

#define SIXTIETH 0.016666667
#define TWELFTH 0.08333333
#define SIXTIETH_RADIAN 0.10471976
#define TWELFTH_RADIAN 0.52359878
#define RIGHT_ANGLE_RADIAN 1.5707963
#define DEG2RAD 0.0174532925

//colors
#define buttonColor ILI9341_BLUE
#define MARK_COLOR 0x0000    //Clock Marks
#define SUBMARK_COLOR 0xF800 //Clock Marks

int mcounter = 0;

TFT_eSprite clock_Face = TFT_eSprite(&tft2);   // Declare Sprite object "spr" with pointer to "tft" object
TFT_eSprite second_Hand = TFT_eSprite(&tft2);  // Declare Sprite object "spr" with pointer to "tft" object
TFT_eSprite minute_Hand = TFT_eSprite(&tft2);  // Declare Sprite object "spr" with pointer to "tft" object
TFT_eSprite hour_Hand = TFT_eSprite(&tft2);    // Declare Sprite object "spr" with pointer to "tft" object

// global x y for cap touch
int BtnX, BtnY;

// other menu related items
volatile bool setupStatus = true;
volatile bool setupMenu = false;
volatile bool setupFace = false;
volatile bool DateOn = true;
int scounter = 0;
bool inRange(int val, int minimum, int maximum) {
  return ((minimum <= val) && (val <= maximum));
}

void setup() {
  //Slideshow/Touch related setup
  //Start serial interface
  Serial.begin(115200);
  delay(500);
  Serial.println("Section setup() - Section Begin");

  //init RTC
  Wire.setSDA (CTP_SDA);
  Wire.setSCL (CTP_SCL);
  if (! rtc.begin()) {
    Serial.println("Section setup() - RTC not found");
    Serial.flush();
    while (1) delay(10);
  }
  Serial.println("Section setup() - RTC init succeeded!");
  
  //init ili9341 adafruit gfx display
  tft.begin();           //Initialize screen
  tft.invertDisplay(1);  //invert colors for ili9341
  tft.setRotation(1);    //set ili9341 landscape
  tft.fillScreen(ILI9341_BLACK);    //fill screen with black
  tft.setTextColor(ILI9341_WHITE);  //set text color to white
  tft.setCursor( 10, 20 );          //set place to start printing text
  tft.print("ILI9341 tft screen init succeeded.");   //print status message on ili9341 display
  Serial.println(F("Section setup() - ILI9341 tft screen init succeeded."));  //print status to serial
  delay(1000);    //wait 1 second to display status

    //init sd card
  Serial.println(F("Section setup() - Initializing filesystem..."));
  if(!SD.begin(SD_CS, SD_SCK_MHZ(20))) {
    Serial.println(F("Section setup() - SD begin() failed"));
    tft.setCursor( 10, 30 );
    tft.print("SD card init failed");   //display test message to ili9341 display
    delay(1000);  //wait one minute to display error
    for(;;); // Fatal error, do not continue
  }
  File root = SD.open("/");  // open SD card main root
  //delay(100);
  //printDirectory(root, 0);   //print directory to serial
  tft.setCursor(10, 30);
  tft.print("SD card init succeeded");   //display test message to ili9341 display
  Serial.println("Section setup() - SD init succeeded!");
  delay(1000);    //wait 1 second to display status  

  //init touch
  //Wire.setSDA (CTP_SDA);
  //Wire.setSCL (CTP_SCL);
  if (! touch.begin(50, &Wire)) {  // pass in 'sensitivity' coefficient and I2C bus
   Serial.println("Section setup() - FT6X36 init failed");  //debug info
   while (1) delay(10);
  }
  tft.setCursor(10, 40);
  tft.print("Touch Screen init succeeded");   
  Serial.println("Section setup() - FT6X36 Touch Screen init succeeded.");  //debug info
  delay(1000);    //wait 1 second to display status 

  //draw choice buttons
  drawChoice();
  static long onesecond = 1000;
  static long pMillis = 0;
  unsigned long countdownValue, prevMillis, currentMillis;
  unsigned long startMillis = millis();
  //how long to wait before continue?
  countdownValue = 10;
  //wait for button press
  while (setupStatus) {
    if(millis() - startMillis > onesecond) {
      //Serial.println("Section loop() - is this showing every second?");  //debug info
      startMillis = millis();  //reset startMillis value
      tft.fillRect(85, 200, 20, 30, ILI9341_BLACK);
      tft.setCursor(85,200);
      tft.print(countdownValue);
      countdownValue --;
      if (countdownValue == 0) {
        setupStatus = 0;
      }
    }
    if (touch.touched()) {
      ProcessTouch(); // get screen location actuall x-y
      if ((inRange(BtnX, 100, 130)) && (inRange(BtnY,20,140))) {  // was the setup button pressed?
        Serial.println("Section loop() - Set time button pressed!");
        //enter setup function
        setupMenu = 1;
        setupStatus = 0;
        drawSetup();
       // drawSetup();
      } else if ((inRange(BtnX, 30, 90)) && (inRange(BtnY,70,220))) {  // was the continue button pressed?
        Serial.println("Section loop() - Continue button pressed!");
        //Continue
        setupMenu = 0;
        setupStatus = 0;
      } else if ((inRange(BtnX, 100, 130)) && (inRange(BtnY,170,290))) {  // was the continue button pressed?
        Serial.println("Section loop() - Set Face button pressed!");
        //draw buttons
        drawFaceButtons();
        setupFace = 1;
        setupMenu = 0;
        setupStatus = 0;
      } 
    }
  }
  while (setupFace) {
    //process touches
    if (touch.touched()) {
      ProcessTouch(); // get screen location actuall x-y
      if ((inRange(BtnX, 20, 55)) && (inRange(BtnY, 15, 135))) {  // was the exit setup button pressed?
        Serial.println("Section loop() - Exit Setup Pressed");
        setupFace = 0;        
      } else if ((inRange(BtnX, 180, 200)) && (inRange(BtnY, 20, 120))) {  // Face 1
        Serial.println("Section loop() - Face 1 pressed");
        ClockFaceChoice = 1;
      } else if ((inRange(BtnX, 140, 170)) && (inRange(BtnY, 20, 120))) {  // Face 2
        Serial.println("Section loop() - Face 2 pressed");
        ClockFaceChoice = 2;
      } else if ((inRange(BtnX, 110, 130)) && (inRange(BtnY, 20, 120))) {  // Face 3
        Serial.println("Section loop() - Face 3 pressed");
        ClockFaceChoice = 3;
      } else if ((inRange(BtnX, 60, 90)) && (inRange(BtnY, 20, 120))) {  // Face 4
        Serial.println("Section loop() - Face 4 pressed");
        ClockFaceChoice = 4;
      } else if ((inRange(BtnX, 180, 200)) && (inRange(BtnY, 200, 240))) {  // Face 5
        Serial.println("Section loop() - Face 5 pressed");
        ClockFaceChoice = 5;
      } else if ((inRange(BtnX, 140, 170)) && (inRange(BtnY, 200, 240))) {  // Face 6
        Serial.println("Section loop() - Face 6 pressed");
        ClockFaceChoice = 6;
      } else if ((inRange(BtnX, 110, 130)) && (inRange(BtnY, 200, 240))) {  // Face 7
        Serial.println("Section loop() - Face 7 pressed");
        ClockFaceChoice = 7;
      } else if ((inRange(BtnX, 30, 60)) && (inRange(BtnY, 180, 290))) {  // was the Date ON/Off button pressed?
        Serial.println("Section loop() - Date On/Off Setup Pressed");
        if (DateOn == 1) {
          DateOn = 0;
        } else {
          DateOn = 1;
        }
      }
    }
  }

  while (setupMenu && !setupFace) {
    if(millis() - pMillis > onesecond) {
      //Serial.println("Section loop() - is this showing every second?");  /debug info
      pMillis = millis();
      //get time and load then display
      DateTime now = rtc.now();  //get current time
      tft.fillRect(15, 175, 100, 20, ILI9341_BLACK);
      tft.setCursor(15, 175);
      sprintf(t, "%02d:%02d:%d \n", now.hour(), now.minute(), now.second());  //load formatted time into "t"
      tft.print(t);
      
      tft.setCursor(165, 160);
      tft.print("Current date:");
      tft.fillRect(165, 175, 140, 20, ILI9341_BLACK);
      tft.setCursor(165, 175);
      sprintf(d, "%02d/%02d/%d \n", now.month(), now.day(), now.year());  //load formatted date info into "d"
      tft.print(d);
    }

    if (touch.touched()) {
      ProcessTouch(); // get screen location actuall x-y
      if ((inRange(BtnX, 20, 60)) && (inRange(BtnY, 90, 220))) {  // was the setup button pressed?
        Serial.println("Section loop() - Exit Setup Pressed");
        setupMenu = 0;        
      } else if ((inRange(BtnX, 180, 210)) && (inRange(BtnY, 70, 100))) {  // was the H+ button pressed?
        Serial.println("Section loop() - Hour + pressed!");
        DateTime now = rtc.now();  //get current time
        DateTime newTime = now + TimeSpan(0,1,0,0);  //add 1 hour
        rtc.adjust(newTime);
      } else if ((inRange(BtnX, 180, 210)) && (inRange(BtnY, 120, 140))) {  // was the H- button pressed?
        Serial.println("Section loop() - Hour - pressed!");
        DateTime now = rtc.now();  //get current time
        DateTime newTime = now - TimeSpan(0,1,0,0);  //subtract 1 hour
        rtc.adjust(newTime);
      } else if ((inRange(BtnX, 130, 160)) && (inRange(BtnY, 70, 100))) {  // was the M+ button pressed?
        Serial.println("Section loop() - Minute + pressed!");
        DateTime now = rtc.now();  //get current time
        DateTime newTime = now + TimeSpan(0,0,1,0);  //add 1 minute
        rtc.adjust(newTime);
      } else if ((inRange(BtnX, 130, 160)) && (inRange(BtnY, 120, 140))) {  // was the M- button pressed?
        Serial.println("Section loop() - Minute - pressed!");
        DateTime now = rtc.now();  //get current time
        DateTime newTime = now - TimeSpan(0,0,1,0);  //subtract 1 minute
        rtc.adjust(newTime);
      } else if ((inRange(BtnX, 90, 120)) && (inRange(BtnY, 70, 100))) {  // was the S+ button pressed?
        Serial.println("Section loop() - Second + pressed!");
        DateTime now = rtc.now();  //get current time
        DateTime newTime = now + TimeSpan(0,0,0,1);  //add 1 second
        rtc.adjust(newTime);
      } else if ((inRange(BtnX, 90, 120)) && (inRange(BtnY, 120, 140))) {  // was the S- button pressed?
        Serial.println("Section loop() - Second - pressed!");
        DateTime now = rtc.now();  //get current time
        DateTime newTime = now - TimeSpan(0,0,0,1);  //subtract 1 second
        rtc.adjust(newTime);
      } else if ((inRange(BtnX, 180, 210)) && (inRange(BtnY, 240, 260))) {  // was the D+ button pressed?
        Serial.println("Section loop() - Day + pressed!");
        DateTime now = rtc.now();  //get current time
        DateTime newTime = now + TimeSpan(1,0,0,0);  //add 1 day
        rtc.adjust(newTime);
      } else if ((inRange(BtnX, 180, 210)) && (inRange(BtnY, 270, 290))) {  // was the D- button pressed?
        Serial.println("Section loop() - Day - pressed!");
        DateTime now = rtc.now();  //get current time
        DateTime newTime = now - TimeSpan(1,0,0,0);  //subtract 1 day
        rtc.adjust(newTime);
      } else if ((inRange(BtnX, 130, 160)) && (inRange(BtnY, 240, 260))) {  // was the Mon+ button pressed?
        Serial.println("Section loop() - Month + pressed!");
        DateTime now = rtc.now();  //get current time
        int currentMonth = now.month();
        if (currentMonth < 12) {
          currentMonth++;
        }
        DateTime newTime = DateTime(now.year(), currentMonth, now.day(), now.hour(), now.hour(), now.second());
        rtc.adjust(newTime);
      } else if ((inRange(BtnX, 130, 160)) && (inRange(BtnY, 270, 290))) {  // was the Mon- button pressed?
        Serial.println("Section loop() - Month - pressed!");
        DateTime now = rtc.now();  //get current time
        int currentMonth = now.month();
        if (currentMonth > 1){
          currentMonth--;
        }
        DateTime newTime = DateTime(now.year(), currentMonth, now.day(), now.hour(), now.hour(), now.second());
        rtc.adjust(newTime);
      } else if ((inRange(BtnX, 90, 210)) && (inRange(BtnY, 240, 260))) {  // was the Y+ button pressed?
        Serial.println("Section loop() - Year + pressed!");
        DateTime now = rtc.now();  //get current time
        DateTime newTime = now + TimeSpan(365,0,0,0);  //add 1 year
        rtc.adjust(newTime);
      } else if ((inRange(BtnX, 90, 210)) && (inRange(BtnY, 270, 290))) {  // was the Y- button pressed?
        Serial.println("Section loop() - Year - pressed!");
        DateTime now = rtc.now();  //get current time
        DateTime newTime = now - TimeSpan(365,0,0,0);  //subtract 1 year
        rtc.adjust(newTime);
      }
    }
  }
  Serial.println("Section setup() - Section End");  //debug info
}

void setup1() {
  //Analog Clock Related Setup
  //Start the serial interface
  Serial.begin(115200);
  delay(500);
  Serial.println("Section setup1() - Section Start");  //debug info

  // init of GC9A01A display for clock
  tft2.init();                     // initialise the display unit
  tft2.setRotation(0);             // 0 = 0 degress, 2 = 180 degrees
  tft2.invertDisplay (1);          // invert colors
  tft2.fillScreen(TFT_BLACK);      // fill screen with Black
  tft2.setCursor( 30, 50 );
  tft2.print("GC9A01A init succeeded.");   //display test message to ili9341 display
  Serial.println(F("Section setup1() - GC9A01A tft screen init succeeded."));  //debug info
  delay(1000);

  //Get date from RTC to test
  DateTime now = rtc.now();
  sprintf(d, "%02d/%02d/%d \n", now.month(), now.day(), now.year());  //load formatted date info into "d"
  Serial.println("Date is:");
  Serial.println(d);

  hh = now.hour();
  mm = now.minute();
  ss = now.second();

  //Get time from RTC to test
  sprintf(t, "%02d:%02d:%d \n", hh, mm, ss);  //load formatted time into "t"
  Serial.println("Time is:");
  Serial.println(t);

// swap byte order when rendering to correct different image endianness
  second_Hand.setSwapBytes(false);
  minute_Hand.setSwapBytes(true);
  hour_Hand.setSwapBytes(true);
  
// Create images for the Second, Minute and Hour hands
  createSecondHand();
  createMinuteHand();
  createHourHand();
  Serial.println("Section setup1() - Section End");  //debug info
}   

void loop() {
  //Setup Menu
  //Populate time from RTC to clock variables
  Serial.println("Section loop() - Section start");  //debug info

  //Slideshow loop
  Serial.println("Section loop() - Start slideshow");
  File root = SD.open("/");  // open SD card main root
  
  //Display a loop of bmp files from the root fo the SD card
  while (true) {
    tft.fillScreen(ILI9341_BLACK);
    File entry =  root.openNextFile();

    if (! entry) {
      // no more files - begin loop again
      root.close();
      return;
    }
    String filename = String(entry.name());  // get file name
    Serial.println("Next File is:");
    Serial.println(filename);
    uint8_t nameSize = String(entry.name()).length();  // get file name size
    String str1 = String(entry.name()).substring( nameSize - 4 );  // save the last 4 characters (file extension)

    if (str1.equalsIgnoreCase(".bmp")) {  // if the file has '.bmp' extension
      bmpDraw(entry.name(), 0, 0);  //  draw BMP
      entry.close();  // close the file
      unsigned long showTime = millis();
      bool advance = false;
    //Do we advance to the next image? Touch or interval?
      while (!advance) {
        if (touch.touched()) {
          ProcessTouch();
          advance = true;
        }
        if (millis() - showTime >= slideshowinterval) {
        advance = true;
        }
      //delay(slideshowinterval);   // delay and then go on to the next bmp file in loop
      delay(50);
      }

    } else {
      Serial.println("Not .bmp file, skipping");    //if not a bmp file close and move on
      entry.close();
    }
  }
  Serial.println("Section loop() - Section end");
}

void loop1() {
  manage_Display();           // manage clock display
}

void manage_Display() {
// Only update the screen once every second and at the start
  if(millis()-ms>=1000 || start) {
    
    //add to counter until 60 and update the date
    mcounter++;
    if (mcounter==60) {
      Serial.println("A minute has passed! Updating Date");
      DateTime now = rtc.now();
      sprintf(d, "%02d/%02d/%d \n", now.month(), now.day(), now.year());
      Serial.println(d);
      hh = now.hour();
      mm = now.minute();
      ss = now.second();
      mcounter =0;
    }

    //main clock workings
    ms = millis();
    if(++ss>59) {
      ss=0;
      if(++mm>59) {
        mm=0;
        if(++hh>23) hh=0;
      }
    }
    
    // Set the angles for the Second, Minute and Hour hands
    sDeg = ss*6;
    if(ss==0 || start) {
      start = 0;
      if(ss==0) mms = millis();
      mDeg = mm*6+sDeg/60;
      hDeg = hh*30+mDeg/12;
      }
    clockUpdate(sDeg,mDeg,hDeg); // call routine to push the sprites for the clock face and clock hands
  }
}

// Create Seconds hand sprite and push the image of the Seconds hand to that sprite
void createSecondHand() {
  second_Hand.setColorDepth(8); // set for 8 bits per pixel
  second_Hand.createSprite(10,136);
  second_Hand.pushImage(0,0,10,136,secondHand);
  second_Hand.setPivot(5,102);
}

// Create Minutes hand sprite and push the image of the Minutes hand to that sprite
void createMinuteHand() {
  minute_Hand.setColorDepth(8);
  minute_Hand.createSprite(7,100);
  minute_Hand.pushImage(0,0,7,100,minuteHandW);
  minute_Hand.setPivot(4,79);
}

// Create Hour hand sprite and push the image of the Hour hand to that sprite
void createHourHand() {
  hour_Hand.setColorDepth(8);
  hour_Hand.createSprite(9,74);
  hour_Hand.pushImage(0,0,9,74,hourHandW);
  hour_Hand.setPivot(5,51);
}

// Create the background sprite i.e. the clock face and push the image of the clock face to that sprite and display date
void createBackground() {
  clock_Face.setColorDepth(16);
  clock_Face.createSprite(240,240);
  clock_Face.setPivot(120,120);
  tft2.setPivot(120,120);
  clock_Face.setTextColor(TFT_WHITE);
  clock_Face.fillSprite(TFT_TRANSPARENT);
  //select correct clock face
  if (ClockFaceChoice == 1) {
    clock_Face.pushImage(0,0,240,240,ClockFace1);
  } else if (ClockFaceChoice == 2) {
    clock_Face.pushImage(0,0,240,240,ClockFace2);
  } else if (ClockFaceChoice == 3) {
    clock_Face.pushImage(0,0,240,240,ClockFace3);
  } else if (ClockFaceChoice == 4) {
    clock_Face.pushImage(0,0,240,240,ClockFace4);
  } else if (ClockFaceChoice == 5) {
    clock_Face.pushImage(0,0,240,240,ClockFace5);
  } else if (ClockFaceChoice == 6) {
    clock_Face.pushImage(0,0,240,240,ClockFace6);
  } else if (ClockFaceChoice == 7) {
    clock_Face.pushImage(0,0,240,240,ClockFace7);
  }
  if (DateOn == 1) {
    clock_Face.setTextSize(1);
    clock_Face.setTextColor(TFT_WHITE);
    clock_Face.fillRect(78, 200, 80, 16, ILI9341_BLACK);
    clock_Face.drawString(d, 80, 200, 2);
  }
  //this section can be used to draw marks or numbers if needed
  //Draw 60 clock marks
  //first two numbers are the ring radius large hour marks(12.3.6.9), second two are hours,  thrid two are minutes/seconds
  //draw_round_clock_mark(104, 120, 112, 120, 114, 114);
}

// On each update, re-create the clock face, and push the sprites of the Seconds, Minutes and Hour hands to their new positions on the display
void clockUpdate(int16_t angle_secondHand, int16_t angle_minuteHand, int16_t angle_hourHand) {
  createBackground();
  if (display_sHand) second_Hand.pushRotated(&clock_Face, angle_secondHand, TFT_TRANSPARENT); // Check if Seconds hand is to be displayed
  minute_Hand.pushRotated(&clock_Face, angle_minuteHand, TFT_TRANSPARENT);
  hour_Hand.pushRotated(&clock_Face, angle_hourHand, TFT_TRANSPARENT);
  clock_Face.pushSprite(0,0,TFT_TRANSPARENT);
}

void bmpDraw(const char *filename, uint8_t x, uint16_t y) {
  // function to draw the bmp files - could add this to another library to save space here?
  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3 * BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  //check image size is within the screen size
  if ((x >= tft.width()) || (y >= tft.height())) return;

  //print status to serial interface
  Serial.println();
  Serial.print(F("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print(F("File not found"));
    return;
  }

  // Parse BMP header
  if (read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if (read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if (bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if ((x + w - 1) >= tft.width())  w = tft.width()  - x;
        if ((y + h - 1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.startWrite();
        tft.setAddrWindow(x, y, w, h);

        for (row = 0; row < h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if (flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if (bmpFile.position() != pos) { // Need seek?
            tft.endWrite();
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col = 0; col < w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
              tft.startWrite();
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft.pushColor(tft.color565(r, g, b));
          } // end pixel
        } // end scanline
        tft.endWrite();
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if (!goodBmp) Serial.println(F("BMP format not recognized."));
}

//Print contents of SD card to serial interface as a test.  Could be moved to another library or omitted altogether?
void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void ProcessTouch() {
  Serial.println("Touch Detected");
  TS_Point p = touch.getPoint();
  
  //Touch Mapping
  //BtnX = map(p.y, 320, 0, 320, 0);
  //BtnY = map(p.x, 0, 240, 240, 15);

  BtnX = p.x;
  BtnY = p.y;

  //Debounce
  delay (250);

  //testing touch
  Serial.print(BtnX);
  Serial.print(",");
  Serial.println(BtnY);
  //tft.fillCircle(BtnX, BtnY, 3, ILI9341_RED);
}

void drawChoice() {
  //Draw buttons and text for choice screen
  //Draw Setup Button  
  tft.setTextSize(2);
  tft.fillRect(10, 100, 140, 30, buttonColor);
  tft.setCursor(35, 110);
  tft.print("Set Time");

  //draw continue button
  tft.setTextSize(2);
  tft.fillRect(170, 100, 140, 30, buttonColor);
  tft.setCursor(190, 110);
  tft.print("Set Face");
  
  //draw continue button
  tft.setTextSize(2);
  tft.fillRect(90, 150, 140, 30, buttonColor);
  tft.setCursor(105, 155);
  tft.print("Continue");

  //draw message
  tft.setTextSize(1);
  tft.setCursor(10, 60);
  tft.print("Please make a selection.");
  tft.setCursor(10, 200);
  tft.print("Continue in: ");
}

void drawFaceButtons() {
  //Draw buttons and text for set face
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);

  tft.fillRect(20, 20, 100, 30, buttonColor);
  tft.setCursor(25, 25);
  tft.print("Face 1");

  tft.fillRect(20, 60, 100, 30, buttonColor);
  tft.setCursor(25, 65);
  tft.print("Face 2");
  
  tft.fillRect(20, 100, 100, 30, buttonColor);
  tft.setCursor(25, 105);
  tft.print("Face 3");

  tft.fillRect(20, 140, 100, 30, buttonColor);
  tft.setCursor(25, 145);
  tft.print("Face 4");
  
  tft.fillRect(180, 20, 100, 30, buttonColor);
  tft.setCursor(185, 25);
  tft.print("Face 5");

  tft.fillRect(180, 60, 100, 30, buttonColor);
  tft.setCursor(185, 65);
  tft.print("Face 6");
  
  tft.fillRect(180, 100, 100, 30, buttonColor);
  tft.setCursor(185, 105);
  tft.print("Face 7");

  tft.fillRect(15, 200, 135, 30, buttonColor);
  tft.setCursor(20, 205);
  tft.print("Exit Setup");

  tft.fillRect(165, 200, 140, 30, buttonColor);
  tft.setCursor(170, 205);
  tft.print("Date on/off");
}

void drawSetup() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);
  
  tft.fillRect(80, 195, 145, 35, buttonColor);
  tft.setCursor(95, 205);
  tft.print("Exit Setup");

  tft.setTextSize(2);
  tft.setCursor(15, 25);
  tft.print("Hour");
    
  tft.setTextSize(3);
  tft.fillRect(80, 15, 35, 35, buttonColor);
  tft.setCursor(88, 20);
  tft.print("+");

  tft.fillRect(115, 15, 35, 35, buttonColor);
  tft.setCursor(123, 20);
  tft.print("-");

  tft.setTextSize(2);
  tft.setCursor(15, 70);
  tft.print("Min");

  tft.setTextSize(3);
  tft.fillRect(80, 65, 35, 35, buttonColor);
  tft.setCursor(88, 70);
  tft.print("+");

  tft.fillRect(115, 65, 35, 35, buttonColor);
  tft.setCursor(123, 70);
  tft.print("-");
    
  tft.setTextSize(2);
  tft.setCursor(15, 115);
  tft.print("Sec");

  tft.setTextSize(3);
  tft.fillRect(80, 110, 35, 35, buttonColor);
  tft.setCursor(88, 115);
  tft.print("+");

  tft.fillRect(115, 110, 35, 35, buttonColor);
  tft.setCursor(123, 115);
  tft.print("-");

  tft.setTextSize(2);
  tft.setCursor(165, 25);
  tft.print("Day");

  tft.setTextSize(3);
  tft.fillRect(230, 15, 35, 35, buttonColor);
  tft.setCursor(238, 20);
  tft.print("+");

  tft.fillRect(265, 15, 35, 35, buttonColor);
  tft.setCursor(273, 20);
  tft.print("-");

  tft.setTextSize(2);
  tft.setCursor(165, 70);
  tft.print("Month");

  tft.setTextSize(3);
  tft.fillRect(230, 65, 35, 35, buttonColor);
  tft.setCursor(238, 70);
  tft.print("+");

  tft.fillRect(265, 65, 35, 35, buttonColor);
  tft.setCursor(273, 70);
  tft.print("-");

  tft.setTextSize(2);
  tft.setCursor(165, 115);
  tft.print("Year");    

  tft.setTextSize(3);
  tft.fillRect(230, 110, 35, 35, buttonColor);
  tft.setCursor(238, 115);
  tft.print("+");

  tft.fillRect(265, 110, 35, 35, buttonColor);
  tft.setCursor(273, 115);
  tft.print("-");

  tft.setTextSize(1.5);
  tft.setCursor(15, 160);
  tft.print("Current time: ");
  tft.setCursor(165, 160);
  tft.print("Current date:");
}

void draw_round_clock_mark(uint8_t innerR1, uint8_t outerR1, uint8_t innerR2, uint8_t outerR2, uint8_t innerR3, uint8_t outerR3) {
  float xm, ym;
  uint8_t xm0, xm1, ym0, ym1, innerR, outerR;
  uint16_t c;
  static float sdeg, mdeg, hdeg;
  int CENTER = 120;
  int wm;  //thick line width

  for (int i = 0; i < 60; i++)
  {
    if ((i % 15) == 0)
    {
      innerR = innerR1;
      outerR = outerR1;
      c = MARK_COLOR;
      wm = 5;
    }
    else if ((i % 5) == 0)
    {
      innerR = innerR2;
      outerR = outerR2;
      c = MARK_COLOR;
      wm = 2;
    }
    else
    {
      innerR = innerR3;
      outerR = outerR3;
      c = SUBMARK_COLOR;
      wm = 1;
    }

    mdeg = (SIXTIETH_RADIAN * i) - RIGHT_ANGLE_RADIAN;
    xm = cos(mdeg);
    ym = sin(mdeg);
    xm0 = xm * outerR + CENTER;
    ym0 = ym * outerR + CENTER;
    xm1 = xm * innerR + CENTER;
    ym1 = ym * innerR + CENTER;

    //clock_Face.drawLine(xm0, ym0, xm1, ym1, c);
    clock_Face.drawWideLine(xm0, ym0, xm1, ym1, wm, c, c);
  }
}