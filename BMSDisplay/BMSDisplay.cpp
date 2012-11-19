// BMSDisplay.cpp
//  Created by corbin dunn on 5/18/12

#include  "Arduino.h" // — for Arduino 1.0

//#include <SD.h>        // Library from Adafruit.com 
//#include <SdFatUtil.h>
//#include <SoftwareSerial.h>
#include <Canbus.h>

#include <Wire.h>

#include <HardwareSerial.h>

//#include <EEPROM.h>
//#include <avr/eeprom.h>

//#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t
//#include <Time.h>
//#include <TimeAlarms.h> // NOTE: not using; I could remove this

#include <stdint.h> // We can compile without this, but it kills xcode completion without it! it took me a while to discover that..

//#include <SPI.h>
#include <UTFT.h>


// Configure your screen sizes. Only tested with this size, and the Adafruit 2.2" TFT display
#define SCREEN_WIDTH 220
#define SCREEN_HEIGHT 176

#define ELITHION_CAN_SPEED CanSpeed500 // TODO: this should be an option!


#define ERROR_COLOR RED_COLOR
#define STORED_ERROR_COLOR 255, 0, 255
#define WARNING_COLOR ORANGE_COLOR
#define BACKGROUND_COLOR BLACK_COLOR

#define STATUS_COLOR 64, 64, 64

#define DEBUG 1

#if DEBUG
    // Make a nice visual assert using the display!
    #define ASSERT(cond, message) \
        if (!(cond)) { \
            Serial.println(message); \
            g_display.setColor(RED_COLOR); g_display.fillRect(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT - 1); \
            g_display.setBackColor(RED_COLOR); g_display.setColor(WHITE_COLOR); g_display.setFont(SmallFont); \
            g_display.print(message, CENTER, 0); delay(5000); \
        }
#else
    #define ASSERT(cond, message)
#endif

// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];

//// Define Joystick connection
//#define UP     A1
//#define RIGHT  A2
//#define DOWN   A3
//#define CLICK  A4
//#define LEFT   A5

int CAN_BUS_LED2 = 8;
int CAN_BUS_LED3 = 7;

CanbusClass g_canBus;

static inline void setupCanbusPins() {
    pinMode(CAN_BUS_LED2, OUTPUT);
    pinMode(CAN_BUS_LED3, OUTPUT);
    
    digitalWrite(CAN_BUS_LED2, LOW);
    digitalWrite(CAN_BUS_LED3, LOW);
    
//    pinMode(UP,INPUT);
//    pinMode(DOWN,INPUT);
//    pinMode(LEFT,INPUT);
//    pinMode(RIGHT,INPUT);
//    pinMode(CLICK,INPUT);
//    
//    digitalWrite(UP, HIGH);       // Enable internal pull-ups
//    digitalWrite(DOWN, HIGH);
//    digitalWrite(LEFT, HIGH);
//    digitalWrite(RIGHT, HIGH);
//    digitalWrite(CLICK, HIGH);
}

#define DISPLAY_CHIP_SELECT 3 // the only variable pin (usually
#define DISPLAY_RESET 9

// MISO == 12
UTFT g_display(ADAFRUIT_2_2_TFT, MOSI/*11*/, SCK /*13*/, DISPLAY_CHIP_SELECT, DISPLAY_RESET);

static inline void setupLCD() {
    g_display.InitLCD();
    g_display.clrScr();
}

void printSOCDisplay(int *yOffset) {
    static int g_lastSOC = -1;
    static URect g_lastSOCRect = URectMake(0, 0, 0, 0);
    
    int soc = g_canBus.getStateOfCharge();
    if (soc != g_lastSOC) {
        g_lastSOC = soc;
        g_display.setFont(SevenSegNumFont);
        
        int fontHeight = g_display.getFont()->y_size;
        int fontWidth = g_display.getFont()->x_size;
        
        int digitCount = 0;
        if (soc == 0) {
            digitCount = 1;
        } else {
            int temp = soc;
            while (temp > 0) {
                digitCount++;
                temp=(temp-(temp % 10))/10;
            }
        }
        
        int yPos = *yOffset;
        int xPos = floor((SCREEN_WIDTH - (digitCount*fontWidth)) / 2.0);
        g_display.setFont(BigFont);
        int percentYPos = yPos + floor((fontHeight - g_display.getFont()->y_size) / 2.0);
        int percentXPos = xPos + digitCount*fontWidth + 2; // spacing..
        
        URect socRect = URectMake(xPos, yPos, (percentXPos - xPos) + g_display.getFont()->x_size, fontHeight);
        if (!URectIsEqual(socRect, g_lastSOCRect)) {
            g_display.setColor(BACKGROUND_COLOR);
            g_display.fillRect(g_lastSOCRect);
            g_lastSOCRect = socRect;
        }

        g_display.setFont(SevenSegNumFont);
        g_display.setBackColor(BACKGROUND_COLOR); // black
        g_display.setColor(WHITE_COLOR); // white
        g_display.printNumI(soc, xPos, yPos);
        
        g_display.setFont(BigFont);
        g_display.print("%", percentXPos, percentYPos);
    }
    *yOffset = URectMaxY(g_lastSOCRect);
}


