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

#include <stdint.h> // We can compile without this, but it kills xcode completion without it! it took me a while to discover that..
#include <UTFT.h>

// Configure your screen sizes. Only tested with this size, and the Adafruit 2.2" TFT display
#define SCREEN_WIDTH 220
#define SCREEN_HEIGHT 176

#define ELITHION_CAN_SPEED CanSpeed500 // TODO: this should be an option!

#define ERROR_COLOR RED_COLOR
#define STORED_ERROR_COLOR 255, 0, 255
#define WARNING_COLOR 255, 128, 0 // organish
#define BACKGROUND_COLOR BLACK_COLOR

#define STATUS_COLOR 64, 64, 64
#define GRAY_COLOR 128, 128, 128

#define DEBUG 0

#define DELAY_BETWEEN_MESSAGES 500

#if 0 // DEBUG
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

#define DISPLAY_CHIP_SELECT 3 // the only variable pin (usually pin 10, but we want it on 3, as the Can bus shield uses 10)
#define DISPLAY_RESET 9 // Don't change
#define BUTTON_PIN A0

// This must match the bitset options above
#define MAX_FAULT_MESSAGES 10
char *FaultKindMessages[MAX_FAULT_MESSAGES] = {
    "Driving and plugged in",
    "Interlock tripped",
    "Communication fault",
    "Charge over current",
    "Discharge over current",
    "Over Temperature",
    "Under voltage",
    "Over voltage",
    "BMS not found",
    "CAN Bus init failed"
};

// MISO == 12
UTFT g_display(ADAFRUIT_2_2_TFT, MOSI/*11*/, SCK /*13*/, DISPLAY_CHIP_SELECT, DISPLAY_RESET);

// Shared global buffer for formatting
#define BUFFER_SIZE 28 // 27, max characters we can show, plus a NULL
char g_buffer[BUFFER_SIZE];

CanbusClass g_canBus;

static inline void setupLCD() {
    g_display.InitLCD();
    g_display.clrScr();
}

// 5 buttons are hooked up, each to a different resistor value from a 5v input, all tapped by one analog read pin to see the voltage value
static inline byte getPushedButton() {
    int val = analogRead(BUTTON_PIN);
    
#define BUTTON_ERROR_MARGIN 15  // +/- this value
    // Values found by measurement. Each button adds 1k resistance, and after the last resistor there is a 10k resistor
#define BUTTON_VALUE_0 928
#define BUTTON_VALUE_1 849
#define BUTTON_VALUE_2 782
#define BUTTON_VALUE_3 725
#define BUTTON_VALUE_4 676
    if (val >= (BUTTON_VALUE_0 - BUTTON_ERROR_MARGIN) && val <= (BUTTON_VALUE_0 + BUTTON_ERROR_MARGIN)) {
        return 5; // rightmost button
    } else if (val >= (BUTTON_VALUE_1 - BUTTON_ERROR_MARGIN) && val <= (BUTTON_VALUE_1 + BUTTON_ERROR_MARGIN)) {
        return 4;
    } else if (val >= (BUTTON_VALUE_2 - BUTTON_ERROR_MARGIN) && val <= (BUTTON_VALUE_2 + BUTTON_ERROR_MARGIN)) {
        return 3;
    } else if (val >= (BUTTON_VALUE_3 - BUTTON_ERROR_MARGIN) && val <= (BUTTON_VALUE_3 + BUTTON_ERROR_MARGIN)) {
        return 2;
    } else if (val >= (BUTTON_VALUE_4 - BUTTON_ERROR_MARGIN) && val <= (BUTTON_VALUE_4 + BUTTON_ERROR_MARGIN)) {
        return 1; // leftmost button
    }
    return 0;
}

enum _CurrentPage {
    CurrentPageStandard,
    CurrentPageDetailed,
};

typedef uint8_t CurrentPage;

static CurrentPage g_currentPage = CurrentPageStandard;
static CurrentPage g_lastPageShown = CurrentPageStandard;

