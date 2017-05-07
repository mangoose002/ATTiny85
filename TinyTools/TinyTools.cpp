#include "TinyTools.h"

void TinyTools::setup_watchdog(int ii){
	byte bb;
	int ww;
	if (ii > 9 ) ii=9;
	  bb=ii & 7;
	if (ii > 7) bb|= (1<<5);
	  bb|= (1<<WDCE);
	ww=bb;

	MCUSR &= ~(1<<WDRF);
	// start timed sequence
	WDTCR |= (1<<WDCE) | (1<<WDE);   
	// set new watchdog timeout value   
	WDTCR = bb;   WDTCR |= _BV(WDIE); 
}


void TinyTools::flashLED(byte led_pin, uint16_t repetitions, uint16_t duration){
  pinMode(led_pin, OUTPUT);
  for (byte i = repetitions ;  i > 0 ; i--){
     digitalWrite(led_pin, HIGH);
     delay(duration);
     digitalWrite(led_pin, LOW); 
     delay(duration);
  }
  pinMode(led_pin, INPUT); // reduce power  
}

//Reads address from DIP switch and resistor array
byte TinyTools::readAddress(byte address_pin){
  byte address;

  sbi(ADCSRA,ADEN);                         // switch Analog to Digitalconverter ON
  delay(5);                                 // Wait for ADC to settle
  uint16_t val = analogRead(address_pin);   // Reads value for address
  cbi(ADCSRA,ADEN);                         // switch Analog to Digitalconverter OFF  
  if(val > 1020)
    address = 0x0;
  else if(val >= 927 && val < 935)
    address = 0x1;
  else if(val >=867 && val < 877)
    address = 0x2;
  else if(val >=792 && val < 810)
    address = 0x3;
  else if(val >=758 && val < 771)
    address = 0x4;
  else if(val >=698 && val < 718)
    address = 0x5;
  else if(val >=665 && val < 684)
    address = 0x6;
  else if(val >=646 && val < 661)
    address = 0x8;
  else if(val >=625 && val < 642)
    address = 0x7;
  else if(val >=602 && val < 622)
    address = 0x9;
  else if(val >=578 && val < 596)
    address = 0xA;
  else if(val >=548 && val < 568)
    address = 0xB;
  else if(val >=529 && val < 545)
    address = 0xC;
  else if(val >=502 && val < 520)
    address = 0xD;
  else if(val >=481 && val < 500)
    address = 0xE;
  else if(val >=455 && val < 478)
    address = 0xF;

 return address;
}

uint16_t TinyTools::readVcc(void) {
  uint16_t result;

  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
  // Read 1.1V reference against Vcc
  ADMUX = (0<<REFS0) | (12<<MUX0);
  delay(2); // Wait for Vref to settle
  ADCSRA |= (1<<ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCW;
  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF  

  return 1126400L / result; // Back-calculate AVcc in mV
}