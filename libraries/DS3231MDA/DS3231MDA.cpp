/*
DS3231.cpp: DS3231 Real-Time Clock library
Eric Ayars
4/1/11

Spliced in DateTime all-at-once reading (to avoid rollover) and unix time
from Jean-Claude Wippler and Limor Fried
Andy Wickert
5/15/11

Released into the public domain.
*/

#include <DS3231MDA.h>
#include <avr/pgmspace.h>
#include <Arduino.h>
#include <Wire.h>

#define CLOCK_ADDRESS 0x68

static const uint8_t daysInMonth [] PROGMEM = { 31,28,31,30,31,30,31,31,30,31,30,31 };



// Constructor
DS3231MDA::DS3231MDA() {
	// nothing to do for this constructor.
}

uint8_t DS3231MDA::bcd2bin(uint8_t val) { 
 return val - 6 * (val >> 4); 
}

uint8_t DS3231MDA::bin2bcd(uint8_t val) { 
 return val + 6 * (val / 10); 
}

// number of days since 2000/01/01, valid for 2001..2099
uint16_t DS3231MDA::date2days(uint16_t y, uint8_t m, uint8_t d) {
    if (y >= 2000)
        y -= 2000;
    uint16_t days = d;
    for (uint8_t i = 1; i < m; ++i)
        days += pgm_read_byte(daysInMonth + i - 1);
    if (m > 2 && y % 4 == 0)
        ++days;
    return days + 365 * y + (y + 3) / 4 - 1;
}
 

long DS3231MDA::time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s) {
    return ((days * 24L + h) * 60 + m) * 60 + s;
}


///// ERIC'S ORIGINAL CODE FOLLOWS /////

bool DS3231MDA::getNow(byte &y, byte &m, byte &d, byte &hh, byte &mm, byte &ss) {
  volatile unsigned long vAttempt=0;
  bool hasError=false;
  

  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(0);	         // This is the first register address (Seconds)  			// We'll read from here on for 7 bytes: secs reg, minutes reg, hours, days, months and years.
  Wire.endTransmission();

  Wire.requestFrom(CLOCK_ADDRESS, 7);

  vAttempt=0;  
  do {
    vAttempt++;  
  } while (hasError=!Wire.available() && vAttempt<1000);

  if (hasError) {
    return false;
  };

  ss = bcd2bin(Wire.read() & 0x7F);
  mm = bcd2bin(Wire.read());
  hh = bcd2bin(Wire.read());
  Wire.read();
  d = bcd2bin(Wire.read());
  m = bcd2bin(Wire.read());
  y = bcd2bin(Wire.read());


  return true;
}

void DS3231MDA::setSecond(byte Second) {
	// Sets the seconds 
	// This function also resets the Oscillator Stop Flag, which is set
	// whenever power is interrupted.
	Wire.beginTransmission(CLOCK_ADDRESS);
	Wire.write(0x00);
	Wire.write(decToBcd(Second));	
	Wire.endTransmission();
	// Clear OSF flag
	byte temp_buffer = readControlByte(1);
	writeControlByte((temp_buffer & 0b01111111), 1);
}


void DS3231MDA::setMinute(byte Minute) {
	// Sets the minutes 
	Wire.beginTransmission(CLOCK_ADDRESS);
	Wire.write(0x01);
	Wire.write(decToBcd(Minute));	
	Wire.endTransmission();
}

void DS3231MDA::setHour(byte Hour) {
	// Sets the hour, without changing 12/24h mode.
	// The hour must be in 24h format.

	bool h12;

	// Start by figuring out what the 12/24 mode is
	Wire.beginTransmission(CLOCK_ADDRESS);
	Wire.write(0x02);
	Wire.endTransmission();
	Wire.requestFrom(CLOCK_ADDRESS, 1);
	h12 = (Wire.read() & 0b01000000);
	// if h12 is true, it's 12h mode; false is 24h.

	if (h12) {
		// 12 hour
		if (Hour > 12) {
			Hour = decToBcd(Hour-12) | 0b01100000;
		} else {
			Hour = decToBcd(Hour) & 0b11011111;
		}
	} else {
		// 24 hour
		Hour = decToBcd(Hour) & 0b10111111;
	}

	Wire.beginTransmission(CLOCK_ADDRESS);
	Wire.write(0x02);
	Wire.write(Hour);
	Wire.endTransmission();
}

