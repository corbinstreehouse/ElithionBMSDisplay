// BMSDisplay.cpp
//  Created by corbin dunn on 5/18/12

#include  "Arduino.h" // — for Arduino 1.0


#include <SD.h>        // Library from Adafruit.com 
#include <SdFatUtil.h>
#include <SoftwareSerial.h>
#include <Canbus.h>

#include <Wire.h>
//#include <Adafruit_MCP23017.h>
//#include <Adafruit_RGBLCDShield.h>

//#include <EEPROM.h>
//#include <avr/eeprom.h>

//#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t
//#include <Time.h>
//#include <TimeAlarms.h> // NOTE: not using; I could remove this

#include <stdint.h> // We can compile without this, but it kills xcode completion without it! it took me a while to discover that..

#define DEBUG 1
/*

Sd2Card card;
SdVolume volume;
SdFile root;
SdFile file;

SoftwareSerial sLCD =  SoftwareSerial(3, 6); // Serial LCD is connected on pin 14 (Analog input 0)
#define COMMAND 0xFE
#define CLEAR   0x01
#define LINE0   0x80
#define LINE1   0xC0

void clear_lcd(void);

// Define Joystick connection
#define UP     A1
#define RIGHT  A2
#define DOWN   A3
#define CLICK  A4
#define LEFT   A5

  
char buffer[512];  //Data will be temporarily stored to this buffer before being written to the file
char tempbuf[15];
char lat_str[14];
char lon_str[14];

int read_size=0;   //Used as an indicator for how many characters are read from the file
int count=0;       //Miscellaneous variable

int D10 = 10;

int LED2 = 8;
int LED3 = 7;

// store error strings in flash to save RAM
#define error(s) error_P(PSTR(s))

void error_P(const char* str) {
  PgmPrint("error: ");
  SerialPrintln_P(str);
  
  clear_lcd();
  sLCD.print("SD error");
  
  if (card.errorCode()) {
    PgmPrint("SD error: ");
    Serial.print(card.errorCode(), HEX);
    
    Serial.print(',');
    Serial.println(card.errorData(), HEX);
   
  }
  while(1);
}

SoftwareSerial mySerial =  SoftwareSerial(4, 5);

#define COMMAND 0xFE
//#define powerpin 4

#define GPSRATE 4800
//#define GPSRATE 38400



// GPS parser for 406a
#define BUFFSIZ 90 // plenty big
//char buffer[BUFFSIZ];
char *parseptr;
char buffidx;
uint8_t hour, minute, second, year, month, date;
uint32_t latitude, longitude;
uint8_t groundspeed, trackangle;
char latdir, longdir;
char status;
uint32_t waypoint = 0;
 
void gps_test(void);
void sd_test(void);
void logging(void);
void read_gps(void);
void readline(void);
uint32_t parsedecimal(char *str);

void setup() {
    Serial.begin(9600);
#if DEBUG
    delay(2000);

    Serial.println("initialized");
    int c = Serial.read();
    while (c == -1) {
        delay(500);
        Serial.println("Waiting...type a character to begin..");
        c = Serial.read();
    }
    Serial.println("Done waiting...starting....");
#endif
    
    
    
  mySerial.begin(GPSRATE);
  pinMode(LED2, OUTPUT); 
  pinMode(LED3, OUTPUT); 
 
  digitalWrite(LED2, LOW);
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
  
  
  Serial.begin(9600);
  Serial.println("ECU Reader");  // For debug use 
  
  sLCD.begin(9600);              // Setup serial LCD and clear the screen 
  clear_lcd();
 
  sLCD.print("D:CAN  U:GPS");
  sLCD.write(COMMAND);
  sLCD.write(LINE1); 
  sLCD.print("L:SD   R:LOG");
  
  while(1)
  {
    
    if (digitalRead(UP) == 0){
      Serial.println("gps");
      sLCD.print("GPS");
      gps_test();
    }
    
    if (digitalRead(DOWN) == 0) {
      sLCD.print("CAN");
      Serial.println("CAN");
      break;
    }
    
    if (digitalRead(LEFT) == 0) {
    
      Serial.println("SD test");
      sd_test();
    }
    
    if (digitalRead(RIGHT) == 0) {
    
      Serial.println("Logging");
      logging();
    }
    
  }
  
  clear_lcd();
  
  if(Canbus.init(CANSPEED_500))  // Initialise MCP2515 CAN controller at the specified speed 
  {
    sLCD.print("CAN Init ok");
      Serial.println("can init ok");

  } else
  {
    sLCD.print("Can't init CAN");
      Serial.println("can't init can");
  } 
   
  delay(1000); 

}
 

void loop() {
#if DEBUG
    Serial.println("loop");
#endif
  if(Canbus.ecu_req(ENGINE_RPM,buffer) == 1)          // Request for engine RPM 
  {
    sLCD.write(COMMAND);                   // Move LCD cursor to line 0 
    sLCD.write(LINE0);
    sLCD.print(buffer);                         // Display data on LCD 
   
    
  } 
  digitalWrite(LED3, HIGH);
   
  if(Canbus.ecu_req(VEHICLE_SPEED,buffer) == 1)
  {
    sLCD.write(COMMAND);
    sLCD.write(LINE0 + 9);
    sLCD.print(buffer);
   
  }
  
  if(Canbus.ecu_req(ENGINE_COOLANT_TEMP,buffer) == 1)
  {
    sLCD.write(COMMAND);
    sLCD.write(LINE1);                     // Move LCD cursor to line 1 
    sLCD.print(buffer);
   
   
  }
  
  if(Canbus.ecu_req(THROTTLE,buffer) == 1)
  {
    sLCD.write(COMMAND);
    sLCD.write(LINE1 + 9);
    sLCD.print(buffer);
     file.print(buffer);
  }  
//  Canbus.ecu_req(O2_VOLTAGE,buffer);
     
   
   digitalWrite(LED3, LOW); 
   delay(100); 
   
   

}


void logging(void)
{
  clear_lcd();
  
  if(Canbus.init(CANSPEED_500))  // Initialise MCP2515 CAN controller at the specified speed 
  {
    sLCD.print("CAN Init ok");
  } else
  {
    sLCD.print("Can't init CAN");
  } 
   
  delay(500);
  clear_lcd(); 
  sLCD.print("Init SD card");  
  delay(500);
  clear_lcd(); 
  sLCD.print("Press J/S click");  
  sLCD.write(COMMAND);
  sLCD.write(LINE1);                     // Move LCD cursor to line 1 
   sLCD.print("to Stop"); 
  
  // initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
  // breadboards.  use SPI_FULL_SPEED for better performance.
  if (!card.init(SPI_HALF_SPEED,9)) error("card.init failed");
  
  // initialize a FAT volume
  if (!volume.init(&card)) error("volume.init failed");
  
  // open the root directory
  if (!root.openRoot(&volume)) error("openRoot failed");

  // create a new file
  char name[] = "WRITE00.TXT";
  for (uint8_t i = 0; i < 100; i++) {
    name[5] = i/10 + '0';
    name[6] = i%10 + '0';
    if (file.open(&root, name, O_CREAT | O_EXCL | O_WRITE)) break;
  }
  if (!file.isOpen()) error ("file.create");
  Serial.print("Writing to: ");
  Serial.println(name);
  // write header
//  file.writeError = 0;
  file.print("READY....");
  file.println();  

  while(1)    // Main logging loop 
  {
     read_gps();
     
     file.print(waypoint++);
     file.print(',');
     file.print(lat_str);
     file.print(',');
     file.print(lon_str);
     file.print(',');
      
    if(Canbus.ecu_req(ENGINE_RPM,buffer) == 1)          // Request for engine RPM
      {
        sLCD.write(COMMAND);                   // Move LCD cursor to line 0 
        sLCD.write(LINE0);
        sLCD.print(buffer);                         // Display data on LCD 
        file.print(buffer);
         file.print(',');
    
      } 
      digitalWrite(LED3, HIGH);
   
      if(Canbus.ecu_req(VEHICLE_SPEED,buffer) == 1)
      {
        sLCD.write(COMMAND);
        sLCD.write(LINE0 + 9);
        sLCD.print(buffer);
        file.print(buffer);
        file.print(','); 
      }
      
      if(Canbus.ecu_req(ENGINE_COOLANT_TEMP,buffer) == 1)
      {
        sLCD.write(COMMAND);
        sLCD.write(LINE1);                     // Move LCD cursor to line 1 
        sLCD.print(buffer);
         file.print(buffer);
       
      }
      
      if(Canbus.ecu_req(THROTTLE,buffer) == 1)
      {
        sLCD.write(COMMAND);
        sLCD.write(LINE1 + 9);
        sLCD.print(buffer);
         file.print(buffer);
      }  
    //  Canbus.ecu_req(O2_VOLTAGE,buffer);
       file.println();  
  
       digitalWrite(LED3, LOW); 
 
       if (digitalRead(CLICK) == 0){  // Check for Click button 
           file.close();
           Serial.println("Done");
           sLCD.write(COMMAND);
           sLCD.write(CLEAR);
     
           sLCD.print("DONE");
          while(1);
        }

  }
 
 
 
 
}
     

void sd_test(void)
{
 clear_lcd(); 
 sLCD.print("SD test"); 
 Serial.println("SD card test");
   
     // initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
  // breadboards.  use SPI_FULL_SPEED for better performance.
  if (!card.init(SPI_HALF_SPEED,9)) error("card.init failed");
  
  // initialize a FAT volume
  if (!volume.init(&card)) error("volume.init failed");
  
  // open root directory
  if (!root.openRoot(&volume)) error("openRoot failed");
  // open a file
  if (file.open(&root, "LOGGER00.CSV", O_READ)) {
    Serial.println("Opened PRINT00.TXT");
  }
  else if (file.open(&root, "WRITE00.TXT", O_READ)) {
    Serial.println("Opened WRITE00.TXT");    
  }
  else{
    error("file.open failed");
  }
  Serial.println();
  
  // copy file to serial port
  int16_t n;
  uint8_t buf[7];// nothing special about 7, just a lucky number.
  while ((n = file.read(buf, sizeof(buf))) > 0) {
    for (uint8_t i = 0; i < n; i++) Serial.print(buf[i]);
  
  
  }
 clear_lcd();  
 sLCD.print("DONE"); 

  
 while(1);  // Don't return 
    

}
void read_gps(void)
{
 uint32_t tmp;

  unsigned char i;
  unsigned char exit = 0;
  
  
  while( exit == 0)
  { 
    
   readline();
 
  // check if $GPRMC (global positioning fixed data)
   if (strncmp(buffer, "$GPRMC",6) == 0) {
     
        digitalWrite(LED2, HIGH);
 
        // hhmmss time data
        parseptr = buffer+7;
        tmp = parsedecimal(parseptr); 
        hour = tmp / 10000;
        minute = (tmp / 100) % 100;
        second = tmp % 100;
        
        parseptr = strchr(parseptr, ',') + 1;
        status = parseptr[0];
        parseptr += 2;
          
        for(i=0;i<11;i++)
        {
          lat_str[i] = parseptr[i];
        }
        lat_str[12] = 0;
      //  Serial.println(" ");
      //  Serial.println(lat_str);
       
        // grab latitude & long data
        latitude = parsedecimal(parseptr);
        if (latitude != 0) {
          latitude *= 10000;
          parseptr = strchr(parseptr, '.')+1;
          latitude += parsedecimal(parseptr);
        }
        parseptr = strchr(parseptr, ',') + 1;
        // read latitude N/S data
        if (parseptr[0] != ',') {
          
          latdir = parseptr[0];
        }
        
        // longitude
        parseptr = strchr(parseptr, ',')+1;
      
        for(i=0;i<12;i++)
        {
          lon_str[i] = parseptr[i];
        }
        lon_str[13] = 0;
        
        //Serial.println(lon_str);
   
        longitude = parsedecimal(parseptr);
        if (longitude != 0) {
          longitude *= 10000;
          parseptr = strchr(parseptr, '.')+1;
          longitude += parsedecimal(parseptr);
        }
        parseptr = strchr(parseptr, ',')+1;
        // read longitude E/W data
        if (parseptr[0] != ',') {
          longdir = parseptr[0];
        }
        
    
        // groundspeed
        parseptr = strchr(parseptr, ',')+1;
        groundspeed = parsedecimal(parseptr);
    
        // track angle
        parseptr = strchr(parseptr, ',')+1;
        trackangle = parsedecimal(parseptr);
    
        // date
        parseptr = strchr(parseptr, ',')+1;
        tmp = parsedecimal(parseptr); 
        date = tmp / 10000;
        month = (tmp / 100) % 100;
        year = tmp % 100;
        
       
        digitalWrite(LED2, LOW);
        exit = 1;
       }
       
  }   

}

      
      

void gps_test(void){
  uint32_t tmp;
  uint32_t lat;
  unsigned char i;
  
  while(1){
  
   readline();
  
  // check if $GPRMC (global positioning fixed data)
  if (strncmp(buffer, "$GPRMC",6) == 0) {
    
    // hhmmss time data
    parseptr = buffer+7;
    tmp = parsedecimal(parseptr); 
    hour = tmp / 10000;
    minute = (tmp / 100) % 100;
    second = tmp % 100;
    
    parseptr = strchr(parseptr, ',') + 1;
    status = parseptr[0];
    parseptr += 2;
      
    for(i=0;i<11;i++)
    {
      lat_str[i] = parseptr[i];
    }
    lat_str[12] = 0;
     Serial.println("\nlat_str ");
     Serial.println(lat_str);
   
  
    // grab latitude & long data
    // latitude
    latitude = parsedecimal(parseptr);
    if (latitude != 0) {
      latitude *= 10000;
      parseptr = strchr(parseptr, '.')+1;
      latitude += parsedecimal(parseptr);
    }
    parseptr = strchr(parseptr, ',') + 1;
    // read latitude N/S data
    if (parseptr[0] != ',') {
      
      latdir = parseptr[0];
    }
    
    //Serial.println(latdir);
    
    // longitude
    parseptr = strchr(parseptr, ',')+1;
  
    for(i=0;i<12;i++)
    {
      lon_str[i] = parseptr[i];
    }
    lon_str[13] = 0;
    
    Serial.println(lon_str);
   
  
    longitude = parsedecimal(parseptr);
    if (longitude != 0) {
      longitude *= 10000;
      parseptr = strchr(parseptr, '.')+1;
      longitude += parsedecimal(parseptr);
    }
    parseptr = strchr(parseptr, ',')+1;
    // read longitude E/W data
    if (parseptr[0] != ',') {
      longdir = parseptr[0];
    }
    

    // groundspeed
    parseptr = strchr(parseptr, ',')+1;
    groundspeed = parsedecimal(parseptr);

    // track angle
    parseptr = strchr(parseptr, ',')+1;
    trackangle = parsedecimal(parseptr);

    // date
    parseptr = strchr(parseptr, ',')+1;
    tmp = parsedecimal(parseptr); 
    date = tmp / 10000;
    month = (tmp / 100) % 100;
    year = tmp % 100;
    
    Serial.print("\nTime: ");
    Serial.print(hour, DEC); Serial.print(':');
    Serial.print(minute, DEC); Serial.print(':');
    Serial.print(second, DEC); Serial.print(' ');
    Serial.print("Date: ");
    Serial.print(month, DEC); Serial.print('/');
    Serial.print(date, DEC); Serial.print('/');
    Serial.println(year, DEC);
    
    sLCD.write(COMMAND);
    sLCD.write(0x80);
    sLCD.print("La");
   
    Serial.print("Lat"); 
    if (latdir == 'N')
    {
       Serial.print('+');
       sLCD.print("+");
    }
    else if (latdir == 'S')
    {  
       Serial.print('-');
       sLCD.print("-");
    }
     
    Serial.print(latitude/1000000, DEC); Serial.write('\°'); Serial.print(' ');
    Serial.print((latitude/10000)%100, DEC); Serial.print('\''); Serial.print(' ');
    Serial.print((latitude%10000)*6/1000, DEC); Serial.print('.');
    Serial.print(((latitude%10000)*6/10)%100, DEC); Serial.println('"');
    
    
    
    sLCD.print(latitude/1000000, DEC); sLCD.write(0xDF); sLCD.print(' ');
    sLCD.print((latitude/10000)%100, DEC); sLCD.print('\''); //sLCD.print(' ');
    sLCD.print((latitude%10000)*6/1000, DEC); sLCD.print('.');
    sLCD.print(((latitude%10000)*6/10)%100, DEC); sLCD.print('"');
    
    sLCD.write(COMMAND);
    sLCD.write(0xC0);
    sLCD.print("Ln");
   
      
    Serial.print("Long: ");
    if (longdir == 'E')
    {
       Serial.print('+');
       sLCD.print('+');
    }
    else if (longdir == 'W')
    { 
       Serial.print('-');
       sLCD.print('-');
    }
    Serial.print(longitude/1000000, DEC); Serial.write('\°'); Serial.print(' ');
    Serial.print((longitude/10000)%100, DEC); Serial.print('\''); Serial.print(' ');
    Serial.print((longitude%10000)*6/1000, DEC); Serial.print('.');
    Serial.print(((longitude%10000)*6/10)%100, DEC); Serial.println('"');
    
    sLCD.print(longitude/1000000, DEC); sLCD.write(0xDF); sLCD.print(' ');
    sLCD.print((longitude/10000)%100, DEC); sLCD.print('\''); //sLCD.print(' ');
    sLCD.print((longitude%10000)*6/1000, DEC); sLCD.print('.');
    sLCD.print(((longitude%10000)*6/10)%100, DEC); sLCD.print('"');
     
  }
  
 //   Serial.println("Lat: ");
 //   Serial.println(latitude);
  
 //   Serial.println("Lon: ");
 //   Serial.println(longitude);
  }



}

void readline(void) {
  char c;
  
  buffidx = 0; // start at begninning
  while (1) {
      c=mySerial.read();
      if (c == -1)
        continue;
Serial.print(c);
      if (c == '\n')
        continue;
      if ((buffidx == BUFFSIZ-1) || (c == '\r')) {
        buffer[buffidx] = 0;
        return;
      }
      buffer[buffidx++]= c;
  }
}
uint32_t parsedecimal(char *str) {
  uint32_t d = 0;
  
  while (str[0] != 0) {
   if ((str[0] > '9') || (str[0] < '0'))
     return d;
   d *= 10;
   d += str[0] - '0';
   str++;
  }
  return d;
}

void clear_lcd(void)
{
  sLCD.write(COMMAND);
  sLCD.write(CLEAR);
}  

*/


