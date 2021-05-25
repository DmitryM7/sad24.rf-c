#include "Adafruit_VL53L0X.h"

#define MV_TIME 30
#define MV_TIME_STEP 1100
#define RLX_TIME 20
#define STP_COUNT 1075 //Это кол-во циклов до полного открытия

  Adafruit_VL53L0X lox = Adafruit_VL53L0X();

void setup() {
  // put your setup code here, to run once:
  pinMode(3,OUTPUT);  //Input 1
  digitalWrite(3,LOW);
  
  pinMode(6,OUTPUT);  //Input 2
  digitalWrite(6,LOW);    
  pinMode(LED_BUILTIN,OUTPUT);


  Serial.begin(19200);

  while (! Serial) {
    delay(1);
  }

  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }

    
}

void stopMove() {
  digitalWrite(3,LOW);    
  digitalWrite(6,LOW);       
}

void frwdStep() {     
     for (unsigned int i=0;i<STP_COUNT;i++) {
      digitalWrite(6,HIGH);      
      digitalWrite(3,LOW);      
      delay(MV_TIME);
      stopMove();      
      delay(RLX_TIME);      
   };
}

void frwdStep2() {     
      digitalWrite(6,HIGH);      
      digitalWrite(3,LOW);      
      delay(MV_TIME_STEP);
      stopMove();      
}

void bcwdStep() {
     for (unsigned int i=0;i<STP_COUNT;i++) {
      digitalWrite(6,LOW);      
      digitalWrite(3,HIGH);      
      delay(MV_TIME);
      stopMove();      
      delay(RLX_TIME);
   };
  
}

int getDistance() {
  VL53L0X_RangingMeasurementData_t measure;  
  lox.rangingTest(&measure, false);

  return measure.RangeStatus != 4 ? measure.RangeMilliMeter : -1;  
}


void setZero() {
  digitalWrite(3,HIGH);    
  digitalWrite(6,LOW);    
  delay(12000);  
  stopMove();
}

void loop() {  
    
  digitalWrite(LED_BUILTIN,HIGH);   
  setZero();
  digitalWrite(LED_BUILTIN,LOW); 
  Serial.print(F("Dist=")); Serial.println(getDistance()); Serial.flush();
  delay(2000);


     

   digitalWrite(LED_BUILTIN,HIGH);   
   for (unsigned int i=0;i<10;i++) {
    frwdStep2();
    Serial.print(F("Dist=")); Serial.println(getDistance()); Serial.flush();
    delay(2000);
   }
    
   digitalWrite(LED_BUILTIN,LOW);     
   
    
 
   Serial.print(F("Dist=")); Serial.println(getDistance()); Serial.flush();

   delay(2000);

   return;


   digitalWrite(LED_BUILTIN,HIGH);   
     stopMove();
   digitalWrite(LED_BUILTIN,LOW);    

    delay(2000);

   digitalWrite(LED_BUILTIN,HIGH);   
    bcwdStep();
   digitalWrite(LED_BUILTIN,LOW);     

   digitalWrite(LED_BUILTIN,HIGH);   
     stopMove();
   digitalWrite(LED_BUILTIN,LOW);    

    delay(10000);    
      
}
