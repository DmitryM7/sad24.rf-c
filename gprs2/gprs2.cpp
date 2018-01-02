#include <Arduino.h>
#include <SoftwareSerial.h>
#include <gprs2.h>
#include <mstr.h>



const char OK_M[] PROGMEM = "OK";



gprs2::gprs2(int pin1,int pin2):_modem(pin1,pin2) {
// Module revision Revision:1137B06SIM900M64_ST_ENHANCE
   _modem.begin(19200); 
   _emptyBuffer(_lastError,20);
};


void gprs2::setInternetSettings(char* vApn,char* vLogin,char* vPass) {
   strcpy(_apn,vApn);
   strcpy(_login,vLogin);
   strcpy(_pass,vPass);
}



bool gprs2::getAnswer3(char* oRes,unsigned int iSize) {
  return _getAnswer3(oRes,iSize,false,false);
};

bool gprs2::_getAnswer3(char* oRes,unsigned int iSize,bool saveCRLF) {
  return _getAnswer3(oRes,iSize,saveCRLF,false);
};

bool gprs2::_getAnswer3(char* oRes,unsigned int iSize,bool saveCRLF,bool showAnswer) {
  unsigned long vCurrTime,
                vLastReadTime;
  unsigned int vI = 0;
  bool wasRead=false;

  _emptyBuffer(oRes, iSize);

  vCurrTime     = millis();
  vLastReadTime = millis();

  while (millis() - vCurrTime < 15000) {

    if (_modem.available()) {

      char vChr  = _modem.read();
     
      if (showAnswer) {
          Serial.println(vChr);
          Serial.flush();
      };

      if (vI >= iSize - 2) {
          // Здесь делаем именно continue чтобы вычитать весь буффер
          continue;
      };

      switch (vChr) {
        case '\0':
          break;
        case '\n':
        case '\r':
           if (!saveCRLF) {

               /***********************************************
                * Убираем начальные и подряд идующие пробелы. *
                ***********************************************/
              if (vI!=0 && oRes[vI-1]!=' ') {
                 oRes[vI] = ' ';
                 vI++;
              };
           } else {
              oRes[vI] = vChr;
              vI++;
           };
          break;
        default:
          oRes[vI] = vChr;
          vI++;
          break;
      };

      vLastReadTime = millis();
      wasRead = true;


    };

     if (millis() - vLastReadTime > 750 && wasRead) {
        oRes[vI+1] = '\0';
       return true;
      };

  }; //WHILE
  return false;         
}                                     



 bool gprs2::isPowerUp() {
     char vTmpStr[3],
          vRes[10];
     mstr _mstr;

  
    _doCmd(F("AT"));
    getAnswer3(vRes,10);

    strcpy_P(vTmpStr, (char*)OK_M);

    if (_mstr.indexOf(vRes,vTmpStr)==-1) {
      _setLastError(__LINE__,vRes);
      return false;
    };

      return true;
 }

 bool gprs2::hasGprs() {
    char  vRes[15], vTmpStr[10];
    mstr _mstr;

    _doCmd(F("AT+CGATT?"));
    getAnswer3(vRes,20);

    strcpy_P(vTmpStr, PSTR("+CGATT: 1")); 

    if (_mstr.indexOf(vRes,vTmpStr) == -1) {
      _setLastError(__LINE__,vRes);      
      return false;
    };

      return true;
 }

 bool gprs2::isGprsUp() {
    char  vRes[35],vTmpStr[15];
    mstr _mstr;
    _doCmd(F("AT+SAPBR=2,1"));
    getAnswer3(vRes,15);
    _mstr.trim(vRes,' ');

    
    /********************************
     * Hasn't  status info          *
     ********************************/
    strcpy_P(vTmpStr, PSTR("+SAPBR: 1,1")); 

    if (_mstr.indexOf(vRes,vTmpStr)==-1) {
      _setLastError(__LINE__,vRes);
      return false;
    };

    return true;   
 }


bool gprs2::gprsUp() {

     char vRes[25],vTmpStr[25];
     long int vCurrTime;
     mstr _mstr;

    while (!hasGprs() && millis() - vCurrTime < 15000) {
      delay(1000);
    };

    if (!hasGprs()) {
      return false;
    };

    if (isGprsUp()) {
      return true;
    };
    _doCmd(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""));
    getAnswer3(vRes,25);