/***************************************************
 This is an example sketch for the Adafruit 2.2" SPI display.
 This library works with the Adafruit 2.2" TFT Breakout w/SD card
 ----> http://www.adafruit.com/products/797
 
 Check out the links above for our tutorials and wiring diagrams
 These displays use SPI to communicate, 3 or 4 pins are required to
 interface (RST is optional)
 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!
 
 Written by Limor Fried/Ladyada for Adafruit Industries.
 MIT license, all text above must be included in any redistribution
 ****************************************************/

#include <Adafruit_GFX.h>
#include <Adafruit_HX8340B.h>

// if we're using fast hardware SPI on an '328 or '168 arduino such
// as an Uno, Duemilanove, Diecimila, etc then the MOSI and CLK
// pins are 'fixed' in hardware. If you'rere using 'bitbang' (slower)
// interfacing, you can change any of these pins as deired.
#define TFT_MOSI  11		// SDI
#define TFT_CLK   13		// SCL
#define TFT_CS    10		// CS
#define TFT_RESET  9		// RESET

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	RED             0xF800
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF

#include <SPI.h>

// Option 1: use any pins but much slower
//Adafruit_HX8340B display(TFT_MOSI, TFT_CLK, TFT_RESET, TFT_CS);

// Option 2: must use the hardware SPI pins
// (for UNO thats sclk = 13 and sid = 11) and pin 10 must be
// an output. This is much faster - also required if you want
// to use the microSD card (see the image drawing example)
Adafruit_HX8340B display(TFT_RESET, TFT_CS);

