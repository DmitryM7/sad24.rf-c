void setup() {
  // put your setup code here, to run once:
  char vA[11],vL[11];

  memset(vA,'0',sizeof(vA));

  Serial.begin(19200);
  Serial.print(F("--->")); Serial.print(vA);Serial.println(F("<---"));

}

void loop() {
  // put your main code here, to run repeatedly:

}