//    strcpy_P(vTmpStr, (char*)pgm_read_word(&(ans_bank[0])));     
strcpy_P(vTmpStr, (char*)OK_M);

    if (_mstr.indexOf(vRes,vTmpStr)==-1) {
     _setLastError(__LINE__,vRes);
      return false;
    };

    _modem.print(F("AT+SAPBR=3,1,\"APN\",\""));
    _modem.print(_apn);
    _modem.println(F("\""));
    _modem.flush();
    getAnswer3(vRes,25);

strcpy_P(vTmpStr, (char*)OK_M);
    if (_mstr.indexOf(vRes,vTmpStr)==-1) {
     _setLastError(__LINE__,vRes);
      return false;
    };

    _modem.print(F("AT+SAPBR=3,1,\"USER\",\""));
    _modem.print(_login);
    _modem.println(F("\""));
    _modem.flush();
    getAnswer3(vRes,25);

strcpy_P(vTmpStr, (char*)OK_M);
    if (_mstr.indexOf(vRes,vTmpStr)==-1) {
     _setLastError(__LINE__,vRes);
      return false;
    };

    _modem.print(F("AT+SAPBR=3,1,\"PWD\",\""));
    _modem.print(_pass);
    _modem.println(F("\""));
    _modem.flush();
    getAnswer3(vRes,25);

strcpy_P(vTmpStr, (char*)OK_M);
    if (_mstr.indexOf(vRes,vTmpStr)==-1) {
     _setLastError(__LINE__,vRes);
      return false;

    };

    _doCmd(F("AT+SAPBR=1,1"));
    getAnswer3(vRes,25);
    
    return isGprsUp();
}


void gprs2::_sendTermCommand() {
     char vRes[20];
     _doCmd(F("AT+HTTPTERM"));
     getAnswer3(vRes,20);
};

bool gprs2::isConnect() {
     char vTmpStr[5],
          vRes[30];
     mstr _mstr;

     _doCmd(F("AT+CREG?"));
     getAnswer3(vRes,30);

     strcpy_P(vTmpStr, PSTR(",1")); 

     if (_mstr.indexOf(vRes,vTmpStr)==-1) {
       _setLastError(__LINE__,vRes);
        return false;
     };
   return true;
 }

 bool gprs2::powerUp() {
   char vRes[30];
   _modem.println(F("AT+CFUN=1"));
   _modem.flush();
   getAnswer3(vRes,30);   
   return isPowerUp();
  }

 bool gprs2::powerDown() {
   char vRes[30];
   _modem.println(F("AT+CFUN=0"));
   _modem.flush();
   getAnswer3(vRes,30);

    return isPowerUp();
  }



