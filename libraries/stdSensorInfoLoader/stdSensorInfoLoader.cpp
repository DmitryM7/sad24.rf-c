#include <Arduino.h>
#include <stdSensorInfoLoader.h>
#include <BMP085.h>
#include <DHT.h>

int stdSensorInfoLoader::getT1() {
 return t1;
}

int stdSensorInfoLoader::getT2() {
 return t2;
}

int stdSensorInfoLoader::getH1() {
 return h1;
}

long stdSensorInfoLoader::getP1() {
 return p1;
}

void stdSensorInfoLoader::setLastMeasure(long long t) {
    lastMeasure = t;
}

long long stdSensorInfoLoader::getLastMeasure() {
    return lastMeasure;
}

unsigned int stdSensorInfoLoader::getDistance() {
 return distance;
}

unsigned int stdSensorInfoLoader::loadDistance() {
  unsigned int duration, cm;
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(55);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  cm      = round(duration / 58);
  return cm;
}

int stdSensorInfoLoader::calcMiddleDistance() {
   int vMeasurement[ __DISTANCE_COUNT__],
       vTemp=0;

  for (byte i=0; i< __DISTANCE_COUNT__;i++) {
    vMeasurement[i]=loadDistance();
    delay(10); //Делаем задержку иначе датчик показывает не верные данные
  };
  //Теперь сортируем массив по возрастанию. По идее массив должен быть почти упорядоченным, поэтому использую алгоритм вставками

  for (byte i=1;i<__DISTANCE_COUNT__;i++) {
    vTemp=vMeasurement[i];
    byte j=i;
    while (j>0 and vMeasurement[j-1] > vTemp) {
      vMeasurement[j]=vMeasurement[j-1];
      j=j-1;
    };
    vMeasurement[j]=vTemp;
  };


  vTemp=0;
  //Теперь отбрасываем "выскакивающие" значения и находим среднее
  for (byte i=2; i< __DISTANCE_COUNT__-2;i++) {
    vTemp+=vMeasurement[i];
  };

  return round(vTemp / (__DISTANCE_COUNT__-4));

}

void stdSensorInfoLoader::loadSensorInfo() {
{
    BMP085   dps = BMP085();
    dps.init(MODE_STANDARD, 17700, true);
    t2 = dps.getTemperature2();
    p1 = dps.getPressure2();
  };


  {
    DHT dht(5, DHT22);
    unsigned long vCurrTime = millis();
    dht.begin();

     /***********************************************************
      *    Датчик влажности тугой. Он может с первого           *
      *    раза данные не считать. Поэтому пытаемся             *
      *    его считать столько раз, сколько успеем за 5 секунд. *
      ***********************************************************/
    do {
      t1 = dht.readTemperature2();
      h1 = dht.readHumidity2();
    } while (t1==0 && h1==0 && millis() - vCurrTime < 3000);

  };


  distance = calcMiddleDistance();
}
