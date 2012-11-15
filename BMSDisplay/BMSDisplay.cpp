// BMSDisplay.cpp
//  Created by corbin dunn on 5/18/12

#include  "Arduino.h" // — for Arduino 1.0

//#include <SD.h>        // Library from Adafruit.com 
//#include <SdFatUtil.h>
//#include <SoftwareSerial.h>
#include <Canbus.h>

#include <Wire.h>

//#include <EEPROM.h>
//#include <avr/eeprom.h>

//#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t
//#include <Time.h>
//#include <TimeAlarms.h> // NOTE: not using; I could remove this

#include <stdint.h> // We can compile without this, but it kills xcode completion without it! it took me a while to discover that..

//#include <SPI.h>
#include <UTFT.h>


// Requires: display of at least 220x176 pixels.
#define SCREEN_WIDTH 220
#define SCREEN_HEIGHT 176


// Display setup:

// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];
/*
#define DEBUG 1

// Define Joystick connection
#define UP     A1
#define RIGHT  A2
#define DOWN   A3
#define CLICK  A4
#define LEFT   A5

int CAN_BUS_LED2 = 8;
int CAN_BUS_LED3 = 7;

CanbusClass Canbus;

static inline void setupCanbus() {
    pinMode(CAN_BUS_LED2, OUTPUT);
    pinMode(CAN_BUS_LED3, OUTPUT);
    
    digitalWrite(CAN_BUS_LED2, LOW);
    pinMode(UP,INPUT);
    pinMode(DOWN,INPUT);
    pinMode(LEFT,INPUT);
    pinMode(RIGHT,INPUT);
    pinMode(CLICK,INPUT);
    
    digitalWrite(UP, HIGH);       // Enable internal pull-ups
    digitalWrite(DOWN, HIGH);
    digitalWrite(LEFT, HIGH);
    digitalWrite(RIGHT, HIGH);
    digitalWrite(CLICK, HIGH);
}

UTFT myGLCD(ADAFRUIT_2_2_TFT, 11, 13, 3, 9);   // Remember to change the model parameter to suit your display module!

float g_stateOfCharge = 0;

static inline void setupLCD() {
    //randomSeed(analogRead(0));
    myGLCD.InitLCD();
    myGLCD.clrScr();
}

void updateSOCDisplay() {
    myGLCD.setBackColor(0, 0, 0); // black
    myGLCD.setColor(255,255,255); // white
    myGLCD.setFont(SevenSegNumFont);
    myGLCD.print("100", CENTER, 50);
}

static inline void printTitle(char *title) {
    myGLCD.setBackColor(64, 64, 64);
    myGLCD.setColor(64, 64, 64);
    myGLCD.fillRect(0, 0, SCREEN_WIDTH - 1, 13);
    
    myGLCD.setColor(255, 255, 255);
    myGLCD.setFont(SmallFont);
    myGLCD.print(title, CENTER, 1);
}


// enums take 2 bytes, which sucks, so I typedef it to a uint8_t
enum _StatusKind {
    StatusKindNormal = 0,
    StatusKindWarning = 1,
    StatusKindError = 2,
};
typedef uint8_t StatusKind;

void printStatus(char *statusMessage, StatusKind statusKind) {
#if DEBUG
    Serial.println(statusMessage);
#endif
    switch (statusKind) {
        case StatusKindNormal: {
            myGLCD.setBackColor(64, 64, 64);
            myGLCD.setColor(64, 64, 64);
            break;
        }
        case StatusKindWarning: {
#warning TODO: appropriate color
            myGLCD.setBackColor(64, 64, 64);
            myGLCD.setColor(64, 64, 64);
            break;
        }
        default:
        case StatusKindError: {
            myGLCD.setBackColor(255, 0, 0); // red background
            myGLCD.setColor(255, 0, 0);
            break;
        }
    }
    //Fill a square at the top
    myGLCD.fillRect(0, 162, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
 
    myGLCD.setColor(255, 255, 255); // white text
    myGLCD.setFont(SmallFont);
    myGLCD.print(statusMessage, CENTER, 163);
}
 
void setup() {
    Serial.begin(9600);
#if DEBUG
    Serial.println("initialized");
#endif
    
    Serial.println("Doing setup");
    setupLCD();
    
    Serial.println("LCD setup");
    printTitle("** BMS Display for Elithion **");

    setupCanbus();
//    updateSOCDisplay();

    if (Canbus.init(CANSPEED_500)) {
        printStatus("Can bus initialized", StatusKindNormal);
    } else {
        printStatus("FAILED CAN BUS INIT", StatusKindError);
        delay(1000); // Show it for 5 seconds...
    }
    

    updateSOCDisplay();
}

char buffer[512];  //Data will be temporarily stored to this buffer before being written to the file

void loop() {
#if DEBUG
//    Serial.println("loop");
#endif
  if(Canbus.ecu_req(ENGINE_RPM,buffer) == 1)          // Request for engine RPM 
  {
//    sLCD.write(COMMAND);                   // Move LCD cursor to line 0 
//    sLCD.write(LINE0);
    Serial.println(buffer);                         // Display data on LCD
   
    
  } 
  digitalWrite(CAN_BUS_LED3, HIGH);
   
  if(Canbus.ecu_req(VEHICLE_SPEED,buffer) == 1)
  {
//    sLCD.write(COMMAND);
//    sLCD.write(LINE0 + 9);
    Serial.println(buffer);
   
  }
  
  if(Canbus.ecu_req(ENGINE_COOLANT_TEMP,buffer) == 1)
  {
//    sLCD.write(COMMAND);
//    sLCD.write(LINE1);                     // Move LCD cursor to line 1 
    Serial.println(buffer);
   
   
  }
  
  if(Canbus.ecu_req(THROTTLE,buffer) == 1)
  {
//    sLCD.write(COMMAND);
//    sLCD.write(LINE1 + 9);
    Serial.println(buffer);
//     file.print(buffer);
  }
//  Canbus.ecu_req(O2_VOLTAGE,buffer);
     
   
   digitalWrite(CAN_BUS_LED3, LOW);
   delay(100); 
   
   

}


*/


