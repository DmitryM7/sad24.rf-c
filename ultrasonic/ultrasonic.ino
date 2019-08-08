#define ECHO_PIN 13
#define TRIG_PIN 3

 
void setup() { 
    Serial.begin (19200); 
    pinMode(TRIG_PIN, OUTPUT); 
    pinMode(ECHO_PIN, INPUT); 
} 
 
void loop() { 
    int duration, cm; 

    
    digitalWrite(TRIG_PIN, LOW); 
    delayMicroseconds(2); 
    digitalWrite(TRIG_PIN, HIGH); 
    delayMicroseconds(10); 
    digitalWrite(TRIG_PIN, LOW); 
    duration = pulseIn(ECHO_PIN, HIGH); 
    cm = duration / 58;
    Serial.print(cm); Serial.println(" cm");   delay(1000);
}
