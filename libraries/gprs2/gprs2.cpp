#include <Arduino.h>
#include <SoftwareSerial.h>
#include <gprs2.h>
#include <mstr.h>
#include <MemoryFree.h>
#include <debug.h>


const char OK_M[] PROGMEM = "OK";
const char SPACE_M[] PROGMEM = " ";
const char COMMA_M[] PROGMEM = ",";



gprs2::gprs2(int pin1,int pin2):_modem(pin1,pin2) {
   // Module revision Revision:1137B06SIM900M64_ST_ENHANCE
   _modem.begin(19200); 
};


void gprs2::setInternetSettings(char* iApn,char* iLogin,char* iPass) {
     _apn   = String(iApn);
     _login = String(iLogin);
     _pass  = String(iPass);
}



bool gprs2::getAnswer3(char* oRes,size_t iSize) {
  return _getAnswer3(oRes,iSize,false);
};


bool gprs2::_getAnswer3(
                        char* oRes,
                        size_t iSize,
                        bool saveCRLF,
                        unsigned long iTimeOut
                       ) {
  unsigned long vCurrTime = millis(), 
                vLastReadTime=0;
  size_t vI = 0;
  bool wasRead=false;


 _emptyBuffer(oRes, iSize);


  do {

     while (_modem.available()) {
      char vChr  = _modem.read();

      if (!isAscii(vChr) || vI > iSize - 3) { //Длина строки sizeof, соответственно последняя позиция будет sizeof-1
          /***********************************************************
          // Если в результате работы получили не Ascii символ       *
          // или уже вычитали доступное количество элементов,        *
          // то просто вычитываем буффер.                            *
          ***********************************************************/
          continue;
      };

      switch (vChr) {
        case '\0':
        case '\r':
        break;
        case '\n':
        case ' ' :
           if (!saveCRLF) {
               /***********************************************
                * Убираем начальные и подряд идующие пробелы. *
                ***********************************************/
              if (vI > 0 && oRes[vI-1]!=' ') {
                 oRes[vI] = ' ';
                 vI++;
              };

           } else {
	     if (vChr) {
              oRes[vI] = vChr;
              vI++;
             };
           };
          break;
         default:
   	      //TCCR1B = _BV(WGM13);  // Как крайний случай при начала чтения отключаем таймер
              oRes[vI] = vChr;
              vI++;
              wasRead = true;             //Учитываем только значимые символы
              vLastReadTime = millis(); 
          break;
      };
     }; // end while _modem.available
   } while ((!wasRead && millis() - vCurrTime < iTimeOut) || (wasRead && millis() - vLastReadTime < 5000UL)); // Читаем пока не прошло 2 секунды, или если что-то считали, то пока между символами не пройдет > 5с (такой большой лаг нужен на случай прерывания)


  if (wasRead) {
      //Ставим нуль терминатор, заодно убираем концевой пробел
      if (oRes[vI-1]==' ') { oRes[vI] = '\0'; oRes[vI-1] = '\0'; } else { oRes[vI] = '\0'; };
   };   

  #if IS_DEBUG>1
    Serial.print(millis()); Serial.print(F("  :")); Serial.println(oRes); Serial.flush();
  #endif
  return wasRead;         
}                                     

bool gprs2::_getAnswerWait(char* oRes,
                           size_t iSize,
                           char* iNeedStr,
                           bool iSaveCRLF,
                           unsigned long iTimeOut) {
     byte vAttempt = 0;
     bool vStatus;

     mstr _mstr;


     do {
       _getAnswer3(oRes,iSize,iSaveCRLF,iTimeOut);  
       vAttempt++;
     }  while ((vStatus=_mstr.indexOf(oRes,iNeedStr)==-1) && vAttempt < 5);

   return vStatus;
}

bool gprs2::_getAnswerWaitLong(char* oRes,
	                       size_t iSize,
	                       char* iNeedStr,
                               unsigned long iTimeOut) {
     mstr _mstr;
    _getAnswer3(oRes,iSize,false,iTimeOut);  
     return _mstr.indexOf(oRes,iNeedStr)==-1;
}


 bool gprs2::isPowerUp() {
     char vTmpStr[3],
          vRes[10];
  
    _doCmd(F("AT"));

     strcpy_P(vTmpStr, (char*)OK_M);

    if (_getAnswerWait(vRes,sizeof(vRes),vTmpStr)) {
      _setLastError(__LINE__,vRes);
      return false;
    };


      return true;

 }


