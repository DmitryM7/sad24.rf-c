#include <Arduino.h>
#include <worker.h>
#include <mstr.h>
#include <EEPROM.h>
#include <MemoryFree.h>
#include <debug.h>
#include <DS3231MDA.h>

#define NEAREST_TIME_BORDER 900000 //Очень большое значение
#define NEAREST_TIME_MULT 86400



worker::worker(unsigned int iStartAddress) {
 _startAddress = iStartAddress;
 pinMode(11, OUTPUT);
 pinMode(12, OUTPUT);
};



/*************************************************************
 * Структура команды следующая:                              *
 *  1. Исполнитель;                                          *
 *  2. Дни недели через зпт., не более 7 штук;               *
 *  3. Время запуска;                                        *
 *  4. Дительность.                                          *
 * Команды могут быть разделены знаком |                     *
 *************************************************************/
bool worker::update(char* iCommand) {
   char *str1,
        _tmpStr1[2],
        _tmpStr2[2],
        vCommand[2];
   unsigned int vAddress = 0;
   bool vNeedReconnect = false;

   mstr _mstr;



   strcpy_P(_tmpStr1, PSTR("|"));
   strcpy_P(_tmpStr2, PSTR(";"));

   while ((str1 = strtok_r(iCommand,_tmpStr1,&iCommand))!=NULL) {

      if (_beforeTaskUpdate) {

         if (!_beforeTaskUpdate(str1)) {
            continue;
         };

      };
      

      if (_mstr.numEntries(str1,_tmpStr2) < 2) {
        continue;
      };

      _mstr.entry(1,str1,_tmpStr2,vCommand);


      if (strcmp_P(vCommand,PSTR("T"))==0) {
         setTime(str1);
         continue;
      };

      if (strcmp_P(vCommand,PSTR("U"))==0) {
         vNeedReconnect = true;
         continue;
      };

      if (strcmp_P(vCommand,PSTR("W"))==0) {
         setTask2(vAddress,str1);
         vAddress++;
 	 continue;
      };

   };

   return vNeedReconnect;
};

void worker::setTask2(unsigned int iAddress,char* iStr) {
   mstr _mstr;
   task _task;
   unsigned int vFactAddress;
   char vStr[20],
        vDelimiter[2];


       strcpy_P(vDelimiter, PSTR(";"));
      
       vFactAddress = _startAddress + iAddress * sizeof(task);   

        if (_mstr.numEntries(iStr,vDelimiter)!=3) {
           return;
        };


        _mstr.entry(2,iStr,vDelimiter,vStr);
        _task.startCode = atol(vStr);


        _mstr.entry(3,iStr,vDelimiter,vStr);
        _task.duration = atol(vStr);


        noInterrupts();
	EEPROM.put(vFactAddress,_task);
	interrupts();

	#if IS_DEBUG>1
         Serial.print(iAddress);
         Serial.print(F(" = "));
         Serial.print(_task.startCode);
         Serial.print(F("->"));
         Serial.print(_task.duration);
         Serial.println(F(" OK"));
        #endif

}


task worker::_getTask(int iAddress) {
  task oTask;
  EEPROM.get(_startAddress + iAddress * sizeof(task),oTask);
  return oTask;
}

void worker::setStartAddress(unsigned int iStartAddress) {
  _startAddress = iStartAddress;
};

void worker::startWater() {
  digitalWrite(11,HIGH);
};
void worker::stopWater() {
  digitalWrite(11,LOW);
};

void worker::startLight() {
  digitalWrite(12,HIGH);
};
void worker::stopLight() {
  digitalWrite(12,LOW);
};

byte worker::_DayOfWeek(int y,byte m,byte d) {
  int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
 
  y -= m < 3;
  return ((y + y/4 - y/100 + y/400 + t[m-1] + d) % 7) + 1; // 01 - 07, 01 = Sunday
};

byte worker::getDayOfWeek() {
  DS3231MDA Clock;
  byte dow,m,d,hh,mm,ss,y;
  

  Clock.getNow(y,m,d,hh,mm,ss);
  dow = _DayOfWeek((int)y,m,d);
 /**************************************
  * Переводим в формат:                *  
  *  1 - понедельник,                  *
  *  7 - воскресенье                   *
  **************************************/
  dow = dow - 1;
  if (dow==0) {
    dow=7;
  };

 return dow;
};