static void checkButtonsAndUpdateCurrentPage() {
    byte pushedButton = getPushedButton();
    if (pushedButton != 0) {
        // Wait for the button to be up
        while (getPushedButton() != 0) {
            // ...
        }
#if DEBUG
        Serial.print("button:");
        Serial.println(pushedButton);
#endif
        // No process it
        switch (pushedButton) {
            case 1: {
                // Left
                g_currentPage = (CurrentPage)(g_currentPage - 1);
                if (g_currentPage < 0) {
                    g_currentPage = CurrentPageDetailed;
                }
                break;
            }
            case 2: {
                // right
                g_currentPage = (CurrentPage)(g_currentPage + 1);
                if (g_currentPage > CurrentPageDetailed) {
                    g_currentPage = CurrentPageStandard;
                }
                break;
            }
            case 5: {
                // enter. clear stored faults (this happens automatically on a delay anyways)
                g_canBus.clearStoredFault();
                break;
            }
        }
    }
}


static int8_t g_lastLimitAmount = -2; // Forces us to go through the code once; -1 is special
static uint8_t g_lastSOC = -1;

static inline void printSOCDisplay(int *yOffset, bool needsRedraw) {
    static URect g_lastSOCRect = URectMake(0, 0, 0, 0);
    
    uint8_t soc = g_canBus.getStateOfCharge();
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

// background colors must be set before calling this. Except the font color..which is always white
void printCenteredText(char *text, int *yOffset, URect *lastRect, uint8_t *font, bool needsRedraw, bool fillBackground) {
    if (text) {
        g_display.setFont(font);

        // manual centering...
        int textWidth = strlen(text)*g_display.getFont()->x_size;
        int xPos = floor((SCREEN_WIDTH - textWidth) / 2.0);
        
        URect currentRect = URectMake(xPos, *yOffset, textWidth, g_display.getFont()->y_size);
        
        // always fill...since the text y position may change, unless we calculate the center (which maybe I should do)
        if (lastRect && (!URectIsEqual(currentRect, *lastRect) || needsRedraw)) {
            int minY = max(URectMinY(*lastRect), *yOffset);
            // Do a smart fill only if we have to...
            if (URectMinY(*lastRect) != URectMinY(currentRect)) {
                // y changed...fill it all, with a flash
                URect rectToFill = *lastRect;
                rectToFill.origin.y = *yOffset;// Don't fill something before us..
                g_display.fillRect(rectToFill);
            } else if (URectMinX(currentRect) > URectMinX(*lastRect)) {
                // Fill to the min of new one and past it
                g_display.fillRect(URectMinX(*lastRect), minY, URectMinX(currentRect), URectMaxY(*lastRect));
                // past it
                g_display.fillRect(URectMaxX(currentRect), minY, URectMaxX(*lastRect), URectMaxY(*lastRect));
                
            }
            *lastRect = currentRect;
        }
        
        if (fillBackground) {
            URect fillRect = currentRect;
            fillRect.origin.x = 0;
            fillRect.size.width = SCREEN_WIDTH;
            g_display.fillRect(fillRect);
        }
        
        // font is always white (for now)
        g_display.setColor(WHITE_COLOR);
        g_display.print(text, URectMinX(currentRect), URectMinY(currentRect));
        *yOffset = URectMaxY(currentRect);
    } else {
        // Clear the prior rect
        if (lastRect && !URectIsEmpty(*lastRect)) {
            URect fillRect = *lastRect;
            fillRect.origin.y = *yOffset;// Don't fill something before us
            if (fillBackground) {
                fillRect.origin.x = 0;
                fillRect.size.width = SCREEN_WIDTH;
            }
            g_display.fillRect(fillRect);
            *lastRect = URectMake(0,0,0,0);
        }
    }
}

void printCenteredTextWithStandardColors(char *text, int *yOffset, URect *lastRect, uint8_t *font, bool needsRedraw) {
    g_display.setColor(BACKGROUND_COLOR);
    g_display.setBackColor(BACKGROUND_COLOR);
    printCenteredText(text, yOffset, lastRect, font, needsRedraw, false);
}

void printCenteredTextWithValue(char *text, int *yOffset, float *lastValue, float newValue, URect *lastRect, uint8_t *font, bool needsRedraw) {
    // print it, if it is different or has moved
    if (newValue != *lastValue || (*yOffset != URectMinY(*lastRect)) || needsRedraw) {
        *lastValue = newValue;

        // format the text then print it
        float_sprintf(g_buffer, text, newValue);
        printCenteredTextWithStandardColors(g_buffer, yOffset, lastRect, font, needsRedraw);
    } else {
        *yOffset = URectMaxY(*lastRect);
    }
}

void printPackCurrent(int *yOffset, bool needsRedraw) {
    static float g_lastValue = -1;
    static URect g_lastValueRect = URectMake(0, 0, 0, 0);
    float current = abs(g_canBus.getPackCurrent());
    printCenteredTextWithValue("amps", yOffset, &g_lastValue, current, &g_lastValueRect, BigFont, needsRedraw);
}

void printPackVoltage(int *yOffset, bool needsRedraw) {
    static float g_lastValue = -1;
    static URect g_lastValueRect = URectMake(0, 0, 0, 0);
    float voltage = g_canBus.getPackVoltage();
    printCenteredTextWithValue("volts", yOffset, &g_lastValue, voltage, &g_lastValueRect, BigFont, needsRedraw);
}

// background color is passed in
void printMessage(char *message, int *yOffset, byte r, byte g, byte b) {
    g_display.setColor(r, g, b);
    g_display.setBackColor(r, g, b);
    printCenteredText(message, yOffset, NULL, SmallFont, false, true);
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
            printMessage(FaultKindMessages[offsetIntoFaultMesssages], yOffset, r, g, b);
        }
        faults = faults >> 1; // Drop off the right most bit
        offsetIntoFaultMesssages++;
    }
    
}

