#include <Arduino.h>
#include <SoftwareSerial.h>
#include <gprs2.h>
#include <mstr.h>
#include <debug.h>
#define WAIT_ANSWER_TIME 45000


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
  return _getAnswer3(oRes,iSize,false,false);
};

bool gprs2::_getAnswer3(char* oRes,size_t iSize,bool saveCRLF) {
  return _getAnswer3(oRes,iSize,saveCRLF,false);
};

bool gprs2::_getAnswer3(char* oRes,size_t iSize,bool saveCRLF,bool showAnswer) {
  unsigned long vCurrTime = millis(), 
                vLastReadTime=0;
  size_t vI = 0;
  bool wasRead=false;


 _emptyBuffer(oRes, iSize);
  do {

    while (_modem.available()) {

      char vChr  = _modem.read();

      if (!isAscii(vChr) || vI > iSize - 3) {
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
              oRes[vI] = vChr;
              vI++;
          break;
      };
      wasRead = true;
      vLastReadTime = millis(); 
    }; // end while _modem.available


     if (wasRead) {
        oRes[vI] = '\0';
     };    

  } while ((millis() - vCurrTime < 2000UL) && (!wasRead || (wasRead && millis() - vLastReadTime < 750UL)));

  return wasRead;         
}                                     

bool gprs2::_getAnswerWait(char* oRes,size_t iSize,char* iNeedStr,bool iSaveCRLF=false,bool iDebug=false) {
     byte vAttempt = 0;
     bool vStatus;

     mstr _mstr;


     do {
       _getAnswer3(oRes,iSize,iSaveCRLF);  
       vAttempt++;
     }  while ((vStatus=_mstr.indexOf(oRes,iNeedStr)==-1) && vAttempt < 10);

   return vStatus;
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
    char  vRes[12], vTmpStr[8];
    mstr _mstr;

     _doCmd(F("AT+CGATT?"));
     strcpy_P(vTmpStr, PSTR("+CGATT:")); 
 
     if (_getAnswerWait(vRes,sizeof(vRes),vTmpStr)) {
      _setLastError(__LINE__,vRes);      
      return false;
     };

     strcpy_P(vTmpStr, PSTR(": 1")); 
     if (_mstr.indexOf(vRes,vTmpStr)==-1) {
      _setLastError(__LINE__,vRes);      
      return false;
     };

      return true;
 }

