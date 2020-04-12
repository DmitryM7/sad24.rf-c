/***************************************************************
 * Приложение измеряет уровень воды                            *
 * и если он ниже заданного, то запрещает включение насоса.    *
 ***************************************************************/

 #include <worker.h>

/** Задаем параметры подключения УЗД **/
#define ECHO_PIN 10
#define TRIG_PIN 3

/** Задаю пороговые значения в сантиметрах **/


workerInfo _water;
workerInfo _light;


int getDistance() {
    int duration, cm;     
    digitalWrite(TRIG_PIN, LOW); 
    delayMicroseconds(2); 
    digitalWrite(TRIG_PIN, HIGH); 
    delayMicroseconds(55); 
    digitalWrite(TRIG_PIN, LOW); 
    duration = pulseIn(ECHO_PIN, HIGH); 
    cm = duration / 58;
    return cm;
}
 
void setup() { 
    Serial.begin (19200); 
    pinMode(TRIG_PIN, OUTPUT); 
    pinMode(ECHO_PIN, INPUT); 
    pinMode(LED_BUILTIN, OUTPUT);

} 
 
void loop() { 
   int vCurrDistance = getDistance();
   worker _worker(0);    
   digitalWrite(LED_BUILTIN,HIGH);
  
   Serial.print(vCurrDistance); Serial.println(" cm");   

   if (vCurrDistance>150) {
      _worker.startWater();
   } else {
      _worker.stopWater();
   };
   delay(1000);
   digitalWrite(LED_BUILTIN,LOW);
   delay(1000);
   
}
