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
#define GRAY_COLOR 128, 128, 128

#define DEBUG 0

#define DELAY_BETWEEN_MESSAGES 500

#if DEBUG
    // Make a nice visual assert using the display!
    #define ASSERT(cond, message) \
        if (!(cond)) { \
            Serial.println(message); \
            g_display.setColor(RED_COLOR); g_display.fillRect(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT - 1); \
            g_display.setBackColor(RED_COLOR); g_display.setColor(WHITE_COLOR); g_display.setFont(SmallFont); \
            g_display.print(message, CENTER, 0); delay(5000); \
        }
    #define DELAY_BETWEEN_MESSAGES 5 // start faster
#else
    #define ASSERT(cond, message)
#endif

// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];

int CAN_BUS_LED2 = 8;
int CAN_BUS_LED3 = 7;

CanbusClass g_canBus;

static inline void setupCanbusPins() {
    pinMode(CAN_BUS_LED2, OUTPUT);
    pinMode(CAN_BUS_LED3, OUTPUT);
    
    digitalWrite(CAN_BUS_LED2, LOW);
    digitalWrite(CAN_BUS_LED3, LOW);
}

#define DISPLAY_CHIP_SELECT 3 // the only variable pin (usually
#define DISPLAY_RESET 9

// MISO == 12
UTFT g_display(ADAFRUIT_2_2_TFT, MOSI/*11*/, SCK /*13*/, DISPLAY_CHIP_SELECT, DISPLAY_RESET);

static inline void setupLCD() {
    g_display.InitLCD();
    g_display.clrScr();
}

static int8_t g_lastLimitAmount = -2; // Forces us to go through the code once; -1 is special