float p = 3.1415926;

void tftPrintTest();
void testlines(uint16_t color);
void testfastlines(uint16_t color1, uint16_t color2);
void testdrawrects(uint16_t color);

void setup(void) {
    delay(1000);
    
    Serial.begin(9600);
    Serial.print("hello!");
    display.begin();
    
    Serial.println("init");
    uint16_t time = millis();
    display.fillScreen(BLACK);
    time = millis() - time;
    
    Serial.println("time:");
    Serial.println(time, DEC);
    delay(500);
    
    display.fillScreen(BLACK);
    display.setCursor(0,0);
    display.print("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa");
    delay(1000);
    
    // tft print function!
    Serial.println("tft test:");
    tftPrintTest();
    delay(2000);
    
    //a single pixel
    Serial.println("draw pixel:");
    display.drawPixel(display.width()/2, display.height()/2, GREEN);
    delay(500);
    
    // line draw test
    Serial.println("draw line:");
    testlines(YELLOW);
    delay(500);
    
    // optimized lines
    testfastlines(RED, BLUE);
    delay(500);
    
    testdrawrects(GREEN);
    delay(1000);
    
//    testfillrects(YELLOW, MAGENTA);
    delay(1000);
    
//    display.fillScreen(BLACK);
//    testfillcircles(10, BLUE);
//    testdrawcircles(10, WHITE);
//    delay(1000);
//    
//    testroundrects();
//    delay(500);
//    
//    testtriangles();
//    delay(500);
    
    Serial.println("done");
    delay(1000);
}

