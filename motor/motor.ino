#include <avr/wdt.h>
#include <debug.h>
#include <Adafruit_VL53L0X.h>
#include <BMP085.h>
#include <DHT.h>
#include <structs.h>

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

#define MIN_TEMP_C 20
#define MAX_TEMP_C 32
#define MAX_STEP_COUNT 10

/* Границы температур во внутреннем представлении */
#define MIN_TEMP_C_INNER MIN_TEMP_C * 100
#define MAX_TEMP_C_INNER MAX_TEMP_C * 100

/* Шаг двигателя */
#define MV_TIME_STEP 1100




sensorInfo _sensorInfo;
int d;

int prevT=MIN_TEMP_C_INNER;

void loadSensorInfo1() {
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

 #ifdef IS_DEBUG
  Serial.print(F("T from sensor = "));
  Serial.println(_sensorInfo.t1);
 #endif

 {
   VL53L0X_RangingMeasurementData_t measure;  
   lox.rangingTest(&measure, false);
   _sensorInfo.distance = measure.RangeStatus != 4 ? measure.RangeMilliMeter : -1;
 }

}

/******************************************
*  Функции для работы с двигателем        *
*******************************************/

void syncEngine(int dT) {
  int stepT;
  int stepEngine;
  stepT=(int)round((MAX_TEMP_C_INNER - MIN_TEMP_C_INNER) / MAX_STEP_COUNT);
  stepEngine=round(dT / stepT);

  if (stepEngine>0) {
    for (unsigned int i=0;i<stepEngine;i++) {
      frwdStep2();
      wdt_reset();
    };
  } else if (stepEngine<0) {
    for (unsigned int i=0;i<abs(stepEngine);i++) {
      bcwdStep2();
      wdt_reset();
    };
  }
  
}

void wakeUp() {
   pinMode(4,OUTPUT);
   digitalWrite(4,HIGH);
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

void setZero() {
  digitalWrite(3,HIGH);    
  digitalWrite(6,LOW);    
  delay(12000);  
  stopMove();
}

/*****************************************
 * Конец функций для работы с двигателем *
 *****************************************/

void setup() {
  // put your setup code here, to run once:
  #ifdef IS_DEBUG
   Serial.begin(19200);
   Serial.println(F("GO"));
  #endif
  pinMode(3,OUTPUT);
  pinMode(3,OUTPUT);
  pinMode(LED_BUILTIN,OUTPUT);
  stopMove();


  if (!lox.begin()) {
    #ifdef IS_DEBUG
     Serial.println(F("Failed to boot VL53L0X"));
    #endif
  }


  wakeUp(); 

  digitalWrite(LED_BUILTIN,HIGH);   
  setZero();
  digitalWrite(LED_BUILTIN,LOW);  
  wdt_enable (WDTO_8S);
}


void loop() {        
  int currT,dT;
  
  loadSensorInfo1();

  //Определеяем выходит ли температура за область допустимых значений
  if (_sensorInfo.t1 < MIN_TEMP_C_INNER) {
     currT=MIN_TEMP_C_INNER;
  };

  if (_sensorInfo.t1 > MAX_TEMP_C_INNER) {
    currT=MAX_TEMP_C_INNER;
  };


 //Находим разницу насколько нужно двигать
  dT = currT-prevT;


  #ifdef IS_DEBUG
   Serial.print(F("Curr T=")); Serial.print(currT); Serial.print(F("; Prev T=")); Serial.print(prevT), Serial.print(F("; dT=")); Serial.println(dT);
  #endif
  
  prevT=currT;
  
  syncEngine(dT);
  delay(60000);
}
