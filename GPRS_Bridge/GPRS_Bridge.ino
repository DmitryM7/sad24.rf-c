//Serial Relay - Arduino will patch a 
//serial link between the computer and the GPRS Shield
//at 19200 bps 8-N-1
//Computer is connected to Hardware UART
//GPRS Shield is connected to the Software UART 
 
#include <SoftwareSerial.h>
#include <LowPower.h>
#include <debug.h>
#include <structs.h>
#include <gprs2.h>


bool wk() {

  bool vStatus = false;
  byte vAttempt = 0;
                                                                                     
  gprs2 sim900(7, 8);
   
  {
    char _apnPoint[35],_apnLogin[11],_apnPass[11];      
    strcpy_P(_apnPoint,PSTR("internet.beeline.ru"));
    strcpy_P(_apnLogin,PSTR("beeline"));
    strcpy_P(_apnPass,PSTR("beeline"));
    sim900.setInternetSettings(_apnPoint, _apnLogin, _apnPass);
  };

  sim900.wakeUp();  
  delay(__WAIT_MODEM_TIME__);
    
  do {    
    vStatus = sim900.canWork();    
    delay(vAttempt * 3000);
    vAttempt++;
  } while (!vStatus && vAttempt < 3);

#if IS_DEBUG>1
  if (!vStatus) {
      char vError[20];
      sim900.getLastError(vError);
      Serial.print(F("WKE:"));
      Serial.println(vError);
      Serial.flush();
  };
#endif

  return vStatus;

}

SoftwareSerial GPRS(7,8);
unsigned char buffer[64]; // buffer array for data recieve over serial port
int count=0;     // counter for buffer array 
void setup()
{  
      /*pinMode(4,OUTPUT);
      digitalWrite(4,HIGH);*/
      Serial.begin(19200);             // the Serial port of Arduino baud rate.   
      wk();
  
  GPRS.begin(19200);               // the GPRS baud rate   
  
  Serial.println(F("GO"));
  delay(1000);      
}
 
void loop()
{
  
  
  if (GPRS.available())              // if date is comming from softwareserial port ==> data is comming from gprs shield
  {
    while(GPRS.available())          // reading data into char array 
    {
      buffer[count++]=GPRS.read();     // writing data into array
      if(count == 64)break;
    }
    Serial.write(buffer,count);            // if no data transmission ends, write buffer to hardware serial port
    clearBufferArray();              // call clearBufferArray function to clear the storaged data from the array
    count = 0;                       // set counter of while loop to zero
 
 
  };  
  
  if (Serial.available())  {          // if data is available on hardwareserial port ==> data is comming from PC or notebook   
    GPRS.write(Serial.read());       // write it to the GPRS shield    
  };


  


}
void clearBufferArray()              // function to clear buffer array
{
  for (int i=0; i<count;i++)
    { buffer[i]=NULL;}                  // clear all index of array with command NULL
}