void DS3231MDA::setDoW(byte DoW) {
	// Sets the Day of Week
	Wire.beginTransmission(CLOCK_ADDRESS);
	Wire.write(0x03);
	Wire.write(decToBcd(DoW));	
	Wire.endTransmission();
}

void DS3231MDA::setDate(byte Date) {
	// Sets the Date
	Wire.beginTransmission(CLOCK_ADDRESS);
	Wire.write(0x04);
	Wire.write(decToBcd(Date));	
	Wire.endTransmission();
}

void DS3231MDA::setMonth(byte Month) {
	// Sets the month
	Wire.beginTransmission(CLOCK_ADDRESS);
	Wire.write(0x05);
	Wire.write(decToBcd(Month));	
	Wire.endTransmission();
}

void DS3231MDA::setYear(byte Year) {
	// Sets the year
	Wire.beginTransmission(CLOCK_ADDRESS);
	Wire.write(0x06);
	Wire.write(decToBcd(Year));	
	Wire.endTransmission();
}

void DS3231MDA::setClockMode(bool h12) {
	// sets the mode to 12-hour (true) or 24-hour (false).
	// One thing that bothers me about how I've written this is that
	// if the read and right happen at the right hourly millisecnd,
	// the clock will be set back an hour. Not sure how to do it better, 
	// though, and as long as one doesn't set the mode frequently it's
	// a very minimal risk. 
	// It's zero risk if you call this BEFORE setting the hour, since
	// the setHour() function doesn't change this mode.
	
	byte temp_buffer;

	// Start by reading byte 0x02.
	Wire.beginTransmission(CLOCK_ADDRESS);
	Wire.write(0x02);
	Wire.endTransmission();
	Wire.requestFrom(CLOCK_ADDRESS, 1);
	temp_buffer = Wire.read();

	// Set the flag to the requested value:
	if (h12) {
		temp_buffer = temp_buffer | 0b01000000;
	} else {
		temp_buffer = temp_buffer & 0b10111111;
	}

	// Write the byte
	Wire.beginTransmission(CLOCK_ADDRESS);
	Wire.write(0x02);
	Wire.write(temp_buffer);
	Wire.endTransmission();
}

float DS3231MDA::getTemperature() {
	// Checks the internal thermometer on the DS3231 and returns the 
	// temperature as a floating-point value.

  // Updated / modified a tiny bit from "Coding Badly" and "Tri-Again"
  // http://forum.arduino.cc/index.php/topic,22301.0.html
  
  byte tMSB, tLSB;
  float temp3231;
  
  // temp registers (11h-12h) get updated automatically every 64s
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(0x11);
  Wire.endTransmission();
  Wire.requestFrom(CLOCK_ADDRESS, 2);

  // Should I do more "if available" checks here?
  if(Wire.available()) {
    tMSB = Wire.read(); //2's complement int portion
    tLSB = Wire.read(); //fraction portion

    temp3231 = ((((short)tMSB << 8) | (short)tLSB) >> 6) / 4.0;
  }
  else {
    temp3231 = -9999; // Some obvious error value
  }
   
  return temp3231;
}


/***************************************** 
	Private Functions
 *****************************************/

byte DS3231MDA::decToBcd(byte val) {
// Convert normal decimal numbers to binary coded decimal
	return ( (val/10*16) + (val%10) );
}

byte DS3231MDA::bcdToDec(byte val) {
// Convert binary coded decimal to normal decimal numbers
	  return ( (val/16*10) + (val%16) );
}

byte DS3231MDA::readControlByte(bool which) {
	// Read selected control byte
	// first byte (0) is 0x0e, second (1) is 0x0f
	Wire.beginTransmission(CLOCK_ADDRESS);
	if (which) {
		// second control byte
		Wire.write(0x0f);
	} else {
		// first control byte
		Wire.write(0x0e);
	}
	Wire.endTransmission();
	Wire.requestFrom(CLOCK_ADDRESS, 1);
	return Wire.read();	
}

void DS3231MDA::writeControlByte(byte control, bool which) {
	// Write the selected control byte.
	// which=false -> 0x0e, true->0x0f.
	Wire.beginTransmission(CLOCK_ADDRESS);
	if (which) {
		Wire.write(0x0f);
	} else {
		Wire.write(0x0e);
	}
	Wire.write(control);
	Wire.endTransmission();
}