bool gprs2::hasGprsNetwork() {
    char  vRes[15], vTmpStr[8];
    mstr _mstr;

     _doCmd(F("AT+CGATT?"));

     strcpy_P(vTmpStr,PSTR("+CGATT:"));
 
    _getAnswerWait(vRes,sizeof(vRes),vTmpStr);

     strcpy_P(vTmpStr, PSTR(": 1")); 

     if (_mstr.indexOf(vRes,vTmpStr)==-1) {
      _setLastError(__LINE__,vRes);      
      return false;
     };

      return true;
 }

bool gprs2::gprsNetworkUp(bool iForce) {
  char vRes[10],
       vTmpStr[3];
  bool hasNetwork=hasGprsNetwork();

   if (!hasNetwork || iForce) {
     _doCmd(F("AT+CGATT=1"));    

      strcpy_P(vTmpStr, (char*)OK_M);
      if (_getAnswerWait(vRes,sizeof(vRes),vTmpStr)) {
       _setLastError(__LINE__,vRes);      
        return false;
      };

     hasNetwork=hasGprsNetwork();
   };

  return hasNetwork;
};

bool gprs2::gprsNetworkDown() {
  char vRes[10],
       vTmpStr[10];

       strcpy_P(vTmpStr, (char*)OK_M);

      _doCmd(F("AT+CGATT=0"));

      if (_getAnswerWait(vRes,sizeof(vRes),vTmpStr)) {
       _setLastError(__LINE__,vRes);      
        return false;
      };

  return !hasGprsNetwork();

};


bool gprs2::hasGprs() {
    char  vRes[35],vTmpStr[12];
    mstr _mstr;


    _doCmd(F("AT+SAPBR=2,1"));

    strcpy_P(vTmpStr, PSTR("+SAPBR:")); 

    if (_getAnswerWait(vRes,sizeof(vRes),vTmpStr)) {
      _setLastError(__LINE__,vRes);      
      return false;
    };
        
    strcpy_P(vTmpStr, PSTR(": 1,1,")); 

    if (_mstr.indexOf(vRes,vTmpStr)==-1) {
      _setLastError(__LINE__,vRes);      
      return false;
    };

    return true;   
 }

bool gprs2::gprsUp(bool iForce) {

     char vRes[10],vTmpStr[3];
     mstr _mstr;

     strcpy_P(vTmpStr, (char*)OK_M);

                 
    _doCmd(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""));

    if (_getAnswerWait(vRes,sizeof(vRes),vTmpStr)) {
     _setLastError(__LINE__,vRes);
      return false;
    };

    _doCmd3(F("AT+SAPBR=3,1,\"APN\",\""),_apn,F("\""));
   
    if (_getAnswerWait(vRes,sizeof(vRes),vTmpStr)) {
     _setLastError(__LINE__,vRes);
      return false;
    };

    _doCmd3(F("AT+SAPBR=3,1,\"USER\",\""),_login,F("\""));

    if (_getAnswerWait(vRes,sizeof(vRes),vTmpStr)) {
     _setLastError(__LINE__,vRes);
      return false;
    };

    _doCmd3(F("AT+SAPBR=3,1,\"PWD\",\""),_pass,F("\""));


    if (_getAnswerWait(vRes,sizeof(vRes),vTmpStr)) {
     _setLastError(__LINE__,vRes);
      return false;
    };

    _doCmd(F("AT+SAPBR=1,1"));

    if (_getAnswerWaitLong(vRes,sizeof(vRes),vTmpStr,5000)) {
     _setLastError(__LINE__,vRes);
      return false;
    };

    
    return true;
}

bool gprs2::gprsDown() {
  char vRes[10],
       vTmpStr[10];

       strcpy_P(vTmpStr, (char*)OK_M);

      _doCmd(F("AT+SAPBR=0,1"));

      if (_getAnswerWait(vRes,sizeof(vRes),vTmpStr)) {
       _setLastError(__LINE__,vRes);      
        return false;
      };

  return !hasGprs();

};




void gprs2::_sendTermCommand(bool iWaitOK) {
     char vRes[10],
          vTmpStr[6];
     bool vStatus;

      if (iWaitOK) {
         strcpy_P(vTmpStr, (char*)OK_M);
      } else {
         strcpy_P(vTmpStr, PSTR("ERROR"));
      };

      _doCmd(F("AT+HTTPTERM"));
      vStatus= _getAnswerWait(vRes,sizeof(vRes),vTmpStr);
};

