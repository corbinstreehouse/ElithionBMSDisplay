//
//  main.cpp
//  Main file
//  ----------------------------------
//  Developed with embedXcode
//
//  Project BMSDisplay
//  Created by corbin dunn on 6/13/12 
//  Copyright (c) 2012 __MyCompanyName__
//

// Core library
#include  "Arduino.h" // — for Arduino 1.0

// Sketch
#include "BMSDisplay.pde"

int main(void)
{
 #if defined(__AVR_ATmega644P__) // Wiring specific
    boardInit();
#else    
    init();
#endif

    setup();
    for (;;) loop();
    return 0;
}
