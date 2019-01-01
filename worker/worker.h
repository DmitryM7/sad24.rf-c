#ifndef worker_h
#define worker_h
#include "SoftwareSerial.h"
#include <DS3231.h>

struct task {
  unsigned long startCode = 0;
  unsigned long duration  = 0;
};

struct workerInfo {
  volatile bool          isWork    = false;
  volatile bool          isEdge    = false;
  volatile unsigned long startTime = 0;
  volatile unsigned long duration  = 0;
};

class worker
{
  public:
   byte maxTaskCount = 5;
   worker(unsigned int iStartAddress);
   byte update(char* iCommand);
   void setTask2(unsigned int iAddress,char* iStr);
   task getTask(int iAddress);
   void setStartAddress(unsigned int iStartAddress);
   void startWater();
   void stopWater();
   void startLight();
   void stopLight();
   byte getDayOfWeek();
   void showDateTime();
   unsigned long getSecMidnight(byte &oDayOfWeek);
   long long getTimestamp(unsigned long &oSecMidnight,byte &oDayOfWeek);
   void setBeforeTaskUpdate(bool (*_mEvent)(char* oStr));
   byte shouldTaskWork2(byte iAddress,unsigned long iSecMidnight,byte iDayOfWeek);
   void setTime(char* vCommand);
   void setDateTime(byte iYear,byte iMonth,byte iDay,byte iHour,byte iMinutes,byte iSec);
   unsigned long getSleepTime();

  private:
   task _getTask(int iAddress);   
   unsigned long _getTaskStart(task iTask);
   void _setDateTime(byte iYear,byte iMonth,byte iDay,byte iHour,byte iMinutes,byte iSec);
   byte _DayOfWeek(int y,byte m,byte d);
   unsigned long _getMinTaskTime(byte iCurrDayOfWeek,unsigned long iCurrTime);       
   unsigned int _startAddress = 0;
   bool (*_beforeTaskUpdate)(char* oStr);

};
#endif