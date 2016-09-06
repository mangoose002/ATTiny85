#include <OregonV2.h>

OregonV2::OregonV2(int _txpin){
	this->txpin = _txpin;
	pinMode(this->txpin,OUTPUT);

	#ifdef THN132N
  	// Create the Oregon message for a temperature only sensor (TNHN132N)
  	byte ID[] = {0xEA, 0x4C};
	#else
  	// Create the Oregon message for a temperature/humidity sensor (THGR2228N)
  	byte ID[] = {0x1A, 0x2D};
	#endif

  	for(int i=0;i<sizeof(this->OregonMessageBuffer);i++){
  		this->OregonMessageBuffer[i] = 0;
  	}

  	//Set default values
  	setType(ID);
  	setChannel(0x20);
  	setId(0xEF);
}

void OregonV2::send(float temp,byte hum,byte level){
	for(int i=4;i<sizeof(this->OregonMessageBuffer);i++){
  		this->OregonMessageBuffer[i] = 0;
  	}

	setBatteryLevel(level);
	setTemperature(temp);
	#ifndef THN132N
	setHumidity(hum);
	#endif

    calculateAndSetChecksum();	// Calculate the checksum
    sendOregon();				// Send the Message over RF

    digitalWrite(this->txpin, LOW);					// Send a "pause"
    delayMicroseconds(this->TWOTIME * 8);

    sendOregon();				// Send a copie of the first message. The v2.1 protocol send the message twice
    digitalWrite(this->txpin, LOW);
}

/******************************************************************/
/******************************************************************/
/******************************************************************/
/**
 * \brief    Set the sensor type
 * \param    data       Oregon message
 * \param    type       Sensor type
 */
void OregonV2::setType(byte* type){
	this->OregonMessageBuffer[0] = type[0];
  	this->OregonMessageBuffer[1] = type[1];
}


/**
 * \brief    Set the sensor channel
 * \param    data       Oregon message
 * \param    channel    Sensor channel (0x10, 0x20, 0x30)
 */
void OregonV2::setChannel(byte channel){
  this->OregonMessageBuffer[2] = channel;
}

/**
 * \brief    Set the sensor ID
 * \param    data       Oregon message
 * \param    ID         Sensor unique ID
 */
void OregonV2::setId(byte ID){
  this->OregonMessageBuffer[3] = ID;
}

/**
 * \brief    Set the sensor battery level
 * \param    data       Oregon message
 * \param    level      Battery level (0 = low, 1 = high)
 */
void OregonV2::setBatteryLevel(byte level){
  if (!level) 
  	this->OregonMessageBuffer[4] = 0x0C;
  else 
  	this->OregonMessageBuffer[4] = 0x00;
}

/**
 * \brief    Set the sensor temperature
 * \param    data       Oregon message
 * \param    temp       the temperature
 */
void OregonV2::setTemperature(float temp){
  // Set temperature sign
  if (temp < 0){
    this->OregonMessageBuffer[6] = 0x08;
    temp *= -1;
  }  else  {
    this->OregonMessageBuffer[6] = 0x00;
  }

  // Determine decimal and float part
  int tempInt = (int)temp;
  int td = (int)(tempInt / 10);
  int tf = (int)round((float)((float)tempInt / 10 - (float)td) * 10);

  int tempFloat =  (int)round((float)(temp - (float)tempInt) * 10);

  // Set temperature decimal part
  this->OregonMessageBuffer[5] = (td << 4);
  this->OregonMessageBuffer[5] |= tf;

  // Set temperature float part
  this->OregonMessageBuffer[4] |= (tempFloat << 4);
}

/**
 * \brief    Set the sensor humidity
 * \param    data       Oregon message
 * \param    hum        the humidity
 */
void OregonV2::setHumidity(byte hum){
  this->OregonMessageBuffer[7] = (hum / 10);
  this->OregonMessageBuffer[6] |= (hum - this->OregonMessageBuffer[7] * 10) << 4;
}



/**
 * \brief    Send logical "0" over RF
 * \details  azero bit be represented by an off-to-on transition
 * \         of the RF signal at the middle of a clock period.
 * \         Remenber, the Oregon v2.1 protocol add an inverted bit first
 */