void worker::showDateTime() {

 };

unsigned long worker::getSecMidnight(byte &oDayOfWeek) {
  DS3231MDA Clock;  
    byte m,d,hh,mm,ss,y;
  Clock.getNow(y,m,d,hh,mm,ss);

     oDayOfWeek = _DayOfWeek((int)y,m,d);
  /**************************************
   * Переводим в формат:                *  
   *  1 - понедельник,                  *
   *  7 - воскресенье                   *
   **************************************/
   oDayOfWeek = oDayOfWeek - 1;
   if (oDayOfWeek==0) {
     oDayOfWeek=7;
   };




  return hh * 3600UL + mm * 60UL + (unsigned long) ss;
};

void worker::_setDateTime(byte iYear,byte iMonth,byte iDay,byte iHour,byte iMinutes,byte iSec) {
  DS3231MDA Clock;  
  Clock.setYear(iYear);
  Clock.setMonth(iMonth);
  Clock.setDate(iDay);
  Clock.setClockMode(false);
  Clock.setHour(iHour);
  Clock.setMinute(iMinutes);
  Clock.setSecond(iSec);
}


void worker::setBeforeTaskUpdate(bool (*iEvent)(char* oStr)) {
  _beforeTaskUpdate = iEvent;  
}


byte worker::shouldTaskWork2(byte iAddress,
                             unsigned long iSecMidnight,
                             byte iDayOfWeek
                            ) {
  unsigned long secTaskBeg, secTaskEnd;
  bool isCurrWeekDay,isInTime;
  task _task=_getTask(iAddress);  


  {
    byte vH   = lowByte((_task.startCode & 32505856)  >> 20),
         vM   = lowByte((_task.startCode & 1032192)   >> 14),
         vS   = lowByte((_task.startCode & 16128)     >> 8);

        secTaskBeg    =  vH * 3600UL + vM * 60UL + (unsigned long) vS;
  };

  secTaskEnd    = secTaskBeg + _task.duration;
  secTaskEnd    = min(secTaskEnd, 86400);

  isCurrWeekDay = bitRead(_task.startCode,32 - iDayOfWeek);
  isInTime      = secTaskBeg <= iSecMidnight && iSecMidnight <= secTaskEnd;


  if (isCurrWeekDay && isInTime) {
     return lowByte(_task.startCode);
  };

  return 0;

};



void worker::setTime(char* vCommand) {
   mstr _mstr;
   char vDelimiter[2],vTimeStr[15],vUnit[3];
   byte vYear,vMonth,vDay,vHour,vMinutes,vSec;

   if (strlen(vCommand)!=14) {
      return;
   };


   strcpy_P(vDelimiter, PSTR(";"));


   _mstr.entry(2,vCommand,vDelimiter,vTimeStr);


   _mstr.substr(vTimeStr,0,2,vUnit);
   vYear = byte(atoi(vUnit));


   _mstr.substr(vTimeStr,2,2,vUnit);
   vMonth = byte(atoi(vUnit));

   _mstr.substr(vTimeStr,4,2,vUnit);
   vDay = byte(atoi(vUnit));

   _mstr.substr(vTimeStr,6,2,vUnit);
   vHour = byte(atoi(vUnit));

   _mstr.substr(vTimeStr,8,2,vUnit);
   vMinutes = byte(atoi(vUnit));

   _mstr.substr(vTimeStr,10,2,vUnit);
   vSec = byte(atoi(vUnit));

   #if IS_DEBUG>1
    Serial.print(F("TME:"));
    Serial.print(vYear);
    Serial.print(F("-"));
    Serial.print(vMonth);
    Serial.print(F("-"));
    Serial.print(vDay);
    Serial.print(F(" "));
    Serial.print(vHour);
    Serial.print(F(":"));
    Serial.print(vMinutes);
    Serial.print(F(":"));
    Serial.println(vSec);
    Serial.flush();
  #endif

 _setDateTime(vYear,vMonth,vDay,vHour,vMinutes,vSec);
  
};

void worker::setDateTime(byte iYear,byte iMonth,byte iDay,byte iHour,byte iMinutes,byte iSec) {
     _setDateTime(iYear,iMonth,iDay,iHour,iMinutes,iSec);
};

