//#define DEBUG
#define DHTMODEL dht22
#define IDHEX    0xF0

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

#include <TinyTools.h>
#include <OregonV2.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include <dht11.h>
#include <dht22.h>

#ifdef DEBUG
#include <SoftwareSerial.h>
#endif

#define TX_PIN  PB0
#define DHT_PIN PB1
#define LED_PIN PB5
#define SRX_PIN PB3
#define STX_PIN PB4

#define WDT_RST_COUNT         450  // wdt is set to 8s, 450*8=3600 seconds
#define WDT_VCC_COUNT         75   // wdt is set to 8s, 8x75=600 seconds
#define WDT_COUNT             7    // wdt is set to 8s, 8x7=56 seconds
#define LOW_BATTERY_LEVEL     3200 //mV. Led will flash if battery level is lower than this value

#ifdef DEBUG
SoftwareSerial   TinySerial(SRX_PIN, STX_PIN);
#endif
OregonV2         Radio(TX_PIN);

DHTMODEL          dht;
volatile boolean  lowBattery;
volatile byte     count;
volatile byte     batteryCount;
volatile uint16_t resetCount;

void(*Reboot)(void)=0;             // reset function on adres 0.

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
  resetCount--,
  batteryCount--;
  count--;
}

/******************************************************************/
void setup(){
  //Force 8MHz CPU
  cli(); // Disable interrupts
  CLKPR = 0x80; // Prescaler enable
  delay(5);
  CLKPR = 0x00; // Clock division factor 1
  sei(); // Enable interrupts

  lowBattery = !(TinyTools::readVcc() >= LOW_BATTERY_LEVEL); // Initialize battery level value
  TinyTools::setup_watchdog(9);                              //Set to 8s
  digitalWrite(TX_PIN,LOW);
  Radio.setId(IDHEX); //Set ID

  lowBattery   = false;
  count        = WDT_COUNT;
  batteryCount = 0;
  resetCount   = WDT_RST_COUNT;

  #ifdef DEBUG
  TinySerial.begin(9600);
  TinySerial.println("\n[Oregon V2.1 encoder]");
  #endif
}

void loop(){
  if(resetCount == 0){
    resetCount = WDT_RST_COUNT;
    Reboot(); //Reboot every 60 minutes
  }

  if(batteryCount == 0){ //We only read VCC every 10 minutes
    batteryCount = WDT_VCC_COUNT;
    lowBattery = !(TinyTools::readVcc() >= LOW_BATTERY_LEVEL); // Initialize battery level value
  }

  if(count == 0){
    count = WDT_COUNT;
    if(lowBattery){
      TinyTools::flashLED(LED_PIN,5,250); //Blinking led
    }

    float h = 0;
    float t = 0;
    int   c = 0;
    int chk;

    for(int i=0;i<3;i++){
      chk = dht.read(DHT_PIN);
      if(chk == DHTLIB_OK && dht.humidity > 5){
        t+= dht.temperature;
        h+= dht.humidity;
        c++;
      } 
      #ifdef DEBUG
        else if(chk == DHTLIB_ERROR_CHECKSUM){
        TinySerial.println("Checksum error");
      } else if(chk == DHTLIB_ERROR_TIMEOUT){
        TinySerial.println("Time out error");
      } else {
        TinySerial.println("Unknown error");
      }
      #endif
      delay(200);
    }
    
    if(c > 1)
      t = t / c;
      h = h / c;

      if(h > 5){ //It is a nonsense to have humidity less that 5%
        #ifdef DEBUG
          TinySerial.print("Temperature : "); TinySerial.print(t); TinySerial.write(176); // caractère °
          TinySerial.write('C'); TinySerial.println();
          TinySerial.print("Humidity : "); TinySerial.print(h);
          TinySerial.write('%'); TinySerial.println();
        #endif
        Radio.send(t,h,!lowBattery);
      }
    }
    
    system_sleep();
}