void printSOCDisplay(int *yOffset, bool needsRedraw) {
    static int g_lastSOC = -1;
    static URect g_lastSOCRect = URectMake(0, 0, 0, 0);
    
    int soc = g_canBus.getStateOfCharge();
    // We always redraw if the limit is 0!
    if (soc != g_lastSOC || needsRedraw|| g_lastLimitAmount == 0) {
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
        
        // Use red if we are limited!! so it really shows up
        if (g_lastLimitAmount == 0) {
            g_display.setColor(RED_COLOR);
            g_display.setBackColor(RED_COLOR);
        } else {
            g_display.setColor(BACKGROUND_COLOR);
            g_display.setBackColor(BACKGROUND_COLOR);
        }
        
        URect socRect = URectMake(xPos, yPos, (percentXPos - xPos) + g_display.getFont()->x_size, fontHeight);
        if (!URectIsEqual(socRect, g_lastSOCRect)) {
            g_display.fillRect(g_lastSOCRect);
            g_lastSOCRect = socRect;
        }

        g_display.setFont(SevenSegNumFont);
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

void printCenteredText(char *text, int *yOffset, URect *lastRect, uint8_t *font) {
    if (text) {
        g_display.setFont(font);

        // manual centering...
        int textWidth = strlen(text)*g_display.getFont()->x_size;
        int xPos = floor((g_display.getDisplayXSize() - textWidth) / 2.0);
        
        URect currentRect = URectMake(xPos, *yOffset, textWidth, g_display.getFont()->y_size);
        // always fill...since the text y position may change, unless we calculate the center (which maybe I should do)
        if (lastRect && !URectIsEqual(currentRect, *lastRect)) {
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
        g_display.print(text, URectMinX(currentRect), URectMinY(currentRect));
        *yOffset = URectMaxY(currentRect);
    } else {
        // Clear the prior rect
        if (lastRect && !URectIsEmpty(*lastRect)) {
            g_display.setColor(BACKGROUND_COLOR);
            g_display.fillRect(*lastRect);
            *lastRect = URectMake(0,0,0,0);
        }
    }
}

#define BUFFER_SIZE 28 // 27, max characters we can show, plus a NULL

void printCenteredTextWithValue(char *text, int *yOffset, float *lastValue, float newValue, URect *lastRect, uint8_t *font) {
    // print it, if it is different or has moved
    if (newValue != *lastValue || (*yOffset != URectMinY(*lastRect))) {
        *lastValue = newValue;

        // format the text then print it
        char buffer[BUFFER_SIZE];
        float_sprintf(buffer, text, newValue);
        printCenteredText(buffer, yOffset, lastRect, font);
    } else {
        *yOffset = URectMaxY(*lastRect);
    }
}

void printPackCurrent(int *yOffset) {
    static float g_lastValue = -1;
    static URect g_lastValueRect = URectMake(0, 0, 0, 0);
    float current = abs(g_canBus.getPackCurrent());
    printCenteredTextWithValue("amps", yOffset, &g_lastValue, current, &g_lastValueRect, BigFont);
}

void printPackVoltage(int *yOffset) {
    static float g_lastValue = -1;
    static URect g_lastValueRect = URectMake(0, 0, 0, 0);
    float voltage = g_canBus.getPackVoltage();
    printCenteredTextWithValue("volts", yOffset, &g_lastValue, voltage, &g_lastValueRect, BigFont);
}

void printLoadCurrent(int *yOffset) {
    static float g_lastValue = -1;
    static URect g_lastValueRect = URectMake(0, 0, 0, 0);
    float current = abs(g_canBus.getLoadCurrent());
    printCenteredTextWithValue("amps (current)", yOffset, &g_lastValue, current, &g_lastValueRect, SmallFont);
}

void printAvgSourceCurrent(int *yOffset) {
    static float g_lastValue = -1;
    static URect g_lastValueRect = URectMake(0, 0, 0, 0);
    float current = abs(g_canBus.getAverageSourceCurrent());
    printCenteredTextWithValue("amps avg charge", yOffset, &g_lastValue, current, &g_lastValueRect, SmallFont);
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

static void printStatusWithColor(const char *statusMessage, byte r, byte g, byte b, int *yOffset) {
//    static const char *g_currentStatus = NULL;
//    if (g_currentStatus != statusMessage) // corbin
    {
//        g_currentStatus = statusMessage;
        if (statusMessage != NULL) {
            g_display.setFont(SmallFont);
            printMessage(statusMessage, yOffset, 1, 1, r, g, b);
        } else {
            // Clear the prior...
            URect r = URectMake(0, *yOffset, g_display.getDisplayXSize(), g_display.getFont()->y_size + 2);
            g_display.setColor(BACKGROUND_COLOR);
            g_display.fillRect(r);
        }
    }
//    else {
//        // Update the yOffset;
//        if (g_currentStatus) {
//            g_display.setFont(SmallFont);
//            *yOffset += g_display.getFont()->y_size + 2;
//        }
//    }
}

static inline void printStatus(char *statusMessage) {
    int yOffset = 0;
    printStatusWithColor(statusMessage, STATUS_COLOR, &yOffset);
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

static void printWarnings(FaultKindOptions warnings, int *yOffset, byte r, byte g, byte b) {
    // printFaultsOrWarnings should work, but the values aren't what I expect. By observation, I'm figuring things out. If it is a bitset...i'm screwed
    switch (warnings) {
        case 0: {
            // Nothing to do, no warnings
            break;
        }
        case 2: {
            char buffer[BUFFER_SIZE];
            sprintf(buffer, "Warning: %s", FaultKindMessages[StoredFaultKindOverVoltage - 1]);
#if 0 // DEBUG
            Serial.println("    ----------- printing warning");
            Serial.println(buffer);
#endif
            printMessage(buffer, yOffset, 0, 0, WARNING_COLOR);
            break;
        }
        default: {
            // Unknown yet...
            char buffer[BUFFER_SIZE];
            sprintf(buffer, "RAW WARNINGS: %d", warnings);
            printMessage(buffer, yOffset, 0, 0, WARNING_COLOR);
            break;
        }
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
            char buffer[BUFFER_SIZE];
            sprintf(buffer, "Stored error number: %03u", storedFault);
            printMessage(buffer, yOffset, 0, 0, r, g, b);
        }
    }
}

static unsigned long g_storedFaultStartTime = 0;
static inline void startStoredFaultClearTimer() {
    g_storedFaultStartTime = millis();
}

static inline void clearStoredFaultClearTimer() {
    g_storedFaultStartTime = 0;
}

#if DEBUG
    #define TIME_TO_CLEAR_STORED_FAULT (1000*30*1) // 30 seconds
#else
    #define TIME_TO_CLEAR_STORED_FAULT (1000*60*3) // 3 minutes
#endif

static inline void checkAndClearStoredFault() {
    if (g_storedFaultStartTime > 0 && (millis() - g_storedFaultStartTime) >= TIME_TO_CLEAR_STORED_FAULT) {
        clearStoredFaultClearTimer();
        g_canBus.clearStoredFault();
    }
}

// returns the prior y we drew at if we cleared
static int printErrorsAndWarnings(int *yOffset) {
    FaultKindOptions presentFaults, presentWarnings;
    StoredFaultKind storedFault;
    g_canBus.getFaults(&presentFaults, &storedFault, &presentWarnings);
    
    int result = *yOffset;
    // See if they changed
    static FaultKindOptions g_lastPresentFaults = 0;
    static FaultKindOptions g_lastPresentWarnings = 0;
    static StoredFaultKind g_lastStoredFault = 0;
    static URect g_lastRect = URectMake(0, 0, 0, 0);
    
    bool forcedUpdate = !URectIsEmpty(g_lastRect) && URectMinY(g_lastRect) != *yOffset;
    
    if (g_lastPresentFaults != presentFaults || g_lastPresentWarnings != presentWarnings || g_lastStoredFault != storedFault || forcedUpdate) {
        g_lastPresentFaults = presentFaults;
        g_lastPresentWarnings = presentWarnings;
        
        // Start a timer to clear the last stored fault; show it for a minute, then reset it
        if (g_lastStoredFault != storedFault) {
            g_lastStoredFault = storedFault;
            if (g_lastStoredFault) {
                // Start the timer
                startStoredFaultClearTimer();
            } else {
                // Stop the timer to clear
                clearStoredFaultClearTimer();
            }
        }
        
        // Fill the last rect we drew as we may be drawing different things
        if (!URectIsEmpty(g_lastRect)) {
            g_display.setColor(BACKGROUND_COLOR);
            // make sure we dont' fill before the last y we drew
            int amountOver = *yOffset - URectMinY(g_lastRect);
            if (amountOver > 0) {
                g_lastRect.origin.y += amountOver;
                g_lastRect.size.height -= amountOver;
            }
            g_display.fillRect(g_lastRect);
        }
        if (presentFaults || storedFault || presentWarnings) {
            int yOriginOffset = *yOffset;
            g_lastRect = URectMake(0, yOriginOffset, SCREEN_WIDTH, 1);
            
            g_display.setFont(SmallFont);
            // print a border on the top and bottom
            g_display.setColor(ERROR_COLOR);
            g_display.fillRect(g_lastRect);
            *yOffset += URectHeight(g_lastRect);
            
            printFaultsOrWarnings(presentFaults, yOffset, ERROR_COLOR);
            printStoredFault(storedFault, yOffset, STORED_ERROR_COLOR);
            printWarnings(presentWarnings, yOffset, WARNING_COLOR);
            
            g_display.setColor(ERROR_COLOR);
            g_lastRect.origin.y = *yOffset;
            g_display.fillRect(g_lastRect);

            // Save the entire size so we can clear it on the next pass/update
            g_lastRect.size.height = URectMaxY(g_lastRect) - yOriginOffset;
            g_lastRect.origin.y = yOriginOffset;
        } else {
            result = URectMaxY(g_lastRect); // store off the old value to redraw in case it went into the next area
            g_lastRect = URectMake(0, 0, 0, 0);
        }
    }
    return result;
}

void setup() {
#if DEBUG
    Serial.begin(9600);
    Serial.println("initialized");
#endif
    
    setupLCD();
    
    int yOffset;

    printStatus("BMS Display for Elithion");
    delay(DELAY_BETWEEN_MESSAGES);
    printStatus("by corbin dunn");
    delay(DELAY_BETWEEN_MESSAGES);
    
    setupCanbusPins();

    if (g_canBus.init(ELITHION_CAN_SPEED)) {
        printStatus("CAN bus initialized");
        delay(DELAY_BETWEEN_MESSAGES); // Show that for a brief moment...
        
        // TODO: Get and print the elithion BMS version
        
        // See how we are powered
        IOFlags ioFlags = g_canBus.getIOFlags();
        if (ioFlags & IOFlagPowerFromSource) {
            printStatus("BMS in charging mode");
        } else if (ioFlags & IOFlagPowerFromLoad) {
            printStatus("BMS in discharging mode");
        } else {
            // BMS not found...
        }
        delay(DELAY_BETWEEN_MESSAGES);
        
        printStatus(NULL); // Show something?
    }
}

static inline void printTimeLeftCharging(int *yOffset) {
    // Calculate how long we have to finish charging at the current rate we are going
    float currentAvgSourceCurrent = abs(g_canBus.getAverageSourceCurrent());
    int depthOfDischarge = g_canBus.getDepthOfDischarge();
    int minutesLeft = 0;
    
    char *text = NULL;
    char buffer[BUFFER_SIZE];
    if (depthOfDischarge > 0 && currentAvgSourceCurrent > 0) {
        float hoursFractionalLeft = (float)depthOfDischarge / (float)currentAvgSourceCurrent;
        int minutesLeft = hoursFractionalLeft*60;
        if (hoursFractionalLeft < 1) {
            sprintf(buffer, "%d minutes left", minutesLeft);
            text = buffer;
        } else {
            int hoursLeft = floor(hoursFractionalLeft);
            minutesLeft = minutesLeft - hoursLeft * 60;
            sprintf(buffer, "%d hrs %d mins left", hoursLeft, minutesLeft);
            text = buffer;
        }
    }
    static float g_lastValue = -1;
    static URect g_lastValueRect = URectMake(0, 0, 0, 0);
    float current = minutesLeft;
    printCenteredText(text, yOffset, &g_lastValueRect, SmallFont);
    if (!text) {
        // no text to show, but leave space...
        g_display.setFont(SmallFont);
        *yOffset += g_display.getFont()->y_size;
    }
}

static void _formatVoltageInBuff(char *buffer, char *kind, float value, int cellNumber) {
    int d1 = floor(value);
    float f2 = value - d1;     // Get fractional part (678.0123 - 678 = 0.0123).
    int d2 = round(f2 * 100);   // Turn into integer with 2 digits
    
    char *format;
    sprintf(buffer, "%s cell: %d.%02dv (#%d)", kind, d1, d2, cellNumber);
}

static inline void printMinMaxCells(int *yOffset) {
    // This just always writes it again and again..which is okay, since it doesn't change positions, so it won't flicker

    char buffer[BUFFER_SIZE];

    _formatVoltageInBuff(buffer, "Min", g_canBus.getMinVoltage(), g_canBus.getMinVoltageCellNumber());
    printCenteredText(buffer, yOffset, NULL, SmallFont);

    _formatVoltageInBuff(buffer, "Avg", g_canBus.getAvgVoltage(), g_canBus.getAvgVoltageCellNumber());
    printCenteredText(buffer, yOffset, NULL, SmallFont);

    _formatVoltageInBuff(buffer, "Max", g_canBus.getMaxVoltage(), g_canBus.getMaxVoltageCellNumber());
    printCenteredText(buffer, yOffset, NULL, SmallFont);
}

// string constants
static const char *const c_ChargingStr = "Charging";
static const char *const c_pack = "Pack";
static const char *const c_cell = "Cell";
static const char *const c_vFormatStr = "%s voltage too %s";
static const char *const c_high = "high";
static const char *const c_low = "low";
static const char *const c_tempFormatStr = "Temperature too %s";
static const char *const c_currentPeak = "Current peak too long";

// TODO: corbin, clean this method up. it got ugly fast
static inline void printChargingStatus(bool charging, int *yOffset) {
    // Print a note at the top in blue.. assumes this never goes away
    int8_t limitAmount = charging ? g_canBus.getChargeLimitValue() : g_canBus.getDischargeLimitValue();
#if 1 // DEBUG
    Serial.print(charging? "charg limit:" : "discharge");
    Serial.println(limitAmount);
#endif
    
    char buffer[BUFFER_SIZE];
    
    uint8_t limitRectColor = 255; // full red
    uint8_t limitBlue = 0;
    
    if (g_lastLimitAmount != limitAmount) {
        // ignore errors in reading the limit value
        if (limitAmount != LimitCauseErrorReadingValue) {
            g_lastLimitAmount = limitAmount;
        } else {
            // BMS may not be intialized if we couldn't find it. Assume no limit, as we show another error right under.s
            if (g_lastLimitAmount < 0) {
                g_lastLimitAmount = 100;
            }
        }
        if (g_lastLimitAmount == 100) {
            if (charging) {
                printStatusWithColor(c_ChargingStr, BLUE_COLOR, yOffset);
            } else {
                // don't show anything if we are normally ready and have no problems
                printStatusWithColor(NULL, GRAY_COLOR, yOffset);
            }
        } else {
            // Print the limit, and then we will print why we are limited next in red!!
            sprintf(buffer, "%s limited to %d%%", charging? c_ChargingStr : "Discharging", limitAmount);
            // Make it more red as we get closer to the limit (0% is limited all the way)
            limitRectColor = 255*(100-limitAmount)/100;
            if (charging) {
                limitBlue = 255*(limitAmount)/100;
            } else {
                limitBlue = 0;
            }
            printStatusWithColor(buffer, limitRectColor, 0, limitBlue, yOffset);
        }
    } else {
        // Limit amount hasn't changed...but update our y post
        g_display.setFont(SmallFont);
        *yOffset += g_display.getFont()->y_size + 2;
        limitRectColor = 255*(100-g_lastLimitAmount)/100;
        if (charging) {
            limitBlue = 255*(limitAmount)/100;
        } else {
            limitBlue = 128;
        }
    }
    
    g_display.setFont(SmallFont);
    
    LimitCause limitCause = charging ? g_canBus.getChargeLimitCause() : g_canBus.getDischargeLimitCause();
    static LimitCause g_lastLimitCause = 0;
    if (limitCause != g_lastLimitCause) {
        g_lastLimitCause = limitCause;
        if (g_lastLimitCause == LimitCauseNone) {
            // Fill in the last area...
            g_display.setColor(BACKGROUND_COLOR);
            URect r = URectMake(0, *yOffset, SCREEN_WIDTH, g_display.getFont()->y_size + 1);
            g_display.fillRect(r);
        } else {
            const char *ptr = buffer;
            switch (g_lastLimitCause) {
                case LimitCausePackVoltageTooLow: {
                    sprintf(buffer, c_vFormatStr, c_pack, c_low);
                    break;
                }
                case LimitCausePackVoltageTooHigh: {
                    sprintf(buffer, c_vFormatStr, c_pack, c_high);
                    break;
                }
                case LimitCauseCellVoltageTooLow: {
                    sprintf(buffer, c_vFormatStr, c_cell, c_low);
                    break;
                }
                case LimitCauseCellVoltageTooHigh: {
                    sprintf(buffer, c_vFormatStr, c_cell, c_high);
                    break;
                }
                case LimitCauseTempTooHighToDischarge:
                case LimitCauseTempTooHighToCharge: {
                    sprintf(buffer, c_tempFormatStr, c_high);
                    break;
                }
                case LimitCauseTempTooLowToCharge:
                case LimitCauseTempTooLowToDischarge: {
                    sprintf(buffer, c_tempFormatStr, c_low);
                    break;
                }
                case LimitCauseDischargingCurrentPeakTooLong:
                case LimitCauseChargingCurrentPeakTooLong: {
                    ptr = c_currentPeak;
                    break;
                }
            }
            
            printMessage(ptr, yOffset, 0, 1, limitRectColor, 0, limitBlue);
        }
    } else if (g_lastLimitCause != LimitCauseNone) {
        // account for the limit so we don't draw over it
        *yOffset += g_display.getFont()->y_size + 1;
    }
}

void loop() {
    // High when it is doing work
    digitalWrite(CAN_BUS_LED3, HIGH);
    
    checkAndClearStoredFault();

    // Find out if we are in discharge or charge mode (i don't support both)
    IOFlags ioFlags = g_canBus.getIOFlags();
    bool isCharging = ioFlags & IOFlagPowerFromSource; // If the bit isn't set, we are discharging, or we couldn't find the BMS on the can bus
    
    // Each area is completely responsible for the height and width in that area. It is almost as though I need to develop some lightweight view mechanism
    int yOffset = 0;
    printChargingStatus(isCharging, &yOffset);
    
    int lastY = printErrorsAndWarnings(&yOffset);
    
    //hardcode the y start for everything else. The warnings and errors might bleed into this area.
#define SOC_START_Y 40
    bool needsRedraw = (lastY > SOC_START_Y);
    yOffset = SOC_START_Y;
    printSOCDisplay(&yOffset, needsRedraw);
    printPackCurrent(&yOffset);
    printPackVoltage(&yOffset);

    if (isCharging) {
        printTimeLeftCharging(&yOffset);
    } else {
        // Leave a gap
        yOffset += 12;
    }

    printMinMaxCells(&yOffset);
    
    // TODO: remove the delays...?
    delay(100);
    digitalWrite(CAN_BUS_LED3, LOW);
    delay(100);
}

