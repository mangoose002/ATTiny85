//#define DEBUG //For 123d.circuits.io
//
//
//
//                           +-\/-+
//          Ain0 (D 5) PB5  1|    |8  Vcc
//          Ain3 (D 3) PB3  2|    |7  PB2 (D 2) Ain1 -
// LED +pin Ain2 (D 4) PB4  3|    |6  PB1 (D 1) pwm1 - Shock sensor pin
//                     GND  4|    |5  PB0 (D 0) pwm0 - RF433 tx pin
// 
// **** INCLUDES *****
#include <avr/sleep.h>
#include <avr/wdt.h>
#ifndef DEBUG
#include <RCSwitch.h>
#endif

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
 
#define LOW_BATTERY_LEVEL 2600 //mV. Led will flash if battery level is lower than this value
#define WDT_COUNT         75   // wdt is set to 8s, 8x75=600 seconds
#define LED_WDT_COUNT     5    // wdt is set to 8s, 8x5=40 seconds

volatile boolean state = false;
volatile boolean f_wdt = 0;
volatile boolean f_led_wdt = 0;
volatile byte count = WDT_COUNT;
volatile byte led_count = LED_WDT_COUNT;
volatile boolean f_int = 0;
volatile boolean lowBattery = 0;
volatile byte address = 0;
volatile byte group;
volatile byte sw;

const byte TX_PIN      = PB0;  // Pin number for the 433mhz OOK transmitter
const byte LED_PIN     = PB4;  // For battery level indication
const byte WAKEUP_PIN  = PB1;  // Use pin PB1 as wake up pin
const byte ADDRESS_PIN = PB3;  // For address reading through DIP switches

#ifndef DEBUG
RCSwitch mySwitch = RCSwitch();
#endif

/******************************************************************/
 
void setup()
{
  cli(); // Disable interrupts
  //Force 8MHz CPU
  CLKPR = 0x80; // Prescaler enable
  CLKPR = 0x00; // Clock division factor 1
  sei(); // Enable interrupts
  
  setup_watchdog(9);                 // 8 seconds
  pinMode(LED_PIN,OUTPUT);           // For Battery level notification
  pinMode(ADDRESS_PIN,INPUT);        // For address reading
  pinMode(TX_PIN, OUTPUT);           // For radio
  pinMode(WAKEUP_PIN, INPUT);        // Set the pin to input
  digitalWrite(WAKEUP_PIN, HIGH);    // Activate internal pullup resistor

  #ifndef DEBUG
  mySwitch.enableTransmit(TX_PIN);
  #endif
 
  lowBattery = !(readVcc() >= LOW_BATTERY_LEVEL); // Initialize battery level value
  address    = readAddress();   //Between 0 and 15
  sw         = 1 + address % 4; //Between 1 and 4
  group      = 1 + address / 4; //Between 1 and 4
 
  PCMSK  |= bit (PCINT1);  // set pin change interrupt PB1
  GIFR   |= bit (PCIF);    // clear any outstanding interrupts
  GIMSK  |= bit (PCIE);    // enable pin change interrupts  
  cbi(ADCSRA,ADEN);        // switch Analog to Digitalconverter OFF
  sei();

  ReadAndSend();
}
 
 
// set system into the sleep state 
// system wakes up when watchdog is timed out
void system_sleep() {
  #ifndef DEBUG
  //cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF
 
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
 
  sleep_mode();                        // System sleeps here
 
  sleep_disable();                     // System continues execution here when watchdog timed out 
  //sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
  #else
    delay(8000);
  #endif
  if (count >= WDT_COUNT) {
   f_wdt=true;  // set WDTl flag
   count=0;
  }
  if (led_count >= LED_WDT_COUNT) {
   f_led_wdt=true;  // set WDTl flag
   led_count=0;
  }
  count++;
  led_count++;
}
 
// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9=8sec
void setup_watchdog(int ii) {
 
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
 
// Watchdog Interrupt Service / is executed when watchdog timed out 
ISR(WDT_vect) {   
  if (count >= WDT_COUNT) {
   f_wdt=true;  // set WDTl flag
   count=0;
  }
  if (led_count >= LED_WDT_COUNT) {
   f_led_wdt=true;  // set WDTl flag
   led_count=0;
  }
  count++;
  led_count++;
} 
 
ISR (PCINT0_vect){  
  f_int=true; // set INT flag
}

void ReadAndSend(){
  state = digitalRead(WAKEUP_PIN);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  delay(250);
  digitalWrite(LED_PIN, LOW); 
  pinMode(LED_PIN, INPUT); // reduce power
  
  #ifndef DEBUG  
  if(state)
      mySwitch.switchOn('c',group,sw);       // Zibase signal id
  else
      mySwitch.switchOff('c',group,sw);      // Zibase signal id
  #endif
}

void loop()
{
  system_sleep();
  
  if(f_led_wdt && lowBattery){
    lowBatteryWarning();
    f_led_wdt = false; // Reset LED Flag
  }

  if (f_int) {
    cli();
    ReadAndSend();
    f_int=false;  // Reset INT Flag
    sei();
  } 
  else if ( f_wdt ) {
    cli(); // disable interrupts
    lowBattery = !(readVcc() >= LOW_BATTERY_LEVEL);
    ReadAndSend();
    f_wdt=false; // reset WDT Flag
    sei(); // enable interrupts
  }
}

void lowBatteryWarning () {
  pinMode(LED_PIN, OUTPUT);
  for (byte i = 5 ;  i > 0 ; i--){
     digitalWrite(LED_PIN, HIGH);
     delay(250);
     digitalWrite(LED_PIN, LOW); 
     delay(250);
  }
  pinMode(LED_PIN, INPUT); // reduce power  
}

uint16_t readVcc(void) {
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

//Reads address from DIP switch and resistor array
byte readAddress(void){
  byte address = 0x2;  //For previous version consistency

  return address; //Before installing a real DIP switch

  sbi(ADCSRA,ADEN);                         // switch Analog to Digitalconverter ON
  delay(2);                                 // Wait for ADC to settle
  uint16_t val = analogRead(ADDRESS_PIN);   // Reads value for address
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