char *c_strWarningFormat = "Warning: %s";

static void printWarnings(FaultKindOptions warnings, int *yOffset, byte r, byte g, byte b) {
    // printFaultsOrWarnings should work, but the values aren't what I expect. By observation, I'm figuring things out. If it is a bitset...i'm screwed
    switch (warnings) {
        case 0: {
            // Nothing to do, no warnings
            break;
        }
        case 1:
        {
            sprintf(g_buffer, c_strWarningFormat, FaultKindMessages[StoredFaultKindUnderVoltage - 1]);
            break;
        }
        case 2: {
            sprintf(g_buffer, c_strWarningFormat, FaultKindMessages[StoredFaultKindOverVoltage - 1]);
            break;
        }
        default: {
            // Unknown yet...
            sprintf(g_buffer, "Warning: %d", warnings);
            break;
        }
    }
    if (warnings) {
        printMessage(g_buffer, yOffset, WARNING_COLOR);
    }
}


static void printStoredFault(StoredFaultKind storedFault, int *yOffset, byte r, byte g, byte b) {
    if (storedFault) {
        if (storedFault < StoredFaultKindNoBatteryVoltage) {
            // Find the offset into the messages
            int offsetIntoFaultMesssages = storedFault - 1;
            printMessage(FaultKindMessages[offsetIntoFaultMesssages], yOffset, r, g, b);
        } else {
            // Print the error number
            sprintf(g_buffer, "Stored error: %03u", storedFault);
            printMessage(g_buffer, yOffset, r, g, b);
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
static int printErrorsAndWarnings(int *yOffset, bool needsRedraw) {
    FaultKindOptions presentFaults, presentWarnings;
    StoredFaultKind storedFault;
    g_canBus.getFaults(&presentFaults, &storedFault, &presentWarnings);
    
    int result = *yOffset;
    // See if they changed
    static FaultKindOptions g_lastPresentFaults = 0;
    static FaultKindOptions g_lastPresentWarnings = 0;
    static StoredFaultKind g_lastStoredFault = 0;
    static URect g_lastRect = URectMake(0, 0, 0, 0);
    
    bool forcedUpdate = needsRedraw || (!URectIsEmpty(g_lastRect) && URectMinY(g_lastRect) != *yOffset);
    
#if 0 //DEBUG
    if (presentFaults) {
        Serial.print("faults:");
        Serial.print(presentFaults);
        Serial.print(" y:");
        Serial.println(*yOffset);
    }
#endif
    
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
            // make sure we don't fill before the last y we drew
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
            result = URectMaxY(g_lastRect); // store off the  old value to redraw in case it went into the next area
            g_lastRect = URectMake(0, 0, 0, 0);
        }
    }
    return result;
}

void setup() {
#if DEBUG
    Serial.begin(9600);
#endif
    
    setupLCD();
    
    int yOffset = 0;
    printMessage("BMS Display v1.1", &yOffset, STATUS_COLOR);
    delay(DELAY_BETWEEN_MESSAGES);
    yOffset = 0;
    printMessage("by corbin dunn", &yOffset, STATUS_COLOR);
    delay(DELAY_BETWEEN_MESSAGES);
    
    g_canBus.init(ELITHION_CAN_SPEED); // Errors initing are ignored
}

static inline void printTimeLeftCharging(int *yOffset, bool needsRedraw) {
    // Calculate how long we have to finish charging at the current rate we are going
    float currentAvgSourceCurrent = abs(g_canBus.getAverageSourceCurrent());
    int depthOfDischarge = g_canBus.getDepthOfDischarge();
    int minutesLeft = 0;
    
    char *text = NULL;
    if (depthOfDischarge > 0 && currentAvgSourceCurrent > 0) {
        float hoursFractionalLeft = (float)depthOfDischarge / (float)currentAvgSourceCurrent;
        int minutesLeft = hoursFractionalLeft*60;
        if (hoursFractionalLeft < 1) {
            sprintf(g_buffer, "%d minutes left", minutesLeft);
            text = g_buffer;
        } else {
            int hoursLeft = floor(hoursFractionalLeft);
            minutesLeft = minutesLeft - hoursLeft * 60;
            sprintf(g_buffer, "%d hrs %d mins left", hoursLeft, minutesLeft);
            text = g_buffer;
        }
    }
    static float g_lastValue = -1;
    static URect g_lastValueRect = URectMake(0, 0, 0, 0);
    float current = minutesLeft;
    printCenteredTextWithStandardColors(text, yOffset, &g_lastValueRect, SmallFont, needsRedraw);
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

static inline void printMinMaxCells(int *yOffset, bool needsRedraw) {
    // This just always writes it again and again..which is okay, since it doesn't change positions, so it won't flicker

    _formatVoltageInBuff(g_buffer, "Min", g_canBus.getMinVoltage(), g_canBus.getMinVoltageCellNumber());
    printCenteredTextWithStandardColors(g_buffer, yOffset, NULL, SmallFont, needsRedraw);

    _formatVoltageInBuff(g_buffer, "Avg", g_canBus.getAvgVoltage(), g_canBus.getAvgVoltageCellNumber());
    printCenteredTextWithStandardColors(g_buffer, yOffset, NULL, SmallFont, needsRedraw);

    _formatVoltageInBuff(g_buffer, "Max", g_canBus.getMaxVoltage(), g_canBus.getMaxVoltageCellNumber());
    printCenteredTextWithStandardColors(g_buffer, yOffset, NULL, SmallFont, needsRedraw);
}

// string constants
static char *const c_ChargingStr = "Charging";
static const char *const c_pack = "Pack";
static const char *const c_cell = "Cell";
static const char *const c_vFormatStr = "%s voltage too %s";
static const char *const c_high = "high";
static const char *const c_low = "low";
static const char *const c_tempFormatStr = "Temperature too %s";
static char *const c_currentPeak = "Current peak too long";

static void setColorBasedOnLimitAmount(bool charging) {
    if (g_lastLimitAmount == 100) {
        // Normal BG color
        if (charging) {
            // Blue background
            g_display.setColor(BLUE_COLOR);
            g_display.setBackColor(BLUE_COLOR);
        } else {
            g_display.setColor(BACKGROUND_COLOR);
            g_display.setBackColor(BACKGROUND_COLOR);
        }
    } else {
        uint8_t limitBlue = 0; // Change the blue tone too when charging
        // Make it more red as we get closer to the limit (0% is limited all the way)
        byte limitRed = 255*(100-g_lastLimitAmount)/100;
        if (charging) {
            limitBlue = 255*(g_lastLimitAmount)/100;
        } else {
            limitBlue = 0;
        }
        g_display.setColor(limitRed, 0, limitBlue);
        g_display.setBackColor(limitRed, 0, limitBlue);
    }
}

static inline void printChargingOrLimitAmount(bool charging, int *yOffset, bool needsRedraw) {
    // Print a note at the top in blue.. assumes this never goes away
    int8_t limitAmount = charging ? g_canBus.getChargeLimitValue() : g_canBus.getDischargeLimitValue();
#if 0 //  DEBUG
    Serial.print(charging? "charg limit:" : "discharge");
    Serial.println(limitAmount);
#endif
    
    static URect g_lastDrawnRect = URectMake(0, 0, 0, 0);
    if (g_lastLimitAmount != limitAmount || needsRedraw) {
        // ignore errors in reading the limit value
        if (limitAmount != LimitCauseErrorReadingValue) {
            g_lastLimitAmount = limitAmount;
        } else {
            // BMS may not be intialized if we couldn't find it. Assume no limit, as we show another error right under.s
            if (g_lastLimitAmount < 0) {
                g_lastLimitAmount = 100;
            }
        }
        setColorBasedOnLimitAmount(charging);
        
        char *text;
        if (g_lastLimitAmount == 100) {
            text = charging ? c_ChargingStr : NULL;
        } else {
            // Print the limit, and then we will print why we are limited next in red!!
            sprintf(g_buffer, "%s limited to %d%%", charging? c_ChargingStr : "Discharging", limitAmount);
            text = g_buffer;
        }
        // We always force a redraw
        printCenteredText(text, yOffset, &g_lastDrawnRect, SmallFont, needsRedraw, true);
#if 0 // DEBUG
        Serial.print("printing: ");
        Serial.println(text);
        Serial.print("height:");
        Serial.println(URectHeight(g_lastDrawnRect));
#endif
    } else {
        *yOffset += URectHeight(g_lastDrawnRect);
    }
}

static inline void printChargingLimitReason(bool charging, int *yOffset, bool needsRedraw) {
    LimitCause limitCause = charging ? g_canBus.getChargeLimitCause() : g_canBus.getDischargeLimitCause();
    static LimitCause g_lastLimitCause = 0;
    static URect g_lastLimitRect = URectMake(0, 0, 0, 0);
    if (limitCause != g_lastLimitCause || needsRedraw) {
        g_lastLimitCause = limitCause;
        char *text = g_buffer;
        switch (g_lastLimitCause) {
            case LimitCauseNone: {
                text = NULL;
                break;
            }
            case LimitCausePackVoltageTooLow: {
                sprintf(g_buffer, c_vFormatStr, c_pack, c_low);
                break;
            }
            case LimitCausePackVoltageTooHigh: {
                sprintf(g_buffer, c_vFormatStr, c_pack, c_high);
                break;
            }
            case LimitCauseCellVoltageTooLow: {
                sprintf(g_buffer, c_vFormatStr, c_cell, c_low);
                break;
            }
            case LimitCauseCellVoltageTooHigh: {
                sprintf(g_buffer, c_vFormatStr, c_cell, c_high);
                break;
            }
            case LimitCauseTempTooHighToDischarge:
            case LimitCauseTempTooHighToCharge: {
                sprintf(g_buffer, c_tempFormatStr, c_high);
                break;
            }
            case LimitCauseTempTooLowToCharge:
            case LimitCauseTempTooLowToDischarge: {
                sprintf(g_buffer, c_tempFormatStr, c_low);
                break;
            }
            case LimitCauseDischargingCurrentPeakTooLong:
            case LimitCauseChargingCurrentPeakTooLong: {
                text = c_currentPeak;
                break;
            }
        }
        if (g_lastLimitCause) {
            setColorBasedOnLimitAmount(charging);
        } else {
            g_display.setColor(BACKGROUND_COLOR);
        }
        printCenteredText(text, yOffset, &g_lastLimitRect, SmallFont, needsRedraw, true);
    } else {
        // account for the limit so we don't draw over it
        *yOffset += URectHeight(g_lastLimitRect);
    }
}

static inline void printChargingStatus(bool charging, int *yOffset, bool needsRedraw) {
    printChargingOrLimitAmount(charging, yOffset, needsRedraw);
    printChargingLimitReason(charging, yOffset, needsRedraw);
}

#define SOC_START_Y 40

static inline void printStandardPage(bool needsRedraw) {
    IOFlags ioFlags = g_canBus.getIOFlags();
    checkButtonsAndUpdateCurrentPage();
    
    bool isCharging = ioFlags & IOFlagPowerFromSource; // If the bit isn't set, we are discharging, or we couldn't find the BMS on the can bus
    
    // Each area is completely responsible for the height and width in that area. It is almost as though I need to develop some lightweight view mechanism
    int yOffset = 0;
    
    // print the common things
    printChargingStatus(isCharging, &yOffset, needsRedraw);
    checkButtonsAndUpdateCurrentPage();
    
    int lastY = printErrorsAndWarnings(&yOffset, needsRedraw);
    needsRedraw = needsRedraw || (lastY > SOC_START_Y); // we might have to redraw the rest of the stuff depending on the location of the last errors..
    
    //hardcode the y start for everything else. The warnings and errors might bleed into this area.
    yOffset = SOC_START_Y;
    printSOCDisplay(&yOffset, needsRedraw);
    checkButtonsAndUpdateCurrentPage();
    printPackCurrent(&yOffset, needsRedraw);
    checkButtonsAndUpdateCurrentPage();
    printPackVoltage(&yOffset, needsRedraw);
    checkButtonsAndUpdateCurrentPage();
    
    if (isCharging) {
        printTimeLeftCharging(&yOffset, needsRedraw);
    } else {
        // Leave a gap
        yOffset += 12;
    }
    
    printMinMaxCells(&yOffset, needsRedraw);
}

static inline void drawGraphSOC(int yPos) {
    int soc = g_canBus.getStateOfCharge();
    if (soc != g_lastSOC) {
        g_lastSOC = soc;
        // Right align; 4 chars
        int xPos = SCREEN_WIDTH - g_display.getFont()->x_size*9;
        if (g_lastSOC < 100) {
            g_display.print(" ", xPos, yPos);
            xPos++;
        }
        if (g_lastSOC < 10) {
            g_display.print(" ", xPos, yPos);
            xPos++;
        }
        sprintf(g_buffer, "SOC: %d%%", g_lastSOC);
        g_display.print(g_buffer, xPos, yPos);
    }
}

static inline void setColorForGraphForCellLevel(float cellLevel) {
    if (cellLevel <= 2.7 || cellLevel >= 3.7) {
        // sagging to below pretty low..red
        g_display.setColor(RED_COLOR);
    } else if (cellLevel < 3.0 || cellLevel >= 3.5) {
        g_display.setColor(ORANGE_COLOR); // getting low or high
    } else {
        g_display.setColor(0, 255, 0); // green
    }
}

static void _formatVoltage2InBuff(char *buffer, char *kind, float value) {
    int d1 = floor(value);
    float f2 = value - d1;     // Get fractional part (678.0123 - 678 = 0.0123).
    int d2 = round(f2 * 100);   // Turn into integer with 2 digits
    
    char *format;
    sprintf(buffer, "%s: %d.%02dv", kind, d1, d2);
}


static inline void drawCellGraphPage(bool needsRedraw) {
    // individual cell levels; everything drawn each time
    g_display.setFont(SmallFont);
    g_display.setBackColor(BACKGROUND_COLOR);
    g_display.setColor(WHITE_COLOR);
    
    int yPos = 1;
    if (needsRedraw) {
        // Print the title
        g_display.print("Cell Levels", 0, yPos);
    }
    drawGraphSOC(yPos);
    
    yPos += g_display.getFont()->y_size + 4;
    
    int maxYPos = SCREEN_HEIGHT - 1 - 3*g_display.getFont()->y_size - 2;
    int xPos = 0;
    int amountAvailableXWidth = SCREEN_WIDTH - 1;
    g_display.setColor(BLUE_COLOR);
    g_display.drawRoundRect(xPos, yPos, amountAvailableXWidth, maxYPos);
    
    amountAvailableXWidth -= 2;

    yPos++;

    int graphHeight = maxYPos - yPos - 1;
    
#define MIN_GRAPH_VOLTAGE 2.0
#define MAX_GRAPH_VOLTAGE 4.0
    
#define MAX_CELL_NUMBERS_TO_SHOW 4

    // I really only care about min and max cells
    byte minCellCount = 0;
    byte minCells[MAX_CELL_NUMBERS_TO_SHOW];
    
    byte maxCells[MAX_CELL_NUMBERS_TO_SHOW];
    byte maxCellCount = 0; // at most 128 cells...
    
    float minCellVoltage = 5.0;
    float maxCellVoltage = 0.0;
    
    // Get each cell's voltage and draw it
    int numberOfCells = g_canBus.getNumberOfCells();
    if (numberOfCells > 0) {
        int amountToShowPerCell = floor(amountAvailableXWidth / numberOfCells);
        if (amountToShowPerCell <= 0) {
            // Don't have enough room to show it all..should do something!
            amountToShowPerCell = 3; // Need at least 2 pixels per cell; two for the color, and one for a separator color
        }
        // Find the best starting xPos
        xPos = xPos + floor((amountAvailableXWidth - numberOfCells*amountToShowPerCell)/2);
        
        for (int cell = 0; cell < numberOfCells; cell++) {
            float voltage = g_canBus.getVoltageForCell(cell);
            // Keep track of min/max
            if (voltage < minCellVoltage) {
                minCellVoltage = voltage;
                // reset
                minCells[0] = cell;
                minCellCount = 1;
            } else if (voltage == minCellVoltage) {
                // Keep track of it..
                if (minCellCount < MAX_CELL_NUMBERS_TO_SHOW) {
                    minCells[minCellCount] = cell;
                }
                minCellCount++;
            }
            
            // Keep track of min/max
            if (voltage > maxCellVoltage) {
                maxCellVoltage = voltage;
                // reset
                maxCells[0] = cell;
                maxCellCount = 1;
            } else if (voltage == maxCellVoltage) {
                // Keep track of it..
                if (maxCellCount < MAX_CELL_NUMBERS_TO_SHOW) {
                    maxCells[maxCellCount] = cell;
                }
                maxCellCount++;
            }
            
            // how much of a percent is that?
            float percent = (voltage - MIN_GRAPH_VOLTAGE) / (MAX_GRAPH_VOLTAGE - MIN_GRAPH_VOLTAGE);
            int voltYAmount = round(percent * graphHeight);
            int yPosForLevel = maxYPos - voltYAmount;
            // Fill with background to that pos, then down with the color
            int maxXForColor = xPos + amountToShowPerCell - 2;
            int backgroundColorAmount = yPosForLevel - yPos;
            if (backgroundColorAmount > 0) {
                g_display.setColor(BACKGROUND_COLOR);
                g_display.fillRect(xPos, yPos, maxXForColor, yPosForLevel);
            }
            setColorForGraphForCellLevel(voltage);
            g_display.fillRect(xPos, yPosForLevel, maxXForColor, maxYPos);
            
            xPos += amountToShowPerCell;
        }
    }
    
    g_display.setColor(WHITE_COLOR);

    byte charWidth = g_display.getFont()->x_size;
    byte fontHeight = g_display.getFont()->y_size;

    // TODO: maybe make this a function instead of a long bunch of code that is very similar?
    yPos = maxYPos + 2;
    xPos = charWidth; // initial spacing
    _formatVoltage2InBuff(g_buffer, "Min", minCellVoltage);
    g_display.print(g_buffer, xPos, yPos);
    xPos = xPos + strlen(g_buffer)*charWidth + charWidth;
    g_display.print("(", xPos, yPos);
    xPos += charWidth;
    // print the numbers
    for (int cell = 0; cell < min(minCellCount, MAX_CELL_NUMBERS_TO_SHOW); cell++) {
        if (cell > 0) {
            g_display.print(",", xPos, yPos);
            xPos += charWidth;
        }
        sprintf(g_buffer, "%d", minCells[cell]);
        g_display.print(g_buffer, xPos, yPos);
        xPos += strlen(g_buffer)*charWidth;
    }
    if (minCellCount > MAX_CELL_NUMBERS_TO_SHOW) {
        g_display.print(",...)", xPos, yPos);
        xPos += charWidth * 5;
    } else {
        g_display.print(")", xPos, yPos);
        xPos += charWidth * 1;
    }
    // Fill to the end
    g_display.setColor(BACKGROUND_COLOR);
    g_display.fillRect(xPos, yPos, SCREEN_WIDTH - 1, yPos + fontHeight);
    
    yPos += fontHeight;
    
    // max        -------------- this code is almost identical to the above code and should be a function...
    g_display.setColor(WHITE_COLOR);
    yPos = yPos + 2;
    xPos = charWidth; // initial spacing
    _formatVoltage2InBuff(g_buffer, "Max", maxCellVoltage);
    g_display.print(g_buffer, xPos, yPos);
    xPos = xPos + strlen(g_buffer)*charWidth + charWidth;
    g_display.print("(", xPos, yPos);
    xPos += charWidth;
    // print the numbers
    for (int cell = 0; cell < min(maxCellCount, MAX_CELL_NUMBERS_TO_SHOW); cell++) {
        if (cell > 0) {
            g_display.print(",", xPos, yPos);
            xPos += charWidth;
        }
        sprintf(g_buffer, "%d", maxCells[cell]);
        g_display.print(g_buffer, xPos, yPos);
        xPos += strlen(g_buffer)*charWidth;
    }
    
    if (maxCellCount > MAX_CELL_NUMBERS_TO_SHOW) {
        g_display.print(",...)", xPos, yPos);
        xPos += charWidth * 5;
    } else {
        g_display.print(")", xPos, yPos);
        xPos += charWidth * 1;
    }
    // Fill to the end
    g_display.setColor(BACKGROUND_COLOR);
    g_display.fillRect(xPos, yPos, SCREEN_WIDTH - 1, yPos + fontHeight);
    yPos += fontHeight;
}

void loop() {
    // We do checkButtons a lot, as we might "miss" some due to the timeout for each call
    checkButtonsAndUpdateCurrentPage();
    checkAndClearStoredFault();
    
    bool needsRedraw = false;
    // See if the page we are showing is different
    if (g_lastPageShown != g_currentPage) {
        g_lastPageShown = g_currentPage;
        // fill with black
        g_display.clrScr();
        
        g_lastSOC = -1; // Reset the SOC
        needsRedraw = true;
    }

    switch (g_currentPage) {
        case CurrentPageStandard: {
            printStandardPage(needsRedraw);
            break;
        }
        case CurrentPageDetailed: {
            drawCellGraphPage(needsRedraw);
            break;
        }
            
    }
}