bool gprs2::canDoPostUrl() {
      return hasNetwork(3) && isReady() && gprsUp();
};

  bool gprs2::postUrl(char* iUrl, char* iPar, char* oRes) {
    return postUrl(iUrl,iPar,oRes,250);
  }
  bool gprs2::postUrl(char* iUrl, char* iPar, char* oRes,unsigned int iResLength) {
      char _tmpStr[30];
      unsigned int vFactSize  = 0,
                  vFirstSpace = 0;
      unsigned long vCurrTime = 0;
      mstr _mstr;

       if (iResLength < 35) {
          _setSizeParamError(__LINE__);
          _sendTermCommand();
          _emptyBuffer(oRes,iResLength);
          return false;
       };

	if (!canDoPostUrl()) {
             /*************************************************************
              * Здесь не следует ставить, сохранение информации об ошибке.*
              * Она будет уже заполнена в предыдущих полях.               *
              *************************************************************/
             _emptyBuffer(oRes,iResLength);
	     return false;
	};


        vFactSize = strlen(iPar);
        Serial.println(iPar);
	_doCmd(F("AT+HTTPINIT"));
        getAnswer3(oRes,iResLength);
        _doCmd(F("AT+HTTPPARA=\"CID\",1"));
        getAnswer3(oRes,iResLength);
        _doCmd(F("AT+HTTPPARA=\"CONTENT\",\"application/x-www-form-urlencoded\""));
        getAnswer3(oRes,iResLength);

        _modem.print(F("AT+HTTPPARA=\"URL\",\""));
        _modem.print(iUrl);
        _modem.println(F("\""));
        _modem.flush();
        getAnswer3(oRes,iResLength);


        _modem.println(F("AT+HTTPPARA=\"REDIR\",0"));
        _modem.flush();
        getAnswer3(oRes,iResLength);

        /***************************
         *AT+HTTPDATA=%u,30000     *
         ***************************/
       if (vFactSize>0) {
           _modem.print(F("AT+HTTPDATA="));
           _modem.print(vFactSize);
           _modem.println(F(",3000"));
           _modem.flush();
           getAnswer3(oRes,iResLength);
           strcpy_P(_tmpStr, PSTR("DOWNLOAD")); 

           if (_mstr.indexOf(oRes,_tmpStr)==-1) {
             _setLastError(__LINE__,oRes);
             _sendTermCommand();
             _emptyBuffer(oRes,iResLength);
             return false;
           }; 

         _doCmd(iPar);
         getAnswer3(oRes,iResLength);
        strcpy_P(_tmpStr, (char*)OK_M);

         if (_mstr.indexOf(oRes,_tmpStr)==-1) {
           _setLastError(__LINE__,oRes);
           _sendTermCommand();
           _emptyBuffer(oRes,iResLength);
           return false;                   
         };
       };

     _doCmd(F("AT+HTTPACTION=1"));
      getAnswer3(oRes,iResLength);
       vCurrTime = millis();
       strcpy_P(_tmpStr,PSTR("+HTTPACTION")); 

       while (_mstr.indexOf(oRes,_tmpStr)==-1 && millis() - vCurrTime < 30000) {
        getAnswer3(oRes,iResLength);
       };     


         strcpy_P(_tmpStr, PSTR("+HTTPACTION: 1,200")); 

        
         if (_mstr.indexOf(oRes,_tmpStr)==-1) {
            
              /************************************************************
               * Проверяем, что ответ сервера не содержит редирект по 302 *
               ************************************************************/
              strcpy_P(_tmpStr, PSTR("+HTTPACTION: 1,302")); 
              if (_mstr.indexOf(oRes,_tmpStr)==-1) {
                       /*********************************************************
                        * Возможна ошибка сервера. Поэтому                      *
                        * проверяем, что модем не возвращает 500 ошибку         *
                        *********************************************************/
                        strcpy_P(_tmpStr, PSTR("+HTTPACTION: 1,500"));             

                        if (_mstr.indexOf(oRes,_tmpStr)>0) {
                          _setLastError(__LINE__,oRes);
                        } else {
                          _setLastError(__LINE__,oRes);
                        };

                        _sendTermCommand();
                       _emptyBuffer(oRes,iResLength);
                       return false;
            };
         };



      _doCmd(F("AT+HTTPREAD"));
      getAnswer3(oRes,iResLength);    

      _mstr.trim(oRes,' ');
      


       if (oRes[strlen(oRes)-2]!='O' && oRes[strlen(oRes)-1]!='K') {
           _setLastError(__LINE__,oRes);
           _emptyBuffer(oRes,iResLength);
           _sendTermCommand();
           return false;
       };

      oRes[strlen(oRes)-1] = '\0';
      oRes[strlen(oRes)-1] = '\0';

      vFirstSpace = _mstr.indexOf(oRes,":");
      vFirstSpace = _mstr.indexOf(oRes," ",vFirstSpace+2);

      for (int vI = 0; vI < vFirstSpace; vI++) {
          oRes[vI] = ' ';
      };

      _mstr.trim(oRes,' ');
     _sendTermCommand();
   return true;

  }


bool gprs2::saveOnSms() {
   char vRes[30], vTmpStr[30];
   mstr _mstr;
   _doCmd(F("AT+CNMI=1,1,0,0,0"));
   getAnswer3(vRes,30);
strcpy_P(vTmpStr, (char*)OK_M);
   return _mstr.indexOf(vRes,vTmpStr) > -1;
}

bool gprs2::_setSmsTextMode() {
  char vTmpStr[20],vRes[20];
  mstr _mstr;

  _doCmd(F("AT+CMGF=1"));
  getAnswer3(vRes,20);
strcpy_P(vTmpStr, (char*)OK_M);
  return  _mstr.indexOf(vRes,vTmpStr)>-1;
}