unsigned long worker::getSleepTime(byte iCurrDayOfWeek,unsigned long iCurrTime) {

  unsigned long vNearestTime = NEAREST_TIME_BORDER; //Умполчательно присваиваем очень далекое время
  byte weight=0;

   for (byte vJ=32-iCurrDayOfWeek;vJ>24;vJ--) { //Смотрим все дни которые лежат правее текущей даты времени
       byte vTestDayOfWeek = 32 - vJ;
   
       for (byte vI=0;vI<maxTaskCount;vI++) {

                task vTask = _getTask(vI);      
                unsigned long vStartTask = _getTaskStart(vTask);             
                bool vIsWorkDay = bitRead(vTask.startCode,vJ);

                if (!vIsWorkDay || (vTestDayOfWeek==iCurrDayOfWeek && vStartTask<iCurrTime) || vTask.duration==0) { //Если день не рабочий, или текущий левее настоящего времени
                     vStartTask = NEAREST_TIME_BORDER;                   
                } else {
	             vStartTask = weight * NEAREST_TIME_MULT + vStartTask - iCurrTime; // Учитываем, что время нужно не от полуночи, а от текущего
                };
             vNearestTime=min(vNearestTime,vStartTask);
       };

	weight++;
   };

   if (vNearestTime!=NEAREST_TIME_BORDER) {  // Если нашли ближайшее на этой неделе, то возвращаем его
      return vNearestTime; 
   };



   for (byte vJ=31;vJ>24+iCurrDayOfWeek;vJ--) { //Смотрим все дни которые лежат левее текущей даты времени   
       byte vTestDayOfWeek = 32 - vJ;

       for (byte vI=0;vI<maxTaskCount;vI++) {
            task vTask = _getTask(vI);      
                unsigned long vStartTask = _getTaskStart(vTask);             
                bool vIsWorkDay = bitRead(vTask.startCode,vJ);

                if (!vIsWorkDay || (vTestDayOfWeek==iCurrDayOfWeek && vStartTask>iCurrTime) || vTask.duration==0) { //Если день не рабочий, или текущий правее настоящего времени
                     vStartTask = NEAREST_TIME_BORDER;          
                } else {
	             vStartTask = weight * NEAREST_TIME_MULT + vStartTask - iCurrTime; // Учитываем, что время нужно не от полуночи, а от текущего
                };
             vNearestTime=min(vNearestTime,vStartTask);
       };
	weight++;
   };

  
  return  vNearestTime; // Это время от полуночи, а нужно от текущего


};

unsigned long worker::_getTaskStart(task iTask) {
  byte vH,vM,vS;

  vH   = lowByte((iTask.startCode & 32505856)  >> 20);
  vM   = lowByte((iTask.startCode & 1032192)   >> 14);
  vS   = lowByte((iTask.startCode & 16128)     >> 8);
  
  return   vH * 3600UL + vM * 60UL + (unsigned long)vS;
};


  long long worker::getTimestamp() {
       unsigned long vSecMidnight;
       byte vDayOfWeek;
       return getTimestamp(vSecMidnight,vDayOfWeek); 
   };

long long worker::getTimestamp(unsigned long &oSecMidnight,byte &oDayOfWeek) {
  DS3231MDA Clock;
  byte m,d,hh,mm,ss,y,vAttempt=0;
  bool vStatus;


   do {
    vStatus=Clock.getNow(y,m,d,hh,mm,ss);   
    vAttempt++;
   } while (!vStatus && vAttempt<3);


   if (!vStatus) { oSecMidnight=0; oDayOfWeek=0; return 0; };
   
   oDayOfWeek = _DayOfWeek((int)y,m,d);
  /**************************************
   * Переводим в формат:                *  
   *  1 - понедельник,                  *
   *  7 - воскресенье                   *
   **************************************/
   oDayOfWeek = oDayOfWeek - 1;
   if (oDayOfWeek==0) {
     oDayOfWeek=7;
   };


  oSecMidnight = hh * 3600UL + mm * 60UL + (unsigned long) ss;

  return (y - 16) * 365 * 86400LL + m * 31 * 86400LL + d * 86400LL + hh * 3600LL + mm * 60LL + ss;

}