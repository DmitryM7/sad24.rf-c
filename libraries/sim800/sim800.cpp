#include <Arduino.h>
#include <sim800.h>
#include <mstr.h>
#include <debug.h>



sim800::sim800(int pin1,int pin2): _modem(pin1,pin2) {
   // Module revision Revision:1137B06SIM900M64_ST_ENHANCE
   _modem.begin(19200);
};

 bool sim800::isPowerUp() {
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

 bool sim800::hasNetwork() {
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

bool sim800::isConnect() {
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

 bool sim800::getAnswer3(char* oRes,size_t iSize) {
  return _getAnswer3(oRes,iSize,false);
};


bool sim800::_getAnswer3(
                        char* oRes,
                        size_t iSize,
                        bool saveCRLF,
                        unsigned long iTimeOut
                       ) {
  unsigned long vCurrTime = millis(),
                vLastReadTime=0;
  size_t vI = 0;
  bool wasRead=false;
  const char constNewLineDelimeter='\0',constSpace=' ';


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
        case constNewLineDelimeter:
        case '\r':
        break;
        case '\n':
        case constSpace:
           if (!saveCRLF) {
               /***********************************************
                * Убираем начальные и подряд идующие пробелы. *
                ***********************************************/
              if (vI > 0 && oRes[vI-1]!=constSpace) {
                 oRes[vI] = constSpace;
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
      if (oRes[vI-1]==constSpace) { oRes[vI] = constNewLineDelimeter; oRes[vI-1] = constNewLineDelimeter; } else { oRes[vI] = constNewLineDelimeter; };
   };

  #if IS_DEBUG>1
    Serial.print(millis()); Serial.print(F("  :")); Serial.println(oRes); Serial.flush();
  #endif
  return wasRead;
}

bool sim800::_getAnswerWait(char* oRes,
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

bool sim800::_getAnswerWaitLong(char* oRes,
	                       size_t iSize,
	                       char* iNeedStr,
                               unsigned long iTimeOut) {
     mstr _mstr;
    _getAnswer3(oRes,iSize,false,iTimeOut);
     return _mstr.indexOf(oRes,iNeedStr)==-1;
}

void sim800::_setLastError(unsigned int iErrorNum,char* iErrorText) {
  _lastError=String(iErrorNum);
  _lastErrorNum  = iErrorNum;
}



void sim800::getLastError(char* oRes) {
        _lastError.toCharArray(oRes,20);
 }

uint8_t sim800::getLastErrorNum() {
   return _lastErrorNum;
 }


void sim800::_emptyBuffer(char* oBuf,size_t iSize) {
 memset(oBuf,'\0',iSize);
}

void sim800::_doCmd(char* iStr) {
  _modem.println(iStr);
  _modem.flush();
};


void sim800::_doCmd(const __FlashStringHelper *iStr) {
  #if IS_DEBUG>1
   Serial.println(iStr);
  #endif
  _modem.println(iStr);
  _modem.flush();
}


bool sim800::softRestart() {
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

void sim800::hardRestart() {
 pinMode(6,OUTPUT);
 digitalWrite(6,LOW);
 delay(1000);
 pinMode(6,INPUT);
}

bool sim800::wakeUp() {
      pinMode(4,OUTPUT);
      digitalWrite(4,HIGH);
      return true;
};

void sim800::sleep() {
      pinMode(4,OUTPUT);
      digitalWrite(4,LOW);
};

bool sim800::hasGprsNetwork() {
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

 bool sim800::gprsNetworkUp(bool iForce) {
  char vRes[10],vTmpStr[3];
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

bool sim800::gprsNetworkDown() {
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


bool sim800::hasGprs() {
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

bool sim800::gprsUp(bool iForce) {

     char vRes[10],vTmpStr[3];
     mstr _mstr;

     strcpy_P(vTmpStr, (char*)OK_M);


    _modem.print(F("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\";+SAPBR=3,1,\"APN\",\""));
    _modem.print(_apn);
    _modem.print(F("\";+SAPBR=3,1,\"USER\",\""));
    _modem.print(_login);
    _modem.print(F("\";+SAPBR=3,1,\"PWD\",\""));
    _modem.print(_pass);
    _modem.println(F("\";+SAPBR=1,1;"));
    _modem.flush();


    if (_getAnswerWaitLong(vRes,sizeof(vRes),vTmpStr,5000)) {
     _setLastError(__LINE__,vRes);
      return false;
    };


    return true;
}

bool sim800::gprsDown() {
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

bool sim800::canWork() {
   return isPowerUp() && hasNetwork() && isConnect();
};

bool sim800::doInternet() {

  char vRes[4],vTmpStr[4];
  unsigned int vResLength=sizeof(vRes);

	if (!gprsNetworkUp()) {
	   return false;
	 };

	 if(!hasGprs()) {
            gprsUp();

           if (!hasGprs()) {
             return false;
           };

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

    return true;

}

void sim800::setInternetSettings(char* iApn,char* iLogin,char* iPass) {
     _apn   = String(iApn);
     _login = String(iLogin);
     _pass  = String(iPass);
}

  bool sim800::postUrl(char* iUrl, char* iPar, char* oRes) {
    return postUrl(iUrl,iPar,oRes,250);
  }

  bool sim800::postUrl(char* iUrl, char* iPar, char* oRes,unsigned int iResLength) {
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

        _modem.print(F("AT+HTTPPARA=\"CID\",1;+HTTPPARA=\"CONTENT\",\"application/x-www-form-urlencoded\";+HTTPPARA=\"REDIR\",0;"));
        _modem.print(F("+HTTPPARA=\"URL\",\""));
        _modem.print(iUrl);
        _modem.println(F("\";"));
        _modem.flush();

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


           _modem.print(F("AT+HTTPDATA="));
           _modem.print(vFactSize);
           _modem.println(F(",12000"));

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

       _getAnswerWait(oRes,iResLength,_tmpStr,false,30000); //Он выдает ОК, потом может долго думать и выдать уже искомвый ответ. Поставил 30 с как обычно в Инете


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

void sim800::_sendTermCommand(bool iWaitOK) {
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


bool sim800::_setOnSmsMode() {
   char vRes[10],
        vTmpStr[3];
   mstr _mstr;
   _doCmd(F("AT+CNMI=2,0,0,0,0;+CMGF=1;"));
   getAnswer3(vRes,sizeof(vRes));
   strcpy_P(vTmpStr, (char*)OK_M);
   return _mstr.indexOf(vRes,vTmpStr) > -1;
}

bool sim800::_clearSmsBody(char* iRes,unsigned int iSize) {
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

bool sim800::setOnSms(bool (*iSmsEvent)(byte iSms, char* oStr)) {
    _onSmsRead = iSmsEvent;
}

bool sim800::setBeforePostParams(bool (*ifunction)(char* iStr)) {
    _onBeforePostParams = ifunction;
}

bool sim800::setAfterPostParams(bool (*ifunction)()) {
    _onAfterPostParams = ifunction;
}

void sim800::getSmsText(unsigned int iNum,char* oRes,unsigned int iSmsSize) {
   char vCommand[13],vTmpStr[3];
   sprintf_P(vCommand,PSTR("AT+CMGR=%u,1"),iNum);
   _doCmd(vCommand);
   strcpy_P(vTmpStr,(char*)OK_M);
  _getAnswerWait(oRes,iSmsSize,vTmpStr,true);
}

 bool sim800::readSms(bool deleteAfterRead) {
    char vRes[200];

   _setOnSmsMode();
   _setSmsTextMode();

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

 bool sim800::deleteSms(byte iSms) {
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

 bool sim800::deleteAllSms() {
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

bool sim800::_setSmsTextMode() {
  char vTmpStr[3],
       vRes[10];

  _doCmd(F("AT+CMGF=1"));
  strcpy_P(vTmpStr, (char*)OK_M);
  return  _getAnswerWait(vRes,sizeof(vRes),vTmpStr)>-1;
}














sim800::~sim800() {}