void gprs2::getSmsText(unsigned int iNum,char* oRes) {
  char vCommand[15];
  sprintf_P(vCommand,PSTR("AT+CMGR=%u,1"),iNum);
  _doCmd(vCommand);
  _getAnswer3(oRes,200,true);     
}



void gprs2::_setSizeParamError(unsigned int iErrorNum) {
    _emptyBuffer(_lastError,20);  
     strcpy_P(_lastError,PSTR("Param is to small"));
    _lastErrorNum = iErrorNum;
}
void gprs2::_setLastError(unsigned int iErrorNum,char* iErrorText) {
    _emptyBuffer(_lastError,20);  

    if (strlen(iErrorText)>2) {     
       strncpy(_lastError,iErrorText,20);
    } else {
      sprintf_P(_lastError,PSTR("N %d"),iErrorNum);
    };

  _lastErrorNum  = iErrorNum;

}



void gprs2::getLastError(char* oRes) {
      strncpy(oRes,_lastError,19);
      oRes[19] = 0;
 }

uint8_t gprs2::getLastErrorNum() {
   return _lastErrorNum;
 }


void gprs2::_emptyBuffer(char* oBuf,int iSize) {
 memset(oBuf,0,iSize);
}

bool gprs2::deleteAllSms() {
   char vRes[10],
        _tmpStr[20];
   mstr _mstr;

  _doCmd(F("AT+CMGDA=\"DEL ALL\""));
  getAnswer3(vRes,20);

strcpy_P(_tmpStr, (char*)OK_M);
   if (_mstr.indexOf(vRes,_tmpStr)>-1) {
        return true;
   };

  return false;

};

bool gprs2::deleteSms(byte iSms) {
   char vRes[20],
        _tmpStr[10];
   mstr _mstr;

  _modem.print(F("AT+CMGD="));
  _modem.println((int)iSms);
  _modem.flush();
  getAnswer3(vRes,20);

strcpy_P(_tmpStr, (char*)OK_M);
   if (_mstr.indexOf(vRes,_tmpStr)>-1) {
        return true;
   };

  return false;


};

bool gprs2::hasNetwork(byte iWaitAttempt) {
     char vTmpStr[20],
          vRes[20];

     byte vAttempt;
     mstr _mstr;
                                       
    strcpy_P(vTmpStr, PSTR(" 0,0"));  // Пробел обязателен. Без пробела под ошибку попадет 20,0

    for (vAttempt = 0; vAttempt < iWaitAttempt; vAttempt++) {
          delay(vAttempt * 40000);
         _doCmd(F("AT+CSQ"));
         getAnswer3(vRes,20);  
 
       if (_mstr.indexOf(vRes,vTmpStr)==-1) {
         return true;
       };      
    };
      _setLastError(__LINE__,vRes);
      return false;
}
bool gprs2::isReady() {
  unsigned long vCurrTime;

  vCurrTime = millis();           
  while (!isPowerUp() && millis() - vCurrTime < 15000) {
    delay(1000);
  };

  if (!isPowerUp()) {
    return false;
  };


  vCurrTime = millis();
  while (!isConnect() && millis() - vCurrTime < 15000) {
    delay(1000);
  };

  if (!isConnect()) {
    return false;
  };
      
 return true;
};

bool gprs2::sendSms(char* iPhone,char* iText) {
  char vTmpStr[20],vRes[20];
  mstr _mstr;


  _modem.print(F("AT+CMGS=\""));
  _modem.print(iPhone);
  _modem.println("\"");
  getAnswer3(vRes,20);

  
  strcpy_P(vTmpStr, PSTR(">"));   
  if (_mstr.indexOf(vRes,vTmpStr) == -1) {
    _setLastError(__LINE__,vRes);
    return false;  
  };

  _modem.println(iText);
  _modem.write(0x1A);
  getAnswer3(vRes,20);

  strcpy_P(vTmpStr, PSTR("+CMGS:"));   
  if (_mstr.indexOf(vRes,vTmpStr)) {
    _setLastError(__LINE__,vRes);
    return false;
  };
  
  return true; 
}

