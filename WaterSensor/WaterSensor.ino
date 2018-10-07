#include <avr/wdt.h>
/************************************************************************************************
 * Пример кода для посчета количества протекующей жидкости                                      *
 * int Calc;                               
 * NbTopsFan = 0;   //Set NbTops to 0 ready for calculations                                    *
 * 
 * sei();      //Enables interrupts                                                              *
 * delay (1000);   //Wait 1 second                                                               *
 * cli();      //Disable interrupts                                                              *
 * Calc = (NbTopsFan * 60 / 5.5); //(Pulse frequency x 60) / 5.5Q, = flow rate in L/hour         *
 * Serial.print (Calc, DEC); //Prints the number calculated above                                *
 * Serial.print (" L/hour\r\n"); //Prints "L/hour" and returns a  new line                       *
 ************************************************************************************************/

volatile int               NbTopsFan; //measuring the rising edges of the signal
volatile unsigned long int mPrevFlow = 0;
bool                       isWater = false;  
 
void rpm ()     //This is the function that the interupt calls 
{ 
  NbTopsFan++;  //This function measures the rising and falling edge of the hall effect sensors signal
  mPrevFlow = millis();
} 
// The setup() method runs once, when the sketch starts
void setup() //
{ 
  Serial.begin(19200); 

  Serial.println(F("*** Start device ***")); Serial.flush();
  
  pinMode(2, INPUT); 
  attachInterrupt(0, rpm, RISING); 
  
  pinMode(A0, INPUT);

  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);

  digitalWrite(11,HIGH);
  digitalWrite(12,HIGH);

  wdt_enable (WDTO_8S);
  
} 

float getCurrPreasure() {
  unsigned int v;
  float ret;
  v  = analogRead(A0);
  ret = 2 * (float)v * 5 / 1024;
  return ret < 0 ? 0 : ret;  
}

unsigned long int getPrevFlowDiff() {
  return millis() - mPrevFlow;
}


void loop ()    
{

  float mMinPreasure      = 2,
        mMaxPreasure      = 3,
        mCriticalPreasure = 4,
        mCurrPreasure,
        mCurrFlowTimeDiff;
  int mFlowTimeDiff       = 200;

   
  

 /*****************************************************
  * Проверка №1.
  * Нет потока. Проверяем давление и                  *
  *  при достижении нужной границы отключаем воду.    *
  *****************************************************/
 
 if ((mCurrFlowTimeDiff = getPrevFlowDiff()) >= mFlowTimeDiff && (mCurrPreasure = getCurrPreasure()) >= mMaxPreasure) {
    Serial.println(F("Max preasure: ")); Serial.print(mCurrPreasure); Serial.print(F(" and no flow: ")); Serial.print(mCurrFlowTimeDiff); Serial.println(F(" - power off."));    
    Serial.flush();
    digitalWrite(11,HIGH);
    isWater = false;
    return;
  };

 /********************************************************
  * Проверка №2.                                        *
  * Достигли критического значения по давлению.          *
  * Отключаем насос вне зависимости от других параметров *
  ********************************************************/
  if ((mCurrPreasure = getCurrPreasure()) >= mCriticalPreasure) {
    Serial.print(F("!!! Critical preasure: ")); Serial.print(mCurrPreasure); Serial.println(F("- power off !!!"));    
    Serial.print(F("FLOW: ")); Serial.println(getPrevFlowDiff());    
    Serial.flush();
    digitalWrite(11,HIGH);
    isWater = false;
    return;
  };

  /**************************************************
   * Проверка №3.                                  *
   * Давление упало. При этом насос не работает.    *
   * Включаем его.                                  *
   **************************************************/

  if ((mCurrPreasure = getCurrPreasure()) <= mMinPreasure && !isWater) {
    Serial.print(F("Min preasure: ")); Serial.print(mCurrPreasure); Serial.println(F(" - power on."));    
    Serial.flush();
    digitalWrite(11,LOW);
    isWater = true;
    return;
  };

 wdt_reset();

}
