#include <Arduino.h>
#include <worker.h>
#include <mstr.h>
#include <EEPROM.h>




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
         if (_beforeTaskUpdate(str1)==false) {
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
      };

       Serial.println(str1);
       setTask(vAddress,str1);
       vAddress++;
   };
   return vNeedReconnect;
};

void worker::setTask(unsigned int iAddress,char* iStr) {
   mstr _mstr;
   task _task;
   unsigned int vFactAddress;
   char vStr[20],
        vStr1[5],
        vDelimiter=';',
        vDelimiter2=':';

      
       vFactAddress = _startAddress + iAddress * sizeof(task);   

        if (_mstr.numEntries(iStr,vDelimiter)!=4) {
           return;
        };


        _mstr.entry(1,iStr,vDelimiter,vStr);
        strcpy(_task.executor,vStr);


        _mstr.entry(2,iStr,vDelimiter,vStr);
        strcpy(_task.dayOfWeek,vStr);

        _mstr.entry(3,iStr,vDelimiter,vStr);

        if (_mstr.numEntries(vStr,vDelimiter2)!=3) {
         return;
        };

        _mstr.entry(1,vStr,vDelimiter2,vStr1);
	_task.hours     = atoi(vStr1);

        _mstr.entry(2,vStr,vDelimiter2,vStr1);
        _task.minutes   = atoi(vStr1);

        _mstr.entry(3,vStr,vDelimiter2,vStr1);
        _task.seconds   = atoi(vStr1);

        _mstr.entry(4,iStr,vDelimiter,vStr);
        _task.duration = strtol(vStr,NULL,10);

	EEPROM.put(vFactAddress,_task);
        Serial.print(iAddress);
        Serial.print(F(" = "));
        Serial.println(F("write OK"));

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
  DS3231 Clock;

  return long((long)(Clock.getHour(H12, PM)) * 3600 + int(Clock.getMinute()) * 60 + int(Clock.getSecond()));
};

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

bool worker::shouldTaskWork(unsigned int iAddress,bool& oShouldWaterWork,bool& oShouldLightWork) {
 mstr _mstr;
 unsigned long int secMidnight, secTaskBeg, secTaskEnd;
 char dayOfWeek[2];
 task _task;  


  EEPROM.get(_startAddress + iAddress * sizeof(task),_task);

  secMidnight = getSecMidnight(); 
  sprintf_P(dayOfWeek, PSTR("%d"), getDayOfWeek());
  secTaskBeg = (unsigned long)(_task.hours * 3600 + _task.minutes * 60 + _task.seconds);
  secTaskEnd = min((unsigned long)(secTaskBeg + _task.duration), 86400);
/*  Serial.print(iAddress); Serial.print(F(" = ")); Serial.print(secTaskBeg); Serial.print(F(" & ")); Serial.println(secTaskEnd);
  Serial.flush();*/


 if (_mstr.indexOf(_task.dayOfWeek, dayOfWeek) != -1 && secTaskBeg <= secMidnight && secMidnight <= secTaskEnd) {
    oShouldLightWork = strcmp_P(_task.executor, PSTR("L")) == 0;
    oShouldWaterWork = strcmp_P(_task.executor, PSTR("W")) == 0;
    return true;
 };

    oShouldLightWork = false;
    oShouldWaterWork = false;
    return false;

};

void worker::setTime(char* vCommand) {
   mstr _mstr;
   char vDelimiter[2],vTimeStr[14],vUnit[2];
   byte vYear,vMonth,vDay,vHour,vMinutes,vSec;

   if (strlen(vCommand)!=14) {
      return;
   };

   strcpy_P(vDelimiter, PSTR(";"));
   _mstr.entry(2,vCommand,vDelimiter,vTimeStr);

   _mstr.substr(vTimeStr,0,2,vUnit);
   vYear = (byte) atoi(vUnit);

   _mstr.substr(vTimeStr,2,2,vUnit);
   vMonth = (byte) atoi(vUnit);

   _mstr.substr(vTimeStr,4,2,vUnit);
   vDay = (byte) atoi(vUnit);

   _mstr.substr(vTimeStr,6,2,vUnit);
   vHour = (byte) atoi(vUnit);

   _mstr.substr(vTimeStr,8,2,vUnit);
   vMinutes = (byte) atoi(vUnit);

   _mstr.substr(vTimeStr,10,2,vUnit);
   vSec = (byte) atoi(vUnit);

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

 _setDateTime(vYear,vMonth,vDay,vHour,vMinutes,vSec);
  
};

   void worker::setDateTime(byte iYear,byte iMonth,byte iDay,byte iHour,byte iMinutes,byte iSec) {
     _setDateTime(iYear,iMonth,iDay,iHour,iMinutes,iSec);
   };