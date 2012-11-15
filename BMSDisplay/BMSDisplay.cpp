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
    digitalWrite(CAN_BUS_LED3, LOW);
    
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
    
    setupLCD();
    
    printTitle("** BMS Display for Elithion **");

    setupCanbus();
    updateSOCDisplay();

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
//    printStatus("Looping...", StatusKindNormal);
#if DEBUG
//    Serial.println("loop");
#endif

    delay(100);
    digitalWrite(CAN_BUS_LED3, HIGH);
    
    if(Canbus.ecu_req(ENGINE_RPM,buffer) == 1)          // Request for engine RPM
  {
//    sLCD.write(COMMAND);                   // Move LCD cursor to line 0 
//    sLCD.write(LINE0);
    Serial.println(buffer);                         // Display data on LCD
   
    
  }

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
     
   
    delay(100);
   digitalWrite(CAN_BUS_LED3, LOW);
   
   

}


