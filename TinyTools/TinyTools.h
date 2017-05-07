#ifndef _TINYTOOLS_H_
#define _TINYTOOLS_H_

#if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
#elif defined(ENERGIA) // LaunchPad, FraunchPad and StellarPad specific
    #include "Energia.h"
#else
    #include "WProgram.h"
#endif

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

class TinyTools{

public:
	static void setup_watchdog(int);
	static byte readAddress(byte address_pin);
	static uint16_t readVcc();
	static void flashLED(byte led_pin, uint16_t repetitions, uint16_t duration);
};

#endif