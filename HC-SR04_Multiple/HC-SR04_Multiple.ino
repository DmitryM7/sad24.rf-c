#include <HCSR04.h>

byte triggerPin = 10;
byte echoCount = 6;
byte* echoPins = new byte[echoCount] { 12, 13 };

void setup () {
  Serial.begin(19200);
  HCSR04.begin(triggerPin, echoPins, echoCount);
}

void loop () {
  double* distances = HCSR04.measureDistanceCm();

  for (int i = 0; i < echoCount; i++) {
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(distances[i]);
    Serial.println(" cm");
  }
  
  Serial.println("---");
  delay(250);
}
