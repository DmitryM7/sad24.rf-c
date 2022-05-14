#ifndef stdSensorInfoLoader_h
#define stdSensorInfoLoader_h

/************************************
 * Задаем параметры подключения УЗД *
 *************************************/
#define ECHO_PIN 6
#define TRIG_PIN 10
#define __DISTANCE_COUNT__ 14

class stdSensorInfoLoader
{

   public :
           int  getT1();
           int  getT2();
           int  getH1();
           long getP1();
           unsigned int  getDistance();
           int  getMiddleDistance();
           void loadSensorInfo();
           void setLastMeasure(long long t);
           long long getLastMeasure();

   private :
      int t1=19900, t2=1990, h1, distance;
      long p1;
      long long lastMeasure=0;

      unsigned int loadDistance();
      int calcMiddleDistance();
};

#endif