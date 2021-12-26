#include <worker.h>
 worker _worker(0);

void setup() {
  // put your setup code here, to run once:
  interrupts();
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
   Serial.println(vCurrTime);
   delay(5000);

}