// stupid embedded. %.2f don't work!
static void float_sprintf(char *buffer, char *postfix, float value) {
    int d1 = floor(value);
    float f2 = value - d1;     // Get fractional part (678.0123 - 678 = 0.0123).
    int d2 = round(f2 * 10);   // Turn into integer (0).
    
    char *format;
    if (d2 == 0) {
        sprintf(buffer, "%d %s", d1, postfix);
    } else {
        sprintf(buffer, "%d.%d %s", d1, d2, postfix);
    }
}

void printCenteredText(char *text, int *yOffset, float *lastValue, float newValue, URect *lastRect, uint8_t *font) {
    // print it, if it is different or has moved
    if (newValue != *lastValue || (*yOffset != URectMinY(*lastRect))) {
        *lastValue = newValue;

        g_display.setFont(font);

        // TODO: test for buffer overrun!
        char buffer[32];
        float_sprintf(buffer, text, newValue);
        
        // manual centering...
        int textWidth = strlen(buffer)*g_display.getFont()->x_size;
        int xPos = floor((g_display.getDisplayXSize() - textWidth) / 2.0);
        
        URect currentRect = URectMake(xPos, *yOffset, textWidth, g_display.getFont()->y_size);
        // always fill...since the text y position may change, unless we calculate the center (which maybe I should do)
        if (!URectIsEqual(currentRect, *lastRect)) {
            // TODO: think about the moved part, because if the yOffset is >, then we don't want to cutoff the previous yOffset stuff drawn

            // Do a smart fill only if we have to...
            if (URectMinX(currentRect) > URectMinX(*lastRect)) {
                g_display.setColor(BACKGROUND_COLOR);
                // Fill to the min of new one and past it
                g_display.fillRect(URectMinX(*lastRect), URectMinY(*lastRect), URectMinX(currentRect), URectMaxY(*lastRect));
                // past it
                g_display.fillRect(URectMaxX(currentRect), URectMinY(*lastRect), URectMaxX(*lastRect), URectMaxY(*lastRect));
                
            }
            *lastRect = currentRect;
        }
        
        g_display.setColor(WHITE_COLOR);
        g_display.setBackColor(BACKGROUND_COLOR);
        g_display.print(buffer, URectMinX(currentRect), URectMinY(currentRect));
    }
    *yOffset = URectMaxY(*lastRect);
}

void printPackCurrent(int *yOffset) {
    static float g_lastValue = -1;
    static URect g_lastValueRect = URectMake(0, 0, 0, 0);
    float current = abs(g_canBus.getPackCurrent());
    printCenteredText("amps", yOffset, &g_lastValue, current, &g_lastValueRect, BigFont);
}

void printPackVoltage(int *yOffset) {
    static float g_lastValue = -1;
    static URect g_lastValueRect = URectMake(0, 0, 0, 0);
    float voltage = g_canBus.getPackVoltage();
    printCenteredText("volts", yOffset, &g_lastValue, voltage, &g_lastValueRect, BigFont);
}

void printLoadCurrent(int *yOffset) {
    static float g_lastValue = -1;
    static URect g_lastValueRect = URectMake(0, 0, 0, 0);
    float current = abs(g_canBus.getLoadCurrent());
    printCenteredText("amps (current)", yOffset, &g_lastValue, current, &g_lastValueRect, SmallFont);
}

void printAvgSourceCurrent(int *yOffset) {
    static float g_lastValue = -1;
    static URect g_lastValueRect = URectMake(0, 0, 0, 0);
    float current = abs(g_canBus.getAverageSourceCurrent());
    printCenteredText("amps avg charge", yOffset, &g_lastValue, current, &g_lastValueRect, SmallFont);
}


// background color is passed in
void printMessage(const char *message, int *yOffset, int yMinOutset, int yMaxOutset, byte r, byte g, byte b) {
    unsigned char fontHeight = g_display.getFont()->y_size;
    int totalHeight = yMinOutset + fontHeight + yMaxOutset;
    g_display.setColor(r, g, b);
    g_display.fillRect(0, *yOffset, SCREEN_WIDTH - 1, *yOffset + totalHeight);
    g_display.setColor(WHITE_COLOR); // white text always for now
    g_display.setBackColor(r, g, b);
    g_display.print(message, CENTER, *yOffset + yMinOutset);
    *yOffset += totalHeight;
}

static void setAndPrintStatus(char *statusMessage) {
    char *g_currentStatus = NULL;
    if (g_currentStatus != statusMessage) {
        g_currentStatus = statusMessage;
        static char *g_lastStatus = NULL;
        if (g_currentStatus != NULL && g_lastStatus != g_currentStatus) {
            int yOffset = 0;
            g_lastStatus = g_currentStatus;
            g_display.setFont(SmallFont);
            printMessage(g_currentStatus, &yOffset, 1, 1, STATUS_COLOR);
        } else {
            // Clear the prior!
            // corbin...
        }
    }
}