void loop() {
}

void testlines(uint16_t color) {
    display.fillScreen(BLACK);
    for (int16_t x=0; x < display.width()-1; x+=6) {
        display.drawLine(0, 0, x, display.height()-1, color);
    }
    for (int16_t y=0; y < display.height()-1; y+=6) {
        display.drawLine(0, 0, display.width()-1, y, color);
    }
    
    display.fillScreen(BLACK);
    for (int16_t x=0; x < display.width()-1; x+=6) {
        display.drawLine(display.width()-1, 0, x, display.height()-1, color);
    }
    for (int16_t y=0; y < display.height()-1; y+=6) {
        display.drawLine(display.width()-1, 0, 0, y, color);
    }
    
    display.fillScreen(BLACK);
    for (int16_t x=0; x < display.width()-1; x+=6) {
        display.drawLine(0, display.height()-1, x, 0, color);
    }
    for (int16_t y=0; y < display.height()-1; y+=6) {
        display.drawLine(0, display.height()-1, display.width()-1, y, color);
    }
    
    display.fillScreen(BLACK);
    for (int16_t x=0; x < display.width()-1; x+=6) {
        display.drawLine(display.width()-1, display.height()-1, x, 0, color);
    }
    for (int16_t y=0; y < display.height()-1; y+=6) {
        display.drawLine(display.width()-1, display.height()-1, 0, y, color);
    }
    
}

