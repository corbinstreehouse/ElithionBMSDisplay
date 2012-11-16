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

CanbusClass g_canBus;

static inline void setupCanbusPins() {
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

UTFT g_display(ADAFRUIT_2_2_TFT, 11, 13, 3, 9);   // Remember to change the model parameter to suit your display module!

float g_stateOfCharge = 0;

static inline void setupLCD() {
    //randomSeed(analogRead(0));
    g_display.InitLCD();
    g_display.clrScr();
}

void updateSOCDisplay() {
    g_display.setBackColor(0, 0, 0); // black
    g_display.setColor(255,255,255); // white
    g_display.setFont(SevenSegNumFont);
    int soc = g_canBus.stateOfCharge();
    g_display.printNumI(soc, CENTER, 50);
}

static inline void printTitle(char *title) {
    g_display.setBackColor(64, 64, 64);
    g_display.setColor(64, 64, 64);
    g_display.fillRect(0, 0, SCREEN_WIDTH - 1, 13);
    
    g_display.setColor(255, 255, 255);
    g_display.setFont(SmallFont);
    g_display.print(title, CENTER, 1);
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
            g_display.setBackColor(64, 64, 64);
            g_display.setColor(64, 64, 64);
            break;
        }
        case StatusKindWarning: {
#warning TODO: appropriate color
            g_display.setBackColor(64, 64, 64);
            g_display.setColor(64, 64, 64);
            break;
        }
        default:
        case StatusKindError: {
            g_display.setBackColor(255, 0, 0); // red background
            g_display.setColor(255, 0, 0);
            break;
        }
    }
    //Fill a square at the top
    g_display.fillRect(0, 162, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
 
    g_display.setColor(255, 255, 255); // white text
    g_display.setFont(SmallFont);
    g_display.print(statusMessage, CENTER, 163);
}

#define ELITHION_CAN_SPEED CanSpeed500 // TODO: this should be an option!
 
void setup() {
    Serial.begin(9600);
#if DEBUG
    Serial.println("initialized");
#endif
    
    setupLCD();
    
    printTitle("** BMS Display for Elithion **");

    setupCanbusPins();

    if (g_canBus.init(ELITHION_CAN_SPEED)) {
        printStatus("Can bus initialized", StatusKindNormal);
    } else {
        printStatus("FAILED CAN BUS INIT", StatusKindError);
        delay(5000); // Show it for 5 seconds
    }
}

void loop() {
    // High when it is doing work
    digitalWrite(CAN_BUS_LED3, HIGH);
    updateSOCDisplay();
    
    
    
    delay(100);
    digitalWrite(CAN_BUS_LED3, LOW);
    delay(100);
}


