//#define DEBUG
#define DHTMODEL dht22
#define IDHEX    0xA5

#ifndef DHTMODEL
#define DHTMODEL dht22
#endif
/*
 * connectingStuff, Oregon Scientific v2.1 Emitter
 * http://connectingstuff.net/blog/encodage-protocoles-oregon-scientific-sur-arduino/
 *
 * Copyright (C) 2013 olivier.lebrun@gmail.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
/*
                           +-\/-+
          Ain0 (D 5) PB5  1|    |8  Vcc
          Ain3 (D 3) PB3  2|    |7  PB2 (D 2) Ain1 -
 LED +pin Ain2 (D 4) PB4  3|    |6  PB1 (D 1) pwm1 - DHT22 sensor pin
                     GND  4|    |5  PB0 (D 0) pwm0 - RF433 tx pin

Modified by Pierre LENA
=> Merge all Oregon function in a OregonV2 class
=> VCC read
=> Led alarm
=> Only enable ADC converter on VCC read to reduce power consumption
*/

#include <OregonV2.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include <dht11.h>
#include <dht22.h>

#ifdef DEBUG
#include <SoftwareSerial.h>
#endif

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define TX_PIN  PB0
#define DHT_PIN PB1
#define LED_PIN PB5
#define SRX_PIN PB3
#define STX_PIN PB4

#define WDT_VCC_COUNT         75   // wdt is set to 8s, 8x75=600 seconds
#define WDT_COUNT             7    // wdt is set to 8s, 8x7=56 seconds
#define LOW_BATTERY_LEVEL     2600 //mV. Led will flash if battery level is lower than this value

#ifdef DEBUG
SoftwareSerial   TinySerial(SRX_PIN, STX_PIN);
#endif
OregonV2         Radio(TX_PIN);

DHTMODEL         dht;
volatile boolean lowBattery   = false;
volatile byte    count        = WDT_COUNT;
volatile byte    batteryCount = 0;

//Reads Vcc and return the value in mV
uint16_t readVcc(void) {
  uint16_t result;

  //sbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter ON
  // Read 1.1V reference against Vcc
  ADMUX = (0<<REFS0) | (12<<MUX0);
  delay(2); // Wait for Vref to settle
  ADCSRA |= (1<<ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCW;
  //cbi(ADCSRA,ADEN);                    // switch Analog to Digitalconverter OFF  

  #ifdef DEBUG
  TinySerial.print(1018500L / result); TinySerial.println(" mV");
  #endif
  
  return 1018500L / result; // Back-calculate AVcc in mV
}

//Use to alert of low battery
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

// set system into the sleep state
// system wakes up when wtchdog is timed out
void system_sleep() {
  cbi(ADCSRA, ADEN);                   // switch Analog to Digitalconverter OFF

  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();

  sleep_mode();                        // System sleeps here

  sleep_disable();                     // System continues execution here when watchdog timed out
  sbi(ADCSRA, ADEN);                   // switch Analog to Digitalconverter ON
}

// Watchdog Interrupt Service / is executed when watchdog timed out
ISR(WDT_vect) {
  batteryCount--;
  count--;
}

// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii) {

  byte bb;
  int ww;
  if (ii > 9 ) ii = 9;
  bb = ii & 7;
  if (ii > 7) bb |= (1 << 5);
  bb |= (1 << WDCE);
  ww = bb;

  MCUSR &= ~(1 << WDRF);
  // start timed sequence
  WDTCR |= (1 << WDCE) | (1 << WDE);
  // set new watchdog timeout value
  WDTCR = bb;
  WDTCR |= _BV(WDIE);
}

/******************************************************************/
void setup(){
  //Force 8MHz CPU
  cli(); // Disable interrupts
  CLKPR = 0x80; // Prescaler enable
  CLKPR = 0x00; // Clock division factor 1
  sei(); // Enable interrupts

  lowBattery = !(readVcc() >= LOW_BATTERY_LEVEL); // Initialize battery level value
  setup_watchdog(9);                              //Set to 8s
  digitalWrite(TX_PIN,LOW);
  Radio.setId(IDHEX); //Set ID

  #ifdef DEBUG
  TinySerial.begin(9600);
  TinySerial.println("\n[Oregon V2.1 encoder]");
  #endif
}

void loop(){
  if(batteryCount == 0){ //We only read VCC every 10 minutes
    batteryCount = WDT_VCC_COUNT;
    lowBattery = !(readVcc() >= LOW_BATTERY_LEVEL); // Initialize battery level value
  }

  if(count == 0){
    count = WDT_COUNT;
    if(lowBattery){
      lowBatteryWarning(); //Blinking led
    }

    int chk = dht.read(DHT_PIN);
    if(chk == DHTLIB_OK){
      #ifdef DEBUG
        TinySerial.print("Temperature : "); TinySerial.print(dht.temperature); TinySerial.write(176); // caractÃƒÂ¨re Ã‚Â°
        TinySerial.write('C'); TinySerial.println();
        TinySerial.print("Humidity : "); TinySerial.print(dht.humidity);
        TinySerial.write('%'); TinySerial.println();
      #endif
        Radio.send(dht.temperature,dht.humidity,!lowBattery);
    } else if(chk == DHTLIB_ERROR_CHECKSUM){
      #ifdef DEBUG
        TinySerial.println("Checksum error");
      #endif
    } else if(chk == DHTLIB_ERROR_TIMEOUT){
      #ifdef DEBUG
        TinySerial.println("Time out error");
      #endif
    } else {
      #ifdef DEBUG
        TinySerial.println("Unknown error");
      #endif
    }
  }
  system_sleep();
}