void testdrawtext(char *text, uint16_t color) {
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    
    for (uint8_t i=0; i < 168; i++) {
        if (i == '\n') continue;
        display.write(i);
        if ((i > 0) && (i % 21 == 0))
            display.println();
    }
}

void testfastlines(uint16_t color1, uint16_t color2) {
    display.fillScreen(BLACK);
    for (int16_t y=0; y < display.height()-1; y+=5) {
        display.drawFastHLine(0, y, display.width()-1, color1);
    }
    for (int16_t x=0; x < display.width()-1; x+=5) {
        display.drawFastVLine(x, 0, display.height()-1, color2);
    }
}

void testdrawrects(uint16_t color) {
    display.fillScreen(BLACK);
    for (int16_t x=0; x < display.height()-1; x+=6) {
        display.drawRect((display.width()-1)/2 -x/2, (display.height()-1)/2 -x/2 , x, x, color);
    }
}

void testfillrects(uint16_t color1, uint16_t color2) {
    display.fillScreen(BLACK);
    for (int16_t x=display.width()-1; x > 6; x-=6) {
        display.fillRect((display.width()-1)/2 -x/2, (display.height()-1)/2 -x/2 , x, x, color1);
        display.drawRect((display.width()-1)/2 -x/2, (display.height()-1)/2 -x/2 , x, x, color2);
    }
}

