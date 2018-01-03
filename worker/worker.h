#ifndef worker_h
#define worker_h
#include "SoftwareSerial.h"
#include <DS3231.h>

struct task {
  char executor[5];
  char dayOfWeek[30];

  unsigned int hours;
  unsigned int minutes;
  unsigned int seconds;

  unsigned long duration = 0;
};

struct workerInfo {
 bool isWork = false;
 unsigned long startTime = 0;
 unsigned long duration  = 0;
};

class worker
{
  public:
   int maxTaskCount = 5;
   worker(unsigned int iStartAddress);
   byte update(char* iCommand);
   void setTask(unsigned int iAddress,char* iStr);
   task getTask(int iAddress);
   void setStartAddress(unsigned int iStartAddress);
   void startWater();
   void stopWater();
   void startLight();
   void stopLight();
   byte getDayOfWeek();
   void showDateTime();
   long getSecMidnight();
   void setBeforeTaskUpdate(bool (*_mEvent)(char* oStr));
   bool shouldTaskWork(unsigned int iAddress,unsigned long iSecMidnight,bool& oShoulWaterWork,bool& oShouldLightWork);
   void setTime(char* vCommand);
   void setDateTime(byte iYear,byte iMonth,byte iDay,byte iHour,byte iMinutes,byte iSec);
    

  private:
   task _getTask(int iAddress);   
   void _setDateTime(byte iYear,byte iMonth,byte iDay,byte iHour,byte iMinutes,byte iSec);
   byte _DayOfWeek(int y,byte m,byte d);
   
   unsigned int _startAddress = 0;

   bool (*_beforeTaskUpdate)(char* oStr);


};
#endif