bool gprs2::isConnect() {
     char vTmpStr[8],
          vRes[20];
     mstr _mstr;

     _doCmd(F("AT+CREG?"));
     strcpy_P(vTmpStr, PSTR("+CREG")); 

     if (_getAnswerWait(vRes,sizeof(vRes),vTmpStr)) {
        _setLastError(__LINE__,vTmpStr);       
        return false;
     };

       strcpy_P(vTmpStr, PSTR("0,1")); 
       if (_mstr.indexOf(vRes,vTmpStr)==-1) {
          _setLastError(__LINE__,vRes);
          return false;
       };

   return true;
 }


bool gprs2::canWork() {
   return isPowerUp() && hasNetwork() && isConnect();
};

bool gprs2::doInternet() {

         char vRes[4],
              vTmpStr[4];
         unsigned int vResLength=sizeof(vRes);

	if (!gprsNetworkUp()) {
	   return false;
	 };

	 if(!hasGprs()) {
            gprsUp();
	 };
  
        if (!hasGprs()) {
           return false;
        };

 
	/*****************************************************
         * Чаще всего интернет не поднят,                    *
         * поэтому при принудительном отключении             *
         * проверяем на ошибку чтобы сэкономить время.       *
         *****************************************************/
        _sendTermCommand(false);


        strcpy_P(vTmpStr, (char*)OK_M);
	_doCmd(F("AT+HTTPINIT"));


        if(_getAnswerWait(vRes,vResLength,vTmpStr,false,15000)) {
           _sendTermCommand();
           _setLastError(__LINE__,vRes);
           _emptyBuffer(vRes,vResLength);
            return false;
        };

        _doCmd(F("AT+HTTPPARA=\"CID\",1"));


        if(_getAnswerWait(vRes,vResLength,vTmpStr)) {
             _setLastError(__LINE__,vRes);
             _sendTermCommand();
             _emptyBuffer(vRes,vResLength);
             return false;
        };

        _doCmd(F("AT+HTTPPARA=\"CONTENT\",\"application/x-www-form-urlencoded\""));
        if(_getAnswerWait(vRes,vResLength,vTmpStr)) {
             _setLastError(__LINE__,vRes);
             _sendTermCommand();
             _emptyBuffer(vRes,vResLength);
             return false;
        };


        _doCmd(F("AT+HTTPPARA=\"REDIR\",0"));
        if(_getAnswerWait(vRes,vResLength,vTmpStr)) {
             _setLastError(__LINE__,vRes);
             _sendTermCommand();
             _emptyBuffer(vRes,vResLength);
             return false;
        };

  return true;

}

  bool gprs2::postUrl(char* iUrl, char* iPar, char* oRes) {
    return postUrl(iUrl,iPar,oRes,250);
  }

  bool gprs2::postUrl(char* iUrl, char* iPar, char* oRes,unsigned int iResLength) {
      char _tmpStr[20];
      mstr _mstr;


       if (strlen(iUrl)<2) {
           strcpy_P(oRes,PSTR("NURL"));
          _setLastError(__LINE__,oRes);
          _emptyBuffer(oRes,iResLength);
          return false;
       };


       if (iResLength < 35) {
           strcpy_P(oRes,PSTR("IRS"));
           _setLastError(__LINE__,oRes);
          _emptyBuffer(oRes,iResLength);
          return false;
       };

        strcpy_P(_tmpStr, (char*)OK_M);


	_doCmd3(F("AT+HTTPPARA=\"URL\",\""),iUrl,F("\""));
        if(_getAnswerWait(oRes,iResLength,_tmpStr)) {
             _setLastError(__LINE__,oRes);
             _emptyBuffer(oRes,iResLength);
             return false;
        };


        /***************************
         *AT+HTTPDATA=%u,30000     *
         ***************************/

       {
         unsigned int vFactSize  = 0;
         vFactSize = strlen(iPar);

         if (vFactSize>0) {

            sprintf(_tmpStr,"%u",vFactSize);          
           _doCmd3(F("AT+HTTPDATA="),_tmpStr,F(",12000"));           

           strcpy_P(_tmpStr, PSTR("DOWNLOAD")); 

           if (_getAnswerWait(oRes,iResLength,_tmpStr)==-1) {
             _setLastError(__LINE__,oRes);
             _emptyBuffer(oRes,iResLength);
             return false;
           }; 

         _doCmd(iPar);

         strcpy_P(_tmpStr, (char*)OK_M);

         if (_getAnswerWait(oRes,iResLength,_tmpStr)) {
           _setLastError(__LINE__,oRes);
           _emptyBuffer(oRes,iResLength);
           return false;                   
         };




       };

      };   //end param block


       _doCmd(F("AT+HTTPACTION=1"));


       strcpy_P(_tmpStr,PSTR("+HTTPACTION")); 

       _getAnswerWait(oRes,iResLength,_tmpStr,false,60000); //Он выдает ОК, потом может долго думать и выдать уже искомвый ответ. Поставил 30 с как обычно в Инете


         strcpy_P(_tmpStr, PSTR("+HTTPACTION: 1,200"));     

         if (_mstr.indexOf(oRes,_tmpStr)==-1) {
              _setLastError(__LINE__,oRes);
              _emptyBuffer(oRes,iResLength);
              return false;
          };


       _doCmd(F("AT+HTTPREAD"));
       strcpy_P(_tmpStr, PSTR("+HTTPREAD"));             
         if (_getAnswerWait(oRes,iResLength,_tmpStr)) {
           _setLastError(__LINE__,oRes);
           _emptyBuffer(oRes,iResLength);
           return false;                   
         };

     
       strcpy_P(_tmpStr,(char*)OK_M);             
       if (_mstr.indexOf(oRes,_tmpStr)==-1) {
           _setLastError(__LINE__,oRes);
           _emptyBuffer(oRes,iResLength);
           return false;
       };

 {
      char vSplitChar[2];
      unsigned int  vFirstSpace = 0;
      int vResLength=0;

      /* Убираем крайний OK */
     vResLength=strlen(oRes) - 3;
     vResLength=max(0,vResLength);
     oRes[vResLength] = 0;

     vResLength=strlen(oRes);

     /* Результат пустой */

      if (vResLength==0) {
           _setLastError(__LINE__,_tmpStr);
           _emptyBuffer(oRes,iResLength);
           return false;
      };

      // Здесь мы наши первый разделитель двоеточие, после него по станадрту идет пробел и кол-во байт
      strcpy_P(vSplitChar,PSTR(":"));
      vFirstSpace = _mstr.indexOf(oRes,vSplitChar);

      //Здесь мы нашли пробел после кол-ва байт
      strcpy_P(vSplitChar, (char*)SPACE_M);
      vFirstSpace = _mstr.indexOf(oRes,vSplitChar,vFirstSpace+2);       

     _mstr.leftShift2(oRes,vFirstSpace+1);
 };

     return true;

  }