int gprs2::getSmsCount() {
 char vTmpStr[30],vRes[200];
 int vCount = 0;
  mstr _mstr;


    for (unsigned int vI=1; vI<6; vI++) {
         getSmsText(vI,vRes);  

         _mstr.trim(vRes,' ');

strcpy_P(vTmpStr, (char*)OK_M);

         if (!_mstr.isEqual(vRes,vTmpStr)) {
             vCount++;
         };

    };

return vCount;
};

int gprs2::_freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

 bool gprs2::setOnSms(void (*iSmsEvent)(char* oStr)) {
    _onSmsRead = iSmsEvent;
 }

 void gprs2::readSms() {
      readSms(false);
 };

 void gprs2::readSms(bool deleteAfterRead) {
  char vRes[200], 
       vCommand[100];   
  mstr _mstr;

    _setSmsTextMode();
    saveOnSms();



    for (unsigned int vI=1; vI<6; vI++) {

         getSmsText(vI,vRes);
        _emptyBuffer(vCommand,100);
               
         if (strlen(vRes) < 20) {
            continue;
         };

        Serial.print(F("-SMS "));
        Serial.print(vI);
        Serial.print(F(": "));
        Serial.println(vRes);
        Serial.println(F("-"));

        _getSmsBody(vRes,vCommand);

        if (_onSmsRead) {
         if (strlen(vCommand) > 0) {
            if (!_onSmsRead(vI,vCommand)) {                 
                if (deleteAfterRead) {
                    deleteSms(vI);
                };
        	continue;
            };
         };
      };

       if (deleteAfterRead) {
           deleteSms(vI);
       };

   };


 }

void gprs2::_getSmsBody(char* vRes,char* vCommand) {

  unsigned int vCurrPos = 0, vStartPos = 0, vSmsLength;
  bool vSmsBegin = false;
  char vTmpStr[5];
  mstr _mstr;
         vSmsLength = strlen(vRes);

         strcpy_P(vTmpStr, PSTR("+CMGR:"));
         vStartPos = _mstr.indexOf(vRes,vTmpStr);

	      for (unsigned int vJ=vStartPos; vJ<vSmsLength; vJ++) {
                  if (vSmsBegin) {
                     if (vRes[vJ]!='\n' && vRes[vJ]!='\r' && vRes[vJ]!='\0') {
                          vCommand[vCurrPos] = vRes[vJ];        
                          vCurrPos++;
                     };
                  };

                  if (vRes[vJ]=='\n') {

                    if (vSmsBegin) {
                      break;
                    };

                    vSmsBegin = true;  
                  };

                  if (vCurrPos>98) {
                     break;
                  };

	      };
              vCommand[vCurrPos+1] = '\0';     
              vSmsBegin = false;
              vCurrPos = 0;

}
void gprs2::_doCmd(char* iStr) {
  _modem.println(iStr);
  _modem.flush();
};

void gprs2::_doCmd(const __FlashStringHelper *iStr) {
  _modem.println(iStr);
  _modem.flush();
}

void gprs2::softRestart() {
    char vRes[20],vTmpStr[20];
    mstr _mstr;

    Serial.print(F("*REST* : "));
   _doCmd(F("AT+CFUN=1,1"));
    getAnswer3(vRes,20);
    Serial.print(vRes);
    Serial.println(F("*"));
    Serial.flush();

strcpy_P(vTmpStr, (char*)OK_M);

    if (_mstr.indexOf(vRes,vTmpStr)==-1) {
      _setLastError(__LINE__,vRes);
      return false;
    };

    return true;
}

 void gprs2::getCoords(char* oLongitude,char* oLatitdue) {
     char vRes[50],vUnit[10];
     mstr _mstr;
    _doCmd(F("AT+CIPGSMLOC=1,1"));
    getAnswer3(vRes,50);
    _emptyBuffer(oLongitude,10);
    _emptyBuffer(oLatitdue,10);
    if (_mstr.numEntries(vRes,',')>0) {
        _mstr.entry(2,vRes,',',vUnit);
        strcpy(oLongitude,vUnit);
        _mstr.entry(3,vRes,',',vUnit);
        strcpy(oLatitdue,vUnit);
     };    
 };