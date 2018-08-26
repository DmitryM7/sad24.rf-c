﻿#include <Arduino.h>
#include <worker.h>
#include <mstr.h>
#include <EEPROM.h>
#include <debug.h>



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
byte worker::update(char* iCommand) {
   char *str1;
   unsigned int vAddress = 0;
   char _tmpStr1[2],_tmpStr2[2],vCommand[2];
   mstr _mstr;
   bool vNeedReconnect = 0;



   strcpy_P(_tmpStr1, PSTR("|"));
   while ((str1 = strtok_r(iCommand,_tmpStr1,&iCommand))!=NULL) {

      if (_beforeTaskUpdate) {
         if (!_beforeTaskUpdate(str1)) {
            continue;
         };
      };
      
      strcpy_P(_tmpStr2, PSTR(";"));

      if (_mstr.numEntries(str1,_tmpStr2) < 2) {
        continue;
      };

      _mstr.entry(1,str1,_tmpStr2,vCommand);


      if (strcmp_P(vCommand,PSTR("T"))==0) {
         setTime(str1);
         continue;
      };

      if (strcmp_P(vCommand,PSTR("U"))==0) {
         vNeedReconnect = 1;
         continue;
      };

       setTask2(vAddress,str1);
       vAddress++;
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
        #if IS_DEBUG>1
          Serial.println(iStr);
        #endif


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
        Serial.print(F(" -> "));
        Serial.print(_task.duration);
        Serial.println(F(" write OK"));
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
  DS3231 Clock;
  bool century = false;
  byte dow;
  dow = _DayOfWeek((int)Clock.getYear(),Clock.getMonth(century),Clock.getDate());
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
  DS3231 Clock;
  bool H12 = false;
  bool PM;
  bool century = false;
  int day, month, year, hours, minutes;  

  day   = (int)Clock.getDate();
  month = (int)Clock.getMonth(century);
  year  = (int)Clock.getYear();
  hours = (int)Clock.getHour(H12, PM);
  minutes = (int)Clock.getMinute();

  Serial.print(day);
  Serial.print(F("-"));
  Serial.print(month);
  Serial.print(F("-"));
  Serial.print(year);
  Serial.print(F(" "));
  Serial.print(hours);
  Serial.print(F(":"));
  Serial.print(minutes); 
  Serial.print(F(" ")); 
  Serial.print(getDayOfWeek());
  Serial.flush();
 };

 unsigned long worker::getSecMidnight() {
  bool H12 = false, 
       PM;
  unsigned long ret1;
  byte h,m,s;

  DS3231 Clock;

  h = Clock.getHour(H12, PM);
  m = Clock.getMinute();
  s = Clock.getSecond();

  ret1 = h * 3600UL + m * 60UL + (unsigned int)s;

  return ret1;
};

void worker::getSecMidnight2(unsigned long *oT) {
  *oT=getSecMidnight();
}

void worker::_setDateTime(byte iYear,byte iMonth,byte iDay,byte iHour,byte iMinutes,byte iSec) {
  DS3231 Clock;  
  Clock.setYear(iYear);
  Clock.setMonth(iMonth);
  Clock.setDate(iDay);
  Clock.setHour(iHour);
  Clock.setMinute(iMinutes);
  Clock.setSecond(iSec);
}


void worker::setBeforeTaskUpdate(bool (*iEvent)(char* oStr)) {
  _beforeTaskUpdate = iEvent;  
}


byte worker::shouldTaskWork2(byte iAddress,
                             unsigned long iSecMidnight
                            ) {
  unsigned long secTaskBeg, secTaskEnd,vH1,vM1,vS1;
  byte  vH, vM, vS, currDayOfWeek = getDayOfWeek();
  bool isCurrWeekDay,isInTime;
  task _task;  

  EEPROM.get(_startAddress + iAddress * sizeof(task),_task);


  vH   = lowByte((_task.startCode & 32505856)  >> 20);
  vM   = lowByte((_task.startCode & 1032192)   >> 14);
  vS   = lowByte((_task.startCode & 16128)     >> 8);

  vH1  = (unsigned long)vH;
  vM1  = (unsigned long)vM;
  vS1  = (unsigned long)vS;


  secTaskBeg    =  vH1 * 3600 + vM1 * 60 + vS;

  secTaskEnd    = secTaskBeg + _task.duration;
  secTaskEnd    = min(secTaskEnd, 86400);

  isCurrWeekDay = bitRead(_task.startCode,32 - currDayOfWeek);
  isInTime      = secTaskBeg <= iSecMidnight && iSecMidnight <= secTaskEnd;


  if (isCurrWeekDay && isInTime) {
     return lowByte(_task.startCode);
  };

    return 0;

};



void worker::setTime(char* vCommand) {
   mstr _mstr;
   char vDelimiter[2],vTimeStr[15],vUnit[2];
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
    Serial.print(F("Set Time:"));
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

unsigned long worker::getNearestTaskTime() {
   byte vCurrDay = getDayOfWeek(),
        vNum=0;
   unsigned long secTaskBeg,
        secNow=getSecMidnight(),
        vSch[7],
        vTmpSec;
   task _task;  

   
   for (byte vI=0;vI<maxTaskCount;vI++) {

        EEPROM.get(_startAddress + vI * sizeof(task),_task);

           if (_isCurrWeekDay(_task)) {  

              secTaskBeg = _getTaskStart(_task);

              if (secTaskBeg<=secNow) {
                 continue;
              };
              vSch[vNum++] = secTaskBeg;
           };
   };
   vSch[vNum++] = 85000;

   vTmpSec = vSch[0];

   for (byte vI=0;vI<vNum;vI++) {
      if (vSch[vI]<vTmpSec) { vTmpSec = vSch[vI]; };
   };

  return vTmpSec;

};

unsigned long worker::_getTaskStart(task iTask) {
  byte vH,vM,vS;
  unsigned long secTaskBeg;

  vH   = lowByte((iTask.startCode & 32505856)  >> 20);
  vM   = lowByte((iTask.startCode & 1032192)   >> 14);
  vS   = lowByte((iTask.startCode & 16128)     >> 8);

  secTaskBeg    =  (unsigned long)vH * 3600 + (unsigned long)vM * 60 + (unsigned long)vS;

  return secTaskBeg;
};

bool worker::_isCurrWeekDay(task iTask) {
    bool isCurrWeekDay;

    isCurrWeekDay = bitRead(iTask.startCode,32 - getDayOfWeek());
    return isCurrWeekDay;
};