bool gprs2::gprsNetworkUp(bool iForce=false) {
  char vRes[10],
       vTmpStr[3];

   strcpy_P(vTmpStr, (char*)OK_M);

   if (!hasGprsNetwork() || iForce) {
     _doCmd(F("AT+CGATT=1"));

      if (_getAnswerWait(vRes,sizeof(vRes),vTmpStr)) {
       _setLastError(__LINE__,vRes);      
        return false;
      };

   };

  return hasGprsNetwork();
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
    char  vRes[35],vTmpStr[7];
    mstr _mstr;

    /********************************
     * Hasn't  status info          *
     ********************************/
    strcpy_P(vTmpStr, PSTR("+SAPBR")); 


    _doCmd(F("AT+SAPBR=2,1"));

    strcpy_P(vTmpStr, PSTR("+SAPBR")); 

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

bool gprs2::gprsUp(bool iForce=false) {

     char vRes[10],vTmpStr[3];
     mstr _mstr;

     strcpy_P(vTmpStr, (char*)OK_M);

    if (!hasGprs() || iForce) {
                  
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

    if (_getAnswerWait(vRes,sizeof(vRes),vTmpStr)) {
     _setLastError(__LINE__,vRes);
      return false;
    };

  };

    
    return hasGprs();
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




void gprs2::_sendTermCommand(bool iWaitOK=true) {
     char vRes[10],
          vTmpStr[6];
     bool vStatus;

      if (iWaitOK) {
         strcpy_P(vTmpStr, (char*)OK_M);
      } else {
         strcpy_P(vTmpStr, PSTR("ERROR"));
      };

      _doCmd(F("AT+HTTPTERM"));
      vStatus=_getAnswerWait(vRes,sizeof(vRes),vTmpStr);
};

bool gprs2::isConnect() {
     char vTmpStr[6],
          vRes[20];
     mstr _mstr;
     _emptyBuffer(vRes,sizeof(vRes));

     _doCmd(F("AT+CREG?"));
     strcpy_P(vTmpStr, PSTR("+CREG")); 
     if (_getAnswerWait(vRes,sizeof(vRes),vTmpStr)) {

       strcpy_P(vTmpStr, PSTR("0,1")); 
       if (_mstr.indexOf(vRes,vTmpStr)==-1) {
          _setLastError(__LINE__,vRes);
          return false;
       };
     };

   return true;
 }


bool gprs2::canWork() {
   return isPowerUp() && isConnect() && hasNetwork();
};

bool gprs2::canInternet() {
 return gprsNetworkUp() && gprsUp();
}

  bool gprs2::postUrl(char* iUrl, char* iPar, char* oRes) {
    return postUrl(iUrl,iPar,oRes,250);
  }
  bool gprs2::postUrl(char* iUrl, char* iPar, char* oRes,unsigned int iResLength) {
      char _tmpStr[30];
      mstr _mstr;

      #if IS_DEBUG>1
        Serial.println(iUrl);
      #endif

       if (iResLength < 35) {
           strcpy_P(oRes,PSTR("iResLength small"));
           _setLastError(__LINE__,oRes);
          _sendTermCommand();
          _emptyBuffer(oRes,iResLength);
          return false;
       };

	/*****************************************************
         * Чаще всего интернет не поднят,                    *
         * поэтому при принудительном отключении             *
         * проверяем на ошибку чтобы сэкономить время.       *
         *****************************************************/
        _sendTermCommand(false);

        strcpy_P(_tmpStr, (char*)OK_M);
	_doCmd(F("AT+HTTPINIT"));
        if(_getAnswerWait(oRes,iResLength,_tmpStr)) {
           _sendTermCommand();
           _setLastError(__LINE__,oRes);
           _emptyBuffer(oRes,iResLength);
            return false;
        };

        _doCmd(F("AT+HTTPPARA=\"CID\",1"));
        if(_getAnswerWait(oRes,iResLength,_tmpStr)) {
             _setLastError(__LINE__,oRes);
             _sendTermCommand();
             _emptyBuffer(oRes,iResLength);
             return false;
        };

        _doCmd(F("AT+HTTPPARA=\"CONTENT\",\"application/x-www-form-urlencoded\""));
        if(_getAnswerWait(oRes,iResLength,_tmpStr)) {
             _setLastError(__LINE__,oRes);
             _sendTermCommand();
             _emptyBuffer(oRes,iResLength);
             return false;
        };


	_doCmd3(F("AT+HTTPPARA=\"URL\",\""),iUrl,F("\""));
        if(_getAnswerWait(oRes,iResLength,_tmpStr)) {
             _setLastError(__LINE__,oRes);
             _sendTermCommand();
             _emptyBuffer(oRes,iResLength);
             return false;
        };


        _doCmd(F("AT+HTTPPARA=\"REDIR\",0"));
        if(_getAnswerWait(oRes,iResLength,_tmpStr)) {
             _setLastError(__LINE__,oRes);
             _sendTermCommand();
             _emptyBuffer(oRes,iResLength);
             return false;
        };

        /***************************
         *AT+HTTPDATA=%u,30000     *
         ***************************/

       {
         unsigned int vFactSize  = 0;
         vFactSize = strlen(iPar);

         #if IS_DEBUG>1
          Serial.println(iPar);
         #endif

         if (vFactSize>0) {
           sprintf(_tmpStr,"%u",vFactSize);          
           _doCmd3(F("AT+HTTPDATA="),_tmpStr,F(",3000"));

           strcpy_P(_tmpStr, PSTR("DOWNLOAD")); 
           getAnswer3(oRes,iResLength);


           if (_mstr.indexOf(oRes,_tmpStr)==-1) {
             _setLastError(__LINE__,oRes);
             _sendTermCommand();
             _emptyBuffer(oRes,iResLength);
             return false;
           }; 

         _doCmd(iPar);
         strcpy_P(_tmpStr, (char*)OK_M);

         if (_getAnswerWait(oRes,iResLength,_tmpStr)) {
           _setLastError(__LINE__,oRes);
           _sendTermCommand();
           _emptyBuffer(oRes,iResLength);
           return false;                   
         };
       };

      };   //end param block

       _doCmd(F("AT+HTTPACTION=1"));

       strcpy_P(_tmpStr,PSTR("+HTTPACTION")); 

       _getAnswerWait(oRes,iResLength,_tmpStr);

        strcpy_P(_tmpStr, PSTR("+HTTPACTION: 1,200")); 

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

       _doCmd(F("AT+HTTPREAD"));
       strcpy_P(_tmpStr, PSTR("+HTTPREAD"));             
         if (_getAnswerWait(oRes,iResLength,_tmpStr)) {
           _setLastError(__LINE__,oRes);
           _sendTermCommand();
           _emptyBuffer(oRes,iResLength);
           return false;                   
         };
     
       strcpy_P(_tmpStr,(char*)OK_M);             
       if (_mstr.indexOf(oRes,_tmpStr)==-1) {
           _setLastError(__LINE__,oRes);
           _emptyBuffer(oRes,iResLength);
           _sendTermCommand();
           return false;
       };

{
      char vSplitChar[2];
      unsigned int  vFirstSpace = 0, vResLength=0;

      strcpy_P(vSplitChar, (char*)SPACE_M);
      _mstr.trim(oRes,vSplitChar);

     vResLength=strlen(oRes) - 3;
     vResLength=max(0,vResLength);

     oRes[vResLength] = '\0';

      if (strlen(oRes)<=0) {
	   strcpy_P(_tmpStr,PSTR("*NO POST*"));
           _setLastError(__LINE__,_tmpStr);
           _emptyBuffer(oRes,iResLength);
           _sendTermCommand();
           return false;
      };

      strcpy_P(vSplitChar,PSTR(":"));
      vFirstSpace = _mstr.indexOf(oRes,vSplitChar);       

      strcpy_P(vSplitChar, (char*)SPACE_M);
      vFirstSpace = _mstr.indexOf(oRes,vSplitChar,vFirstSpace+2);

      for (int vI = 0; vI < vFirstSpace; vI++) {
          oRes[vI] = ' ';
      };

      _mstr.trim(oRes,vSplitChar);
}
     _sendTermCommand();
   return true;

  }


bool gprs2::saveOnSms() {
   char vRes[10],
        vTmpStr[3];
   mstr _mstr;
   _doCmd(F("AT+CNMI=1,1,0,0,0"));
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
  char vCommand[15],vTmpStr[5];
   sprintf_P(vCommand,PSTR("AT+CMGR=%u,1"),iNum);
  _doCmd(vCommand);
  strcpy_P(vTmpStr,(char*)OK_M);
  _getAnswerWait(oRes,iSmsSize,vTmpStr,true);     
}


void gprs2::_setLastError(unsigned int iErrorNum,char* iErrorText) {
//    _emptyBuffer(_lastError,20);  

    if (strlen(iErrorText)>2) {     
//       strncpy(_lastError,iErrorText,19);
         _lastError=String(iErrorText);
    } else {
//      sprintf_P(_lastError,PSTR("N %d"),iErrorNum);
         _lastError=String(iErrorNum);
    };
//  _lastError[19] = '\0';
  _lastErrorNum  = iErrorNum;

}



void gprs2::getLastError(char* oRes) {
//      strncpy(oRes,_lastError,20);
//      oRes[19] = '\0';
        _lastError.toCharArray(oRes,20);
 }

uint8_t gprs2::getLastErrorNum() {
   return _lastErrorNum;
 }


void gprs2::_emptyBuffer(char* oBuf,size_t iSize) {
 memset(oBuf,'\0',iSize);
// oBuf[0]='\0';
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
   char vRes[20],
        _tmpStr[10];
   mstr _mstr;

  _modem.print(F("AT+CMGD="));
  _modem.flush();
  _modem.println((int)iSms);
  _modem.flush();
  getAnswer3(vRes,sizeof(vRes));

strcpy_P(_tmpStr, (char*)OK_M);
   if (_mstr.indexOf(vRes,_tmpStr)>-1) {
        return true;
   };

  return false;


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
        strcpy_P(vTmpStr, PSTR("- NET"));
        _setLastError(__LINE__,vTmpStr);
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

int gprs2::getSmsCount() {
 char vTmpStr[30],vRes[200];
 int vCount = 0;
  mstr _mstr;


    for (unsigned int vI=1; vI<6; vI++) {
         getSmsText(vI,vRes,sizeof(vRes));  

         _mstr.trim(vRes,' ');

         strcpy_P(vTmpStr, (char*)OK_M);

         if (!_mstr.isEqual(vRes,vTmpStr)) {
             vCount++;
         };

    };

return vCount;
};


bool gprs2::setOnSms(bool (*iSmsEvent)(byte iSms, char* oStr)) {
    _onSmsRead = iSmsEvent;
}


 bool gprs2::readSms(bool deleteAfterRead=true) {
  char vRes[200],
       vDelimiter[2];
  int vFirstLFPos=0;
  mstr _mstr;

    _setSmsTextMode();
    saveOnSms();

    for (unsigned int vI=1; vI<6; vI++) {
         getSmsText(vI,vRes,sizeof(vRes));
             
         if (strlen(vRes) < 20) {
               //******************************************************************
               //Возможно ситуация, когда SMS  содержит текст на кирилице
               //этот текст будет кодироваться существенно больше 200 символов
               // в результате в итоговую строку не попадет завершаюший OK СМС.
               // Это приведет к тому, что СМС будет считаться как отсутствующее,
               // но при этом память СМС будет занята. Поэтому несмотря на то, что
               // СМС не считана память следует очистить.
               //*******************************************************************
	       if (deleteAfterRead) {
	           deleteSms(vI);
	       };
            continue;
         };  

        strcpy_P(vDelimiter, PSTR("\n"));     
        vFirstLFPos = _mstr.indexOf(vRes,vDelimiter,2);

        if (vFirstLFPos == -1) {
            continue;
        };
        vFirstLFPos++;

        _mstr.leftShift(vRes,vFirstLFPos);

        vFirstLFPos = _mstr.indexOf(vRes,vDelimiter);
        for (unsigned int vJ=vFirstLFPos;vJ<strlen(vRes);vJ++) {
            vRes[vJ]='\0';
        };
        

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

void gprs2::_getSmsBody(char* vRes,char* oBody,unsigned int iMaxSmsBodyLength) {

          int  vCurrPos     = 0, 
               vStartPos    = 0, 
               vEndPos      = 0, 
               vStrLen      = strlen(vRes), 
               vLastCharNum = 0,
               copyLength   = 0;

  char vTmpStr[2];
  mstr _mstr;


        /********************************************************
         * Нас интересует перенос строки тела смс от заголовка, *
         * поэтому ищем со второго символа чтобы его пропустить *
         *******************************************************/
         strcpy_P(vTmpStr, PSTR("\n")); 
         vStartPos = _mstr.indexOf(vRes,vTmpStr,2) + 1;           
         vEndPos = min(vStrLen,vStartPos + iMaxSmsBodyLength);  


	      for (unsigned int vJ=vStartPos; vJ<vEndPos; vJ++) {

                 vCurrPos=vJ;

                 if (vRes[vJ]=='\n' || vRes[vJ]=='\r') {  break;  };

               };
         copyLength = vCurrPos - vStartPos;
         vLastCharNum=min(copyLength,iMaxSmsBodyLength-1);
         strncpy(oBody,vRes + vStartPos,copyLength);
         oBody[vLastCharNum] = '\0';     
}

void gprs2::_doCmd(char* iStr) {
  _modem.println(iStr);
  _modem.flush();
};


void gprs2::_doCmd(const __FlashStringHelper *iStr) {
  _modem.println(iStr);
  _modem.flush();
}

void gprs2::_doCmd3(const __FlashStringHelper *iStr1,char* iStr2,const __FlashStringHelper *iStr3) {

  _modem.print(iStr1);
  _modem.flush();
  _modem.print(iStr2);
  _modem.flush();
  _modem.println(iStr3);
  _modem.flush();
}

void gprs2::_doCmd3(const __FlashStringHelper *iStr1,String iStr2,const __FlashStringHelper *iStr3) {
  _modem.print(iStr1);
  _modem.flush();
  _modem.print(iStr2);
  _modem.flush();
  _modem.println(iStr3);
  _modem.flush();
}


void gprs2::softRestart() {
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

 bool gprs2::getCoords(char* oLongitude,char* oLatitdue) {
     char vRes[75],vUnit[20],vTmpStr[20];
     mstr _mstr;

    _emptyBuffer(oLongitude,11);
    _emptyBuffer(oLatitdue,11);

    _doCmd(F("AT+CIPGSMLOC=1,1"));

    strcpy_P(vTmpStr, PSTR("+CIPGSMLOC:"));

    if (_getAnswerWait(vRes,sizeof(vRes),vTmpStr)) {
       _setLastError(__LINE__,vRes);
       return false;
    };

    strcpy_P(vTmpStr, PSTR("CIPGSMLOC: 0"));

    if (_mstr.indexOf(vRes,vTmpStr)==-1) {
       _setLastError(__LINE__,vRes);
       return false;
    };



    strcpy_P(vTmpStr, (char*)COMMA_M);

    if (_mstr.numEntries(vRes,vTmpStr)<=0) {
       _setLastError(__LINE__,vRes);
       return false;
    };

      _mstr.entry(2,vRes,vTmpStr,10,vUnit);

      strncpy(oLongitude,vUnit,10);
      oLongitude[10]='\0';

      _mstr.entry(3,vRes,vTmpStr,10,vUnit);
      strncpy(oLatitdue,vUnit,10);
      oLatitdue[10]='\0';

      return true;
 };


bool gprs2::wakeUp() {   
     char vRes[20],
          vTmpStr[15];
     bool vStatus;


      pinMode(4,OUTPUT);
      digitalWrite(4,LOW);
      delay(1000);


      _doCmd(F("AT+CSCLK=0"));


      strcpy_P(vTmpStr, (char*)OK_M);
      vStatus = _getAnswerWait(vRes,sizeof(vRes),vTmpStr);

      if (vStatus) {
        _setLastError(__LINE__,vRes);
        return false;  
      };

      pinMode(4,INPUT);

 return true;
};

void gprs2::sleep() {   
     char vRes[20],
          vTmpStr[5];
                    
     pinMode(4,INPUT);


     _doCmd(F("AT+CSCLK=1"));


 return false;

};