void OregonV2::sendZero(void){
  digitalWrite(this->txpin, HIGH);
  delayMicroseconds(this->TIME);
  digitalWrite(this->txpin, LOW);
  delayMicroseconds(this->TWOTIME);
  digitalWrite(this->txpin, HIGH);
  delayMicroseconds(this->TIME);
}

/**
 * \brief    Send logical "1" over RF
 * \details  a one bit be represented by an on-to-off transition
 * \         of the RF signal at the middle of a clock period.
 * \         Remenber, the Oregon v2.1 protocol add an inverted bit first
 */
void OregonV2::sendOne(void){
  digitalWrite(this->txpin, LOW);
  delayMicroseconds(this->TIME);
  digitalWrite(this->txpin, HIGH);
  delayMicroseconds(this->TWOTIME);
  digitalWrite(this->txpin, LOW);
  delayMicroseconds(this->TIME);
}

/**
* Send a bits quarter (4 bits = MSB from 8 bits value) over RF
*
* @param data Source data to process and sent
*/

/**
 * \brief    Send a bits quarter (4 bits = MSB from 8 bits value) over RF
 * \param    data   Data to send
 */
void OregonV2::sendQuarterMSB(const byte data){
  for (int i = 4; i <= 7; bitRead(data, i++) ? sendOne() : sendZero());
}

/**
 * \brief    Send a bits quarter (4 bits = LSB from 8 bits value) over RF
 * \param    data   Data to send
 */
void OregonV2::sendQuarterLSB(const byte data){
  for (int i = 0; i <= 3; bitRead(data, i++) ? sendOne() : sendZero());
}

/******************************************************************/
/******************************************************************/
/******************************************************************/

/**
 * \brief    Send a buffer over RF
 * \param    data   Data to send
 * \param    size   size of data to send
 */
void OregonV2::sendData(byte *data, byte size){
  for (byte i = 0; i < size; ++i){
    sendQuarterLSB(data[i]);
    sendQuarterMSB(data[i]);
  }
}

/**
 * \brief    Send an Oregon message
 * \param    data   The Oregon message
 */
void OregonV2::sendOregon(){
  sendPreamble();
  sendData(this->OregonMessageBuffer, sizeof(this->OregonMessageBuffer));
  sendPostamble();
}

/**
 * \brief    Send preamble
 * \details  The preamble consists of 16 "1" bits
 */
void OregonV2::sendPreamble(void){
  byte PREAMBLE[] = {0xFF, 0xFF};
  sendData(PREAMBLE, 2);
}

/**
 * \brief    Send postamble
 * \details  The postamble consists of 8 "0" bits
 */
void OregonV2::sendPostamble(void){
  byte POSTAMBLE[] = {0x00};
  sendData(POSTAMBLE, 1);
}

/**
 * \brief    Send sync nibble
 * \details  The sync is 0xA. It is not use in this version since the sync nibble
 * \         is include in the Oregon message to send.
 */
void OregonV2::sendSync(void){
  sendQuarterLSB(0xA);
}

/**
 * \brief    Sum data for checksum
 * \param    count      number of bit to sum
 * \param    data       Oregon message
 */
int OregonV2::Sum(byte count){
  int s = 0;

  for (byte i = 0; i < count; i++){
    s += (this->OregonMessageBuffer[i] & 0xF0) >> 4;
    s += (this->OregonMessageBuffer[i] & 0xF);
  }

  if (int(count) != count)
    s += (this->OregonMessageBuffer[count] & 0xF0) >> 4;

  return s;
}

/**
 * \brief    Calculate checksum
 * \param    data       Oregon message
 */
void OregonV2::calculateAndSetChecksum(){
#ifdef THN132N
  int s = ((Sum(6) + (this->OregonMessageBuffer[6] & 0xF) - 0xa) & 0xff);
  this->OregonMessageBuffer[6] |=  (s & 0x0F) << 4;     this->OregonMessageBuffer[7] =  (s & 0xF0) >> 4;
#else
  this->OregonMessageBuffer[8] = ((Sum(8) - 0xa) & 0xFF);
#endif
}