bool gprs2::saveOnSms() {
   char vRes[10],
        vTmpStr[3];
   mstr _mstr;
   _doCmd(F("AT+CNMI=2,0,0,0,0"));
   getAnswer3(vRes,sizeof(vRes));
   strcpy_P(vTmpStr, (char*)OK_M);
   return _mstr.indexOf(vRes,vTmpStr) > -1;
}

bool gprs2::_setSmsTextMode() {
  char vTmpStr[3],
       vRes[10];

  _doCmd(F("AT+CMGF=1"));
  strcpy_P(vTmpStr, (char*)OK_M);
  return  _getAnswerWait(vRes,sizeof(vRes),vTmpStr)>-1;
}

void gprs2::getSmsText(unsigned int iNum,char* oRes,unsigned int iSmsSize) {
   char vCommand[13],vTmpStr[3];
   sprintf_P(vCommand,PSTR("AT+CMGR=%u,1"),iNum);
   _doCmd(vCommand);
   strcpy_P(vTmpStr,(char*)OK_M);
  _getAnswerWait(oRes,iSmsSize,vTmpStr,true);     
}


void gprs2::_setLastError(unsigned int iErrorNum,char* iErrorText) {
/*
    if (strlen(iErrorText)>2) {     
         _lastError=String(iErrorText);
    } else {
         _lastError=String(iErrorNum);
    };*/
 _lastError=String(iErrorNum);

  _lastErrorNum  = iErrorNum;
}



void gprs2::getLastError(char* oRes) {
        _lastError.toCharArray(oRes,20);
 }

uint8_t gprs2::getLastErrorNum() {
   return _lastErrorNum;
 }


void gprs2::_emptyBuffer(char* oBuf,size_t iSize) {
 memset(oBuf,'\0',iSize);
}

bool gprs2::deleteAllSms() {
   char vRes[10],
        _tmpStr[20];
   mstr _mstr;

  _doCmd(F("AT+CMGDA=\"DEL ALL\""));
  getAnswer3(vRes,sizeof(vRes));

strcpy_P(_tmpStr, (char*)OK_M);
   if (_mstr.indexOf(vRes,_tmpStr)>-1) {
        return true;
   };

  return false;

};

bool gprs2::deleteSms(byte iSms) {
   char vRes[8],_tmpStr[3];
   mstr _mstr;

  _modem.print(F("AT+CMGD="));
  _modem.flush();
  _modem.println((int)iSms);
  _modem.flush();
  getAnswer3(vRes,sizeof(vRes));

  strcpy_P(_tmpStr, (char*)OK_M);
   return _mstr.indexOf(vRes,_tmpStr)!=-1;
};

bool gprs2::hasNetwork() {
     char vTmpStr[6],vRes[20];
     mstr _mstr;


    _doCmd(F("AT+CSQ"));

     strcpy_P(vTmpStr, PSTR("+CSQ:")); 

       if (_getAnswerWait(vRes,sizeof(vRes),vTmpStr)) {
         _setLastError(__LINE__,vRes);
         return false;
       };      

       strcpy_P(vTmpStr, PSTR(" 0,0")); // Ведущий пробел обязателен. Без пробела под ошибку попадет 20,0

       if (_mstr.indexOf(vRes,vTmpStr)>-1) {
        _setLastError(__LINE__,vRes);
         return false;
       };

      return true;
}

bool gprs2::sendSms(char* iPhone,char* iText) {
  char vTmpStr[8],vRes[20];
  mstr _mstr;


  _modem.print(F("AT+CMGS=\""));
    _modem.flush();
  _modem.print(iPhone);
    _modem.flush();
  _modem.println("\"");
  _modem.flush();
  getAnswer3(vRes,sizeof(vRes));

  
  strcpy_P(vTmpStr, PSTR(">"));   
  if (_mstr.indexOf(vRes,vTmpStr) == -1) {
    _setLastError(__LINE__,vRes);
    return false;  
  };

  _modem.println(iText);
  _modem.write(0x1A);
  getAnswer3(vRes,sizeof(vRes));

  strcpy_P(vTmpStr, PSTR("+CMGS:"));   
  if (_mstr.indexOf(vRes,vTmpStr)) {
    _setLastError(__LINE__,vRes);
    return false;
  };
  
  return true; 
}


bool gprs2::setOnSms(bool (*iSmsEvent)(byte iSms, char* oStr)) {
    _onSmsRead = iSmsEvent;
}

bool gprs2::setBeforePostParams(bool (*ifunction)(char* iStr)) {
    _onBeforePostParams = ifunction;
}

bool gprs2::setAfterPostParams(bool (*ifunction)()) {
    _onAfterPostParams = ifunction;
}




 bool gprs2::readSms(bool deleteAfterRead) {
 char vRes[200];

    for (unsigned int vI=1; vI<6; vI++) {
         _emptyBuffer(vRes,sizeof(vRes));

         getSmsText(vI,vRes,sizeof(vRes));

         #if IS_DEBUG>1
           Serial.print(vI);
           Serial.print(F(":"));
           Serial.println(vRes);
         #endif

         if (strlen(vRes) < 20) {
               /********************************************************************
                *Возможно ситуация, когда SMS  содержит текст на кирилице          *
                *этот текст будет кодироваться существенно больше 200 символов     *
                * в результате в итоговую строку не попадет завершаюший OK СМС.    *
                * Это приведет к тому, что СМС будет считаться как отсутствующее,  *
                * но при этом память СМС будет занята. Поэтому несмотря на то, что *
                * СМС не считана память следует очистить.                          *
                ********************************************************************/
	       if (deleteAfterRead) {
	           deleteSms(vI);
	       };
            continue;
         }; 


        if (!_clearSmsBody(vRes,sizeof(vRes))) { continue; };

        if (_onSmsRead) {
            if (!_onSmsRead(vI,vRes)) {                 
                if (deleteAfterRead) {
                    deleteSms(vI);
                };
        	continue;
            };
      };

       if (deleteAfterRead) {
           deleteSms(vI);
       };

   };

  return true;
 }


void gprs2::_doCmd(char* iStr) {
  _modem.println(iStr);
  _modem.flush();
};


void gprs2::_doCmd(const __FlashStringHelper *iStr) {
  #if IS_DEBUG>1
   Serial.println(iStr);
  #endif
  _modem.println(iStr);
  _modem.flush();
}

void gprs2::_doCmd3(const __FlashStringHelper *iStr1,char* iStr2,const __FlashStringHelper *iStr3) {

  #if IS_DEBUG>1
    Serial.print(iStr1);
    Serial.flush();
    Serial.print(iStr2);
    Serial.flush();
    Serial.println(iStr3);
    Serial.flush();
  #endif
 
  _modem.print(iStr1);
  _modem.flush();
  _modem.print(iStr2);
  _modem.flush();
  _modem.println(iStr3);
  _modem.flush();
}

void gprs2::_doCmd3(const __FlashStringHelper *iStr1,String iStr2,const __FlashStringHelper *iStr3) {

  #if IS_DEBUG>1
    Serial.print(iStr1);
    Serial.flush();
    Serial.print(iStr2);
    Serial.flush();
    Serial.println(iStr3);
    Serial.flush();
  #endif

  _modem.print(iStr1);
  _modem.flush();
  _modem.print(iStr2);
  _modem.flush();
  _modem.println(iStr3);
  _modem.flush();
}


bool gprs2::softRestart() {
    char vRes[20],vTmpStr[5];
    mstr _mstr;

   _doCmd(F("AT+CFUN=1,1"));
    getAnswer3(vRes,sizeof(vRes));


    strcpy_P(vTmpStr, (char*)OK_M);

    if (_mstr.indexOf(vRes,vTmpStr)==-1) {
      _setLastError(__LINE__,vRes);
      return false;
    };

    return true;
}

void gprs2::hardRestart() {
 pinMode(6,OUTPUT);
 digitalWrite(6,LOW);  
 delay(1000);
 pinMode(6,INPUT);
}

 bool gprs2::getCoords(char* oLongitude,char* oLatitdue,char* oRes,size_t iSize) {
     char vTmpStr[15];
     mstr _mstr;

    _emptyBuffer(oLongitude,11);
    _emptyBuffer(oLatitdue,11);

    _doCmd(F("AT+CIPGSMLOC=1,1"));

    strcpy_P(vTmpStr, PSTR("+CIPGSMLOC:"));

    if (_getAnswerWait(oRes,iSize,vTmpStr)) {
       _setLastError(__LINE__,oRes);
       return false;
    };



    strcpy_P(vTmpStr, PSTR("CIPGSMLOC: 0"));

    if (_mstr.indexOf(oRes,vTmpStr)==-1) {
       _setLastError(__LINE__,oRes);
       return false;
    };


    strcpy_P(vTmpStr, (char*)COMMA_M);

    if (_mstr.numEntries(oRes,vTmpStr)<3) {
       _setLastError(__LINE__,oRes);
       return false;
    };

      _mstr.entry(2,oRes,vTmpStr,10,oLongitude);
      _mstr.entry(3,oRes,vTmpStr,10,oLatitdue);

     _emptyBuffer(oRes,iSize);

      return true;
 };


bool gprs2::wakeUp() {   
      pinMode(4,OUTPUT);
      digitalWrite(4,HIGH);
      return true;
};

void gprs2::sleep() {   
      pinMode(4,OUTPUT);
      digitalWrite(4,LOW);
};

bool gprs2::_clearSmsBody(char* iRes,unsigned int iSize) {
	char vDelimiter[2];
        int vFirstLFPos;
        mstr _mstr;

        strcpy_P(vDelimiter, PSTR("\n"));     

        /*********************************
         * Убираем ведующие заголовки.   *
         *********************************/
        vFirstLFPos = _mstr.indexOf(iRes,vDelimiter,2);

        if (vFirstLFPos==-1) {
            return false;
        };

        vFirstLFPos++;

        _mstr.leftShift2(iRes,vFirstLFPos);

        /************************************
         * Убираем хвостовые заголовки.     *
         ************************************/

        vFirstLFPos = _mstr.indexOf(iRes,vDelimiter);

        if (vFirstLFPos==-1) {
            return false;
        };

        memset(iRes+vFirstLFPos,0,strlen(iRes)-vFirstLFPos);      

    return true;
}

gprs2::~gprs2() {
 _modem.end();
}