#include <avr/wdt.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "FastLED.h"
#include "debug.h"

#define ONE_WIRE_BUS 3

/**************************
 * Определяем Dallas      *
 **************************/
OneWire           oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress     insideThermometer;

void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

/********************************
 * Опредеяем светодиодную ленту
 */

#define NUM_LEDS 130
#define DATA_PIN 12
CRGB leds[NUM_LEDS];

bool isLedsWork,mIsDallas=false;

void hide() {
  for (unsigned int vI=0;vI<NUM_LEDS;vI++) {
    leds[vI] = CRGB::Black; 
    
  };
  FastLED.show();
}

void setRGB(int r,int g,int b) {
  for (unsigned int vI=0;vI<NUM_LEDS;vI++) {
    leds[vI].setRGB(r,g,b); 
 
    
  };
  FastLED.show();
}

void doColorByTemp(float iTemp) {
  unsigned int vColor = 50;
  int p[5] = {50,70,85,95,105};
  float vDiff;
  
  if (iTemp <= p[0]) {
       setRGB(0,0, vColor);
  };

     if (iTemp > p[0] && iTemp <= p[1]) {
       vDiff = vColor / (p[1] - p[0]);
       setRGB(0,round((iTemp-p[0]) * vDiff),round(vColor - (iTemp - p[0]) * vDiff));
     };

 
  if (iTemp >= p[1] && iTemp < p[2]) {
       setRGB(0,vColor, 0);
  };  

     if (iTemp > p[2] && iTemp <= p[3]) {
       vDiff = vColor / (p[3] - p[2]);
       setRGB(round((iTemp-p[2]) * vDiff),(int) round(vColor - (iTemp - p[2]) * vDiff),0);
     };

  if (iTemp >= p[3] && iTemp < p[4]) {
       setRGB(vColor, 0, 0);
  };

  
  if (iTemp > p[4]) {
       setRGB(vColor, 0, 0);
       delay(2000);
       hide();
  };
     
     
}

  
void setup() {
  int vDeviceCount;
  
  #ifdef IS_DEBUG 
    #pragma message("Compiling with debug");
    Serial.begin(19200);   
    Serial.println(F("*** START ***"));
  #endif

  sensors.begin();
  vDeviceCount=sensors.getDeviceCount();

 #ifdef IS_DEBUG   
  Serial.print(F("Found "));
  Serial.print(vDeviceCount);
  Serial.println(F(" devices."));
 #endif

  if (sensors.getAddress(insideThermometer, 0)) {
     mIsDallas=true;
      // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
    sensors.setResolution(insideThermometer, 9);
  
    #ifdef IS_DEBUG 
      // show the addresses we found on the bus
      Serial.print(F("Device 0 Address: "));
      printAddress(insideThermometer);
      Serial.println();
      Serial.print(F("Device 0 Resolution: "));
      Serial.print(sensors.getResolution(insideThermometer), DEC); 
      Serial.println();
    #endif  
  } else {
    #ifdef IS_DEBUG
     Serial.println(F("Unable to find address for Device 0")); 
    #endif;
  }
 
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);

  hide();
  isLedsWork = false;

  wdt_enable (WDTO_8S);
}

void loop() {
  float vTempDallas,
        vTempWork;
  

   if (mIsDallas) {
     sensors.requestTemperatures();
     vTempDallas = sensors.getTempC(insideThermometer);
     #ifdef IS_DEBUG
       Serial.print(F("C from dallas = ")); 
       Serial.println(vTempDallas);
     #endif
     vTempWork=vTempDallas;
   };

     

    doColorByTemp(vTempDallas);

    isLedsWork = true;
 
   delay(1000);
   
   wdt_reset();
}
