#ifndef worker_h
#define worker_h
#include "SoftwareSerial.h"
#include <DS3231.h>

struct task {
  unsigned long startCode = 0;
  unsigned long duration  = 0;
};

typedef struct {
  volatile bool          isWork    = false;
  volatile unsigned long startTime = 0;
  volatile unsigned long duration  = 0;
} workerInfo;

class worker
{
  public:
   int maxTaskCount = 5;
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
   long getSecMidnight();
   void setBeforeTaskUpdate(bool (*_mEvent)(char* oStr));
   byte shouldTaskWork2(unsigned int iAddress,unsigned long iSecMidnight,byte iCurrDayOfWeek);
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