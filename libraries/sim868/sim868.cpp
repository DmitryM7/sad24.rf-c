#include <Arduino.h>
#include <SoftwareSerial.h>
#include <sim868.h>
#include <mstr.h>
#include <debug.h>



sim868::sim868(int pin1,int pin2) : sim800(pin1,pin2)  {};


bool sim868::getCoords(sim868_coords& coords) {
    char vRes[70],vTmpStr[9],vA[11],vL[11],vPoint[2];
    mstr _mstr;
    unsigned long vCurrTime = millis();

   strcpy_P(vPoint, (char*)POINT_M);

    //Начинаем долбить опрос координат,
    //Делаем это не менее 1 минуты

    do {

        strcpy_P(vTmpStr,PSTR("+CGNSINF"));
       _doCmd(F("AT+CGNSPWR=1;+CGNSINF"));

       if (_getAnswerWait(vRes,sizeof(vRes),vTmpStr)!=-1) {

          // strcpy_P(vRes,PSTR("+CGNSINF: 1,1,0,1,1,162.369,0.00,12"));
          // strcpy_P(vRes,PSTR("+CGNSINF: 1,1,20220911183139.000,55.917868,37.776849,162.369,0.00,12"));
          // strcpy_P(vRes,PSTR("+CGNSINF: 1,1,20220911183139.000,550.917868,370.776849,162.369,0.00,12"));
          // strcpy_P(vRes,PSTR("+CGNSINF: 1,1,20220911183139.000,,,162.369,0.00,12"));
          // Serial.print(F("--->"));Serial.print(vRes);Serial.println(F("<---"));

         strcpy_P(vTmpStr, (char*)COMMA_M);
        _mstr.entry(4,vRes,vTmpStr,sizeof(vA),vA);
        _mstr.entry(5,vRes,vTmpStr,sizeof(vL),vL);


        if (strlen(vA)==0) {
           coords.a=0;
        } else {

               int p=_mstr.indexOf(vA,vPoint);
               _mstr.leftShift2(vA,p,1);
               coords.a=atol(vA);
        };

        if (strlen(vL)==0) {
           coords.l=0;
        } else {
               int p=_mstr.indexOf(vL,vPoint);

                  _mstr.leftShift2(vL,p,1);
                 coords.l=atol(vL);
        };

       };



    } while (coords.a==0 && coords.l==0 && millis()-vCurrTime<=180000);//запрашиваем координаты в течение 3 минут.


}

   void sim868::btConnect() {
     char vRes[70],vTmpStr[13];

        strcpy_P(vTmpStr,PSTR("AT+BTPOVER=1"));
       _doCmd(vTmpStr);
   }



