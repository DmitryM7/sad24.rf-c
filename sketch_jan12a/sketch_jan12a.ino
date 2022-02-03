#include <worker.h>
 worker _worker(0);

void showLong(long long iVal) {

 char buffer[25];
 sprintf_P(buffer, PSTR("%0ld"),iVal/1000000L);
 Serial.print(buffer);  
 sprintf_P(buffer, PSTR("%0ld"),iVal%1000000L);
 Serial.print(buffer); 

}

void setup() {
  Wire.begin(); // ВАЖНО!!! ЭТА КОМАНДА ДОЛЖНА БЫТЬ ДО ПЕРВОЙ РАБОТЫ С ЧАСАМИ
  // put your setup code here, to run once:
    Serial.begin(19200);
  Serial.println(F("GO"));
  _worker.setDateTime(16, 9, 13, 18, 45, 0);
  Serial.println(F("END"));

}

void loop() {
  // put your main code here, to run repeatedly:
   unsigned long vCurrTime;
   long long vTimeStamp;
   byte vDayOfWeek;
   vTimeStamp   = _worker.getTimestamp(vCurrTime,vDayOfWeek);
   showLong(vTimeStamp); Serial.println();
   delay(5000);

}
