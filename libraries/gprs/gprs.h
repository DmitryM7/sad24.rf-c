#ifndef gprs_h
#define gprs_h
#include <SoftwareSerial.h>
#include <sim800.h>

struct gprs_coords {
  long int a=0,l=0;
};


class gprs : public sim800
{


  public:
    gprs(int pin1,int pin2);
    bool getCoords(gprs_coords& coords);


};

#endif
