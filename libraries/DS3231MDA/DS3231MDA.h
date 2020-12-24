// Modified by Andy Wickert 5/15/11: Spliced in stuff from RTClib

#ifndef DS3231MDA_h
#define DS3231MDA_h

#include <Arduino.h>
#include <Wire.h>
                                    

class DS3231MDA {
	public:			
		//Constructor
		DS3231MDA();    
		// the get*() functions retrieve current values of the registers.
                bool getNow(byte &y, byte &m, byte &d, byte &hh, byte &mm, byte &ss);
		void setSecond(byte Second); 
		void setMinute(byte Minute); 
		void setHour(byte Hour); 
		void setDoW(byte DoW); 
		void setDate(byte Date); 
		void setMonth(byte Month); 
		void setYear(byte Year); 
		void setClockMode(bool h12); 
		float getTemperature(); 
                uint16_t date2days(uint16_t y, uint8_t m, uint8_t d);
                long time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s);

	private:

		byte decToBcd(byte val); 
			// Convert normal decimal numbers to binary coded decimal
		byte bcdToDec(byte val); 
			// Convert binary coded decimal to normal decimal numbers

                uint8_t bin2bcd(uint8_t val);
                uint8_t bcd2bin(uint8_t val);

		byte readControlByte(bool which); 
			// Read selected control byte: (0); reads 0x0e, (1) reads 0x0f
		void writeControlByte(byte control, bool which); 
			// Write the selected control byte. 
			// which == false -> 0x0e, true->0x0f.

};

#endif