static void printFaultsOrWarnings(FaultKindOptions faults, int *yOffset, byte r, byte g, byte b) {
    // Go through the present faults
    int offsetIntoFaultMesssages = 0;
    while (faults > 0) {
        ASSERT(offsetIntoFaultMesssages < MAX_FAULT_MESSAGES, "INTERNAL ERROR: offset");
        // Is the current bit set?
        const FaultKindOptions faultMask = 0x1; // rightmost bit
        if ((faults & faultMask) == faultMask) {
            // That fault is set; print it
            printMessage(FaultKindMessages[offsetIntoFaultMesssages], yOffset, 0, 0, r, g, b);
        }
        faults = faults >> 1; // Drop off the right most bit
        offsetIntoFaultMesssages++;
    }
    
}

static void printStoredFault(StoredFaultKind storedFault, int *yOffset, byte r, byte g, byte b) {
    if (storedFault) {
        if (storedFault < StoredFaultKindNoBatteryVoltage) {
            // Find the offset into the messages
            int offsetIntoFaultMesssages = storedFault - 1;
            printMessage(FaultKindMessages[offsetIntoFaultMesssages], yOffset, 0, 0, r, g, b);
        } else {
            // Print the error number
            char buffer[32];
            sprintf(buffer, "Stored error number: %03u", storedFault);
            printMessage(buffer, yOffset, 0, 0, r, g, b);
        }
    }
}

// Returns the y-offset that we last did drawing at
static void printErrorsAndWarnings(int yOffset) {
    FaultKindOptions presentFaults, presentWarnings;
    StoredFaultKind storedFault;
    g_canBus.getFaults(&presentFaults, &storedFault, &presentWarnings);
    
    // See if they changed
    static FaultKindOptions g_lastPresentFaults = 0;
    static FaultKindOptions g_lastPresentWarnings = 0;
    static StoredFaultKind g_lastStoredFault = 0;
    if (g_lastPresentFaults != presentFaults || g_lastPresentWarnings != presentWarnings || g_lastStoredFault != storedFault) {
        static URect g_lastRect = URectMake(0, 0, 0, 0);
        g_lastPresentFaults = presentFaults;
        g_lastPresentWarnings = presentWarnings;
        g_lastStoredFault = storedFault;
        
        // Fill the last rect we drew as we may be drawing different things
        if (!URectIsEmpty(g_lastRect)) {
            g_display.setColor(BACKGROUND_COLOR);
            g_display.fillRect(g_lastRect);
        }
        if (presentFaults || storedFault || presentWarnings) {
            int yOriginOffset = yOffset;
            g_lastRect = URectMake(0, yOffset, SCREEN_WIDTH, 1);
            
            g_display.setFont(SmallFont);
            // print a border on the top and bottom
            g_display.setColor(ERROR_COLOR);
            g_display.fillRect(g_lastRect);
            yOffset += URectHeight(g_lastRect);
            
            printFaultsOrWarnings(presentFaults, &yOffset, ERROR_COLOR);
            printStoredFault(storedFault, &yOffset, STORED_ERROR_COLOR);
            printFaultsOrWarnings(presentWarnings, &yOffset, WARNING_COLOR);
            // print warnings as hex...so i can figure out what is up as they are wrong..
            if (presentWarnings) {
                char buffer[32];
                sprintf(buffer, "RAW WARNINGS: %d", presentWarnings);
                printMessage(buffer, &yOffset, 0, 0, WARNING_COLOR);
            }
            
            g_display.setColor(ERROR_COLOR);
            g_lastRect.origin.y = yOffset;
            g_display.fillRect(g_lastRect);

            // Save the entire size so we can clear it on the next pass/update
            g_lastRect.size.height = URectMaxY(g_lastRect) - yOriginOffset;
            g_lastRect.origin.y = yOriginOffset;
        } else {
            g_lastRect = URectMake(0, 0, 0, 0);
        }
    }
}


void setup() {
#if DEBUG
    Serial.begin(9600);
    Serial.println("initialized");
#endif
    
    setupLCD();
    
    int yOffset;
//    
//    setAndPrintStatus("BMS Display for Elithion");
//    delay(1000);
//    setAndPrintStatus("by corbin dunn");
//    delay(500);
    
    setupCanbusPins();

    if (g_canBus.init(ELITHION_CAN_SPEED)) {
        setAndPrintStatus("CAN bus initialized");
        delay(500); // Show that for a brief moment...
        setAndPrintStatus(NULL); // Show something?
    }
}

void loop() {
    // High when it is doing work
    digitalWrite(CAN_BUS_LED3, HIGH);

    // Each area is completely responsible for the height and width in that area. It is almost as though I need to develop some lightweight view mechanism
    int yOffset = 0;
    printErrorsAndWarnings(yOffset);
    
    //hardcode the y start for everything else. The warnings and errors might bleed into this area.
    yOffset = 40;
    
    printSOCDisplay(&yOffset);
    printPackCurrent(&yOffset);
    printPackVoltage(&yOffset);
    printLoadCurrent(&yOffset);
//    printAvgSourceCurrent(&yOffset);

    // TODO: remove the delays...?
    delay(100);
    digitalWrite(CAN_BUS_LED3, LOW);
    delay(100);
}