// Declare which fonts we will be using
extern uint8_t SmallFont[];

// Uncomment the next line for Arduino 2009/Uno
//UTFT myGLCD(ITDB22,19,18,17,16);   // Remember to change the model parameter to suit your display module!

UTFT myGLCD(ADAFRUIT_2_2_TFT, 11, 13, 3, 9);   // Remember to change the model parameter to suit your display module!

//UTFT myGLCD(HX8340B_S, 11, 13, 3, 9);   // Remember to change the model parameter to suit your display module!


// Uncomment the next line for Arduino Mega
//UTFT myGLCD(ITDB22,38,39,40,41);   // Remember to change the model parameter to suit your display module!

void setup()
{
    randomSeed(analogRead(0));
    
    // Setup the LCD
    myGLCD.InitLCD();
    myGLCD.setFont(SmallFont);
}

void loop()
{
    int buf[218];
    int x, x2;
    int y, y2;
    int r;
    
    // Clear the screen and draw the frame
    myGLCD.clrScr();
    
    myGLCD.setColor(255, 0, 0);
    myGLCD.fillRect(0, 0, 219, 13);
    myGLCD.setColor(64, 64, 64);
    myGLCD.fillRect(0, 162, 219, 175);
    myGLCD.setColor(255, 255, 255);
    
    myGLCD.setBackColor(255, 0, 0);
    myGLCD.print("** Universal TFT Library **", CENTER, 1);
    myGLCD.setBackColor(64, 64, 64);
    myGLCD.setColor(255,255,0);
    myGLCD.print("> elec.henningkarlsen.com <", CENTER, 163);
    
    myGLCD.setColor(0, 0, 255);
    myGLCD.drawRect(0, 14, 219, 161);
    
    // Draw crosshairs
    myGLCD.setColor(0, 0, 255);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.drawLine(109, 15, 109, 160);
    myGLCD.drawLine(1, 88, 218, 88);
    
    for (int i=9; i<210; i+=10)
        myGLCD.drawLine(i, 86, i, 90);
    for (int i=19; i<155; i+=10)
        myGLCD.drawLine(107, i, 111, i);
    
    // Draw sin-, cos- and tan-lines
    myGLCD.setColor(0,255,255);
    myGLCD.print("Sin", 5, 15);
    for (int i=1; i<218; i++)
    {
        myGLCD.drawPixel(i,88+(sin(((i*1.65)*3.14)/180)*70));
    }
    
    myGLCD.setColor(255,0,0);
    myGLCD.print("Cos", 5, 27);
    for (int i=1; i<218; i++)
    {
        myGLCD.drawPixel(i,88+(cos(((i*1.65)*3.14)/180)*70));
    }
    
    myGLCD.setColor(255,255,0);
    myGLCD.print("Tan", 5, 39);
    for (int i=1; i<218; i++)
    {
        myGLCD.drawPixel(i,88+(tan(((i*1.65)*3.14)/180)));
    }
    
    delay(2000);
    
    myGLCD.setColor(0,0,0);
    myGLCD.fillRect(1,15,218,160);
    myGLCD.setColor(0, 0, 255);
    myGLCD.setBackColor(0, 0, 0);
    myGLCD.drawLine(109, 15, 109, 160);
    myGLCD.drawLine(1, 88, 218, 88);
    
    // Draw a moving sinewave
    x=1;
    for (int i=1; i<(218*20); i++)
    {
        x++;
        if (x==219)
            x=1;
        if (i>219)
        {
            if ((x==109)||(buf[x-1]==88))
                myGLCD.setColor(0,0,255);
            else
                myGLCD.setColor(0,0,0);
            myGLCD.drawPixel(x,buf[x-1]);
        }
        myGLCD.setColor(0,255,255);
        y=88+(sin(((i*1.6)*3.14)/180)*(65-(i / 100)));
        myGLCD.drawPixel(x,y);
        buf[x-1]=y;
    }
    
    delay(2000);
    
    myGLCD.setColor(0,0,0);
    myGLCD.fillRect(1,15,218,160);
    
    // Draw some filled rectangles
    for (int i=1; i<6; i++)
    {
        switch (i)
        {
            case 1:
                myGLCD.setColor(255,0,255);
                break;
            case 2:
                myGLCD.setColor(255,0,0);
                break;
            case 3:
                myGLCD.setColor(0,255,0);
                break;
            case 4:
                myGLCD.setColor(0,0,255);
                break;
            case 5:
                myGLCD.setColor(255,255,0);
                break;
        }
        myGLCD.fillRect(44+(i*15), 23+(i*15), 88+(i*15), 63+(i*15));
    }
    
    delay(2000);
    
    myGLCD.setColor(0,0,0);
    myGLCD.fillRect(1,15,218,160);
    
    // Draw some filled, rounded rectangles
    for (int i=1; i<6; i++)
    {
        switch (i)
        {
            case 1:
                myGLCD.setColor(255,0,255);
                break;
            case 2:
                myGLCD.setColor(255,0,0);
                break;
            case 3:
                myGLCD.setColor(0,255,0);
                break;
            case 4:
                myGLCD.setColor(0,0,255);
                break;
            case 5:
                myGLCD.setColor(255,255,0);
                break;
        }
        myGLCD.fillRoundRect(132-(i*15), 23+(i*15), 172-(i*15), 63+(i*15));
    }
    
    delay(2000);
    
    myGLCD.setColor(0,0,0);
    myGLCD.fillRect(1,15,218,160);
    
    // Draw some filled circles
    for (int i=1; i<6; i++)
    {
        switch (i)
        {
            case 1:
                myGLCD.setColor(255,0,255);
                break;
            case 2:
                myGLCD.setColor(255,0,0);
                break;
            case 3:
                myGLCD.setColor(0,255,0);
                break;
            case 4:
                myGLCD.setColor(0,0,255);
                break;
            case 5:
                myGLCD.setColor(255,255,0);
                break;
        }
        myGLCD.fillCircle(64+(i*15),43+(i*15), 20);
    }
    
    delay(2000);
    
    myGLCD.setColor(0,0,0);
    myGLCD.fillRect(1,15,218,160);
    
    // Draw some lines in a pattern
    myGLCD.setColor (255,0,0);
    for (int i=15; i<160; i+=5)
    {
        myGLCD.drawLine(1, i, (i*1.44)-10, 160);
    }
    myGLCD.setColor (255,0,0);
    for (int i=160; i>15; i-=5)
    {
        myGLCD.drawLine(218, i, (i*1.44)-12, 15);
    }
    myGLCD.setColor (0,255,255);
    for (int i=160; i>15; i-=5)
    {
        myGLCD.drawLine(1, i, 232-(i*1.44), 15);
    }
    myGLCD.setColor (0,255,255);
    for (int i=15; i<160; i+=5)
    {
        myGLCD.drawLine(218, i, 231-(i*1.44), 160);
    }
    
    delay(2000);
    
    myGLCD.setColor(0,0,0);
    myGLCD.fillRect(1,15,218,161);
    
    // Draw some random circles
    for (int i=0; i<100; i++)
    {
        myGLCD.setColor(random(255), random(255), random(255));
        x=22+random(176);
        y=35+random(105);
        r=random(20);
        myGLCD.drawCircle(x, y, r);
    }
    
    delay(2000);
    
    myGLCD.setColor(0,0,0);
    myGLCD.fillRect(1,15,218,160);
    
    // Draw some random rectangles
    for (int i=0; i<100; i++)
    {
        myGLCD.setColor(random(255), random(255), random(255));
        x=2+random(216);
        y=16+random(143);
        x2=2+random(216);
        y2=16+random(143);
        myGLCD.drawRect(x, y, x2, y2);
    }
    
    delay(2000);
    
    myGLCD.setColor(0,0,0);
    myGLCD.fillRect(1,15,218,160);
    
    // Draw some random rounded rectangles
    for (int i=0; i<100; i++)
    {
        myGLCD.setColor(random(255), random(255), random(255));
        x=2+random(216);
        y=16+random(143);
        x2=2+random(216);
        y2=16+random(143);
        myGLCD.drawRoundRect(x, y, x2, y2);
    }
    
    delay(2000);
    
    myGLCD.setColor(0,0,0);
    myGLCD.fillRect(1,15,218,160);
    
    for (int i=0; i<100; i++)
    {
        myGLCD.setColor(random(255), random(255), random(255));
        x=2+random(216);
        y=16+random(143);
        x2=2+random(216);
        y2=16+random(143);
        myGLCD.drawLine(x, y, x2, y2);
    }
    
    delay(2000);
    
    myGLCD.setColor(0,0,0);
    myGLCD.fillRect(1,15,218,160);
    
    for (int i=0; i<10000; i++)
    {
        myGLCD.setColor(random(255), random(255), random(255));
        myGLCD.drawPixel(2+random(216), 16+random(143));
    }
    
    delay(2000);
    
    myGLCD.fillScr(0, 0, 255);
    myGLCD.setColor(255, 0, 0);
    myGLCD.fillRoundRect(40, 57, 179, 119);
    
    myGLCD.setColor(255, 255, 255);
    myGLCD.setBackColor(255, 0, 0);
    myGLCD.print("That's it!", CENTER, 62);
    myGLCD.print("Restarting in a", CENTER, 88);
    myGLCD.print("few seconds...", CENTER, 101);
    
    myGLCD.setColor(0, 255, 0);
    myGLCD.setBackColor(0, 0, 255);
    myGLCD.print("Runtime: (msecs)", CENTER, 146);
    myGLCD.printNumI(millis(), CENTER, 161);
    
    delay (10000);
}