void testfillcircles(uint8_t radius, uint16_t color) {
    for (uint8_t x=radius; x < display.width()-1; x+=radius*2) {
        for (uint8_t y=radius; y < display.height()-1; y+=radius*2) {
            display.fillCircle(x, y, radius, color);
        }
    }
}

void testdrawcircles(uint8_t radius, uint16_t color) {
    for (int16_t x=0; x < display.width()-1+radius; x+=radius*2) {
        for (int16_t y=0; y < display.height()-1+radius; y+=radius*2) {
            display.drawCircle(x, y, radius, color);
        }
    }
}

void testtriangles() {
    display.fillScreen(BLACK);
    int color = 0xF800;
    int t;
    int w = display.width()/2;
    int x = display.height();
    int y = 0;
    int z = display.width();
    for(t = 0 ; t <= 15; t+=1) {
        display.drawTriangle(w, y, y, x, z, x, color);
        x-=4;
        y+=4;
        z-=4;
        color+=100;
    }
}

void testroundrects() {
    display.fillScreen(BLACK);
    int color = 100;
    int i;
    int t;
    for(t = 0 ; t <= 4; t+=1) {
        int x = 0;
        int y = 0;
        int w = display.width();
        int h = display.height();
        for(i = 0 ; i <= 24; i+=1) {
            display.drawRoundRect(x, y, w, h, 5, color);
            x+=2;
            y+=3;
            w-=4;
            h-=6;
            color+=1100;
        }
        color+=100;
    }
}

