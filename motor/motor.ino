#include <Adafruit_VL53L0X.h>
#include <BMP085.h>
#include <DHT.h>
#include <structs.h>

#define MV_MAX_STEP_COUNT 10
#define MV_TIME_STEP 1100
#define MIN_TEMP_C 20
#define MAX_TEMP_C 28

Adafruit_VL53L0X lox = Adafruit_VL53L0X();



sensorInfo _sensorInfo;
int d;
int prevT=MIN_TEMP_C * 100;
int stepPerC=MIN_TEMP_C*100;
float stepPerCd;



void wakeUp() {
   pinMode(4,OUTPUT);
   digitalWrite(4,HIGH);
}

void setup() {
  // put your setup code here, to run once:
  pinMode(3,OUTPUT);  //Input 1
  digitalWrite(3,LOW);
  
  pinMode(6,OUTPUT);  //Input 2
  digitalWrite(6,LOW);    
  pinMode(LED_BUILTIN,OUTPUT);

  Serial.begin(19200);

  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
  }


  wakeUp(); 

  digitalWrite(LED_BUILTIN,HIGH);   
  setZero();
  digitalWrite(LED_BUILTIN,LOW); 

  stepPerCd=(float)MV_MAX_STEP_COUNT / (100*(MAX_TEMP_C - MIN_TEMP_C));
  Serial.print(F("Step per C = "));
  Serial.println(stepPerCd);

}

void stopMove() {
  digitalWrite(3,LOW);    
  digitalWrite(6,LOW);       
}



void frwdStep2() {     
      digitalWrite(6,HIGH);      
      digitalWrite(3,LOW);      
      delay(MV_TIME_STEP);
      stopMove();      
}


void bcwdStep2() {
      digitalWrite(6,LOW);      
      digitalWrite(3,HIGH);      
      delay(MV_TIME_STEP);   
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

void loadSensorInfo1() {
  //Serial.println(F("Enter temp ..."));
  //Serial.flush();
//while (Serial.available()<=0) {  1; };

//_sensorInfo.t1=Serial.parseInt() * 100;



  {
    BMP085   dps = BMP085();
    dps.init(MODE_STANDARD, 17700, true);
    _sensorInfo.t2 = dps.getTemperature2();
    _sensorInfo.p1 = dps.getPressure2();
  };


  {
    DHT dht(5, DHT22);
    dht.begin();
    _sensorInfo.t1 = dht.readTemperature2();
    _sensorInfo.h1 = dht.readHumidity2();
  };

  Serial.println(_sensorInfo.t1);

  _sensorInfo.distance = getDistance();


  
}

void loop() {        
  int currT;
  loadSensorInfo1();
 
  currT=min(max(_sensorInfo.t1,MIN_TEMP_C*100),MAX_TEMP_C*100);
  d=currT-prevT;

  if (d>0) {
    Serial.print(F("Move up = "));
    Serial.println(d);
    Serial.print(F("Step count"));
    Serial.println(round(d * stepPerCd));
    for (int i=0;i<round(d * stepPerCd);i++) {
      frwdStep2();
    }
  } else if (d<0) {
    Serial.print(F("Move down = "));
    Serial.println(round(abs(d) * stepPerCd));
    for (int i=0;i<round(abs(d) * stepPerCd);i++) {
       bcwdStep2();
    };
  };

  prevT=currT;
  

   delay(60000);
}
