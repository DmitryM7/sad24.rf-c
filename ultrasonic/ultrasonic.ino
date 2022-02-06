/***************************************************************
 * Приложение измеряет уровень воды                            *
 * и если он ниже заданного, то запрещает включение насоса.    *
 ***************************************************************/
#include <structs.h> 
#include <worker.h>
                 
/** Задаю пороговые значения в сантиметрах **/


workerInfo _water;
workerInfo _light;


int getMiddleDistance() {
   int vMeasurement[ __DISTANCE_COUNT__],
       vTemp=0;
  
  for (byte i=0; i< __DISTANCE_COUNT__;i++) {
    vMeasurement[i]=getDistance(); 
    delay(100); //Делаем задержку иначе датчик показывает не верные данные
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

  for (byte i=0; i< __DISTANCE_COUNT__;i++) {
    Serial.print(vMeasurement[i]);
    Serial.print(F(","));
  };
    Serial.println();

  return round(vTemp / (__DISTANCE_COUNT__-4));
  
}
int getDistance() {
  unsigned int duration, cm;  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  cm      = round(duration / 58);
  return cm;
}
 
void setup() { 
    Serial.begin (19200); 
    pinMode(TRIG_PIN, OUTPUT); 
    pinMode(ECHO_PIN, INPUT); 
    pinMode(LED_BUILTIN, OUTPUT);

} 
 
void loop() { 
  int vCurrDistance = getMiddleDistance();
   worker _worker(0);    
   digitalWrite(LED_BUILTIN,HIGH);
  
   Serial.print(vCurrDistance); Serial.println(" cm");   
   delay(3000);
   return;
   
}
