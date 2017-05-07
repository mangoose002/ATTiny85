//#define DEBUG //For 123d.circuits.io
//#define ADDRESS 15
//
//
//
//                           +-\/-+
//          Ain0 (D 5) PB5  1|    |8  Vcc
//Address reader (D 3) PB3  2|    |7  PB2 (D 2) Ain1 -
// LED +pin Ain2 (D 4) PB4  3|    |6  PB1 (D 1) pwm1 - Reed sensor pin
//                     GND  4|    |5  PB0 (D 0) pwm0 - RF433 tx pin
// 
// **** INCLUDES *****
#include <TinyTools.h>

#include <avr/sleep.h>
#include <avr/wdt.h>

#ifndef DEBUG
#include <RCSwitch.h>
#endif

#define LOW_BATTERY_LEVEL 3200 //mV. Led will flash if battery level is lower than this value
#define WDT_COUNT         75   // wdt is set to 8s, 8x75=600 seconds
#define LED_WDT_COUNT     5    // wdt is set to 8s, 8x5=40 seconds

volatile boolean state = false;
volatile boolean f_wdt = 0;
volatile boolean f_led_wdt = 0;
volatile byte    count = WDT_COUNT;
volatile byte    led_count = LED_WDT_COUNT;
volatile boolean f_int = 0;
volatile boolean lowBattery = 0;
volatile byte    address = 0;
volatile byte    group;
volatile byte    sw;

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
  
  TinyTools::setup_watchdog(9);                 // 8 seconds
  pinMode(LED_PIN,OUTPUT);           // For Battery level notification
  pinMode(ADDRESS_PIN,INPUT);        // For address reading
  pinMode(TX_PIN, OUTPUT);           // For radio
  pinMode(WAKEUP_PIN, INPUT);        // Set the pin to input
  digitalWrite(WAKEUP_PIN, HIGH);    // Activate internal pullup resistor

  #ifndef DEBUG
  mySwitch.enableTransmit(TX_PIN);
  #endif
 
  lowBattery = !(TinyTools::readVcc() >= LOW_BATTERY_LEVEL); // Initialize battery level value
  
  #ifndef ADDRESS
  address    = TinyTools::readAddress(ADDRESS_PIN);   //Between 0 and 15
  #else
  address    = ADDRESS;
  #endif
  
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
  cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF
 
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
 
  sleep_mode();                        // System sleeps here
 
  sleep_disable();                     // System continues execution here when watchdog timed out 
  sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
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
  TinyTools::flashLED(LED_PIN,2,25);
    
  #ifndef DEBUG  
  if(state)
      mySwitch.switchOn('c',group,sw);       // Zibase signal id
  else
      mySwitch.switchOff('c',group,sw);      // Zibase signal id
  #endif
}

void loop(){
  system_sleep();
  
  if(f_led_wdt && lowBattery){
    TinyTools::flashLED(LED_PIN,5,250);
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
    lowBattery = !(TinyTools::readVcc() >= LOW_BATTERY_LEVEL);
    ReadAndSend();
    f_wdt=false; // reset WDT Flag
    sei(); // enable interrupts
  }
}

