/*                           
                           +-\/-+
          Ain0 (D 5) PB5  1|    |8  Vcc
 ECHO     Ain3 (D 3) PB3  2|    |7  PB2 (D 2) Ain1 -
 TRIGGER  Ain2 (D 4) PB4  3|    |6  PB1 (D 1) pwm1 -
                     GND  4|    |5  PB0 (D 0) pwm0 - RF433 tx pin
*/

#include <x10rf.h>
#include <Ultrasonic.h>

#define TX_PIN  PB0
#define ECHO_PIN PB3
#define TRIGGER_PIN PB4

#define HEIGHT 250
#define WIDTH  95
#define DEPTH  127

#define ID 12

long  measure;
x10rf myx10 = x10rf(TX_PIN,0,5);
Ultrasonic ultrasonic(TRIGGER_PIN,ECHO_PIN); // (Trig PIN,Echo PIN)

/******************************************************************/
void setup(){
  myx10.begin();
}

void loop(){
  measure = (HEIGHT * WIDTH * (DEPTH - ultrasonic.Ranging(CM))) / 1000;
  myx10.RFXmeter(ID,0,measure);
  
  delay(15000);
}
