#ifndef __OregonV2__H_
#define __OregonV2__H_

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#elif defined(ENERGIA) // LaunchPad, FraunchPad and StellarPad specific
#include "Energia.h"	
#else
#include "WProgram.h"
#endif

class OregonV2{
	public:
		OregonV2(int);
		void setType(byte* type);
		void setChannel(byte channel);
		void setId(byte ID);
		void send(float temp, byte hum, byte level);

	private:
		int txpin;		//For Radio transmission
		// Buffer for Oregon message
#ifdef THN132N
		byte OregonMessageBuffer[8];
#else
		byte OregonMessageBuffer[9];
#endif

		const unsigned long TIME    = 512;
		const unsigned long TWOTIME = TIME * 2;

		void sendZero(void);
		void sendOne(void);
		void sendQuarterMSB(const byte data);
		void sendQuarterLSB(const byte data);
		void sendData(byte *data, byte size);
		void sendOregon();
		void sendPreamble(void);
		void sendPostamble(void);
		void sendSync(void);

		int Sum(byte count);
		void calculateAndSetChecksum();
		void setBatteryLevel(byte level);
		void setTemperature(float temp);
		void setHumidity(byte hum);
};

#endif
