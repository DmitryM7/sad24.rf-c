#include <sim868.h> 
 
 sim868 gprs(7,8);
    
void setup() {
  // put your setup code here, to run once:

  Serial.begin(19200);
  
  //gprs.wakeUp();
  //delay(30000);

  

  bool vStatus  = false;
  byte vAttempt = 0;
  char apnPoint[]="internet.mts.ru",apnLogin[]="mts",apnPass[]="mts",vParams[]="",vUrl[75]="xn--24-6kcq7d.xn--p1ai/ri/sa",vOut[250];

  gprs.setInternetSettings(apnPoint,apnLogin,apnPass);
  /*
  do {
     vStatus = gprs.canWork();
     delay(vAttempt*3000);
     vAttempt++;     
  } while (!vStatus && vAttempt < 3);


  gprs.doInternet();

  
  gprs.postUrl(vUrl,vParams,vOut);

  Serial.println(F("---"));
  Serial.println(vOut);
  Serial.println(F("---"));
  */

  {
    sim868_coords coords;
    gprs.getCoords(coords);

    Serial.print(F("COORDS="));Serial.print(coords.a);Serial.print(F("&"));Serial.println(coords.l);
  }

  

}

void loop() {
  // put your main code here, to run repeatedly:

}
