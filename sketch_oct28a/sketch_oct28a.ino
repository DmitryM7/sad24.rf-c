void setup() {
  // put your setup code here, to run once:
  pinMode(10,INPUT);
  Serial.begin(19200);
}

void loop() {
  int r;
  // put your main code here, to run repeatedly:
  r=digitalRead(10);
  Serial.println(r);
  delay(3000);

}