void tftPrintTest() {
    display.fillScreen(BLACK);
    display.setCursor(0, 5);
    display.setTextColor(RED);
    display.setTextSize(1);
    display.println("Hello World!");
    display.setTextColor(YELLOW, GREEN);
    display.setTextSize(2);
    display.println("Hello World!");
    display.setTextColor(BLUE);
    display.setTextSize(3);
    display.println(1234.567);
    delay(1500);
    display.setCursor(0, 5);
    display.fillScreen(BLACK);
    display.setTextColor(WHITE);
    display.setTextSize(0);
    display.println("Hello World!");
    display.setTextSize(1);
    display.setTextColor(GREEN);
    display.print(p, 5);
    display.println(" Want pi?");
    display.print(8675309, HEX); // print 8,675,309 out in HEX!
    display.println(" Print HEX");
    display.setTextColor(WHITE);
    display.println("Sketch has been running for: ");
    display.setTextColor(MAGENTA);
    display.print(millis() / 1000);
    display.setTextColor(WHITE);
    display.print(" seconds.");
}

void mediabuttons() {
    // play
    display.fillScreen(BLACK);
    display.fillRoundRect(25, 10, 78, 60, 8, WHITE);
    display.fillTriangle(42, 20, 42, 60, 90, 40, RED);
    delay(500);
    // pause
    display.fillRoundRect(25, 90, 78, 60, 8, WHITE);
    display.fillRoundRect(39, 98, 20, 45, 5, GREEN);
    display.fillRoundRect(69, 98, 20, 45, 5, GREEN);
    delay(500);
    // play color
    display.fillTriangle(42, 20, 42, 60, 90, 40, BLUE);
    delay(50);
    // pause color
    display.fillRoundRect(39, 98, 20, 45, 5, RED);
    display.fillRoundRect(69, 98, 20, 45, 5, RED);
    // play color
    display.fillTriangle(42, 20, 42, 60, 90, 40, GREEN);
}

/**************************************************************************/
/*!
 @brief  Renders a simple test pattern on the LCD
 */
/**************************************************************************/
void lcdTestPattern(void)
{
    uint32_t i,j;
    display.setWindow(0, 0, display.width()-1, display.height()-1);
    
    for(i=0;i<64;i++)
    {
        for(j=0;j<96;j++)
        {
            if(i>55){display.writeData(WHITE>>8);display.writeData(WHITE);}
            else if(i>47){display.writeData(BLUE>>8);display.writeData(BLUE);}
            else if(i>39){display.writeData(GREEN>>8);display.writeData(GREEN);}
            else if(i>31){display.writeData(CYAN>>8);display.writeData(CYAN);}
            else if(i>23){display.writeData(RED>>8);display.writeData(RED);}
            else if(i>15){display.writeData(MAGENTA>>8);display.writeData(MAGENTA);}
            else if(i>7){display.writeData(YELLOW>>8);display.writeData(YELLOW);}
            else {display.writeData(BLACK>>8);display.writeData(BLACK);}
        }
    }
}