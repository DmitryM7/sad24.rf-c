#ifndef sim868_h
#define sim868_h
#include <SoftwareSerial.h>
#include <sim800.h>

struct sim868_coords {
  long int a=0,l=0;
};


class sim868 : public sim800
{


  public:
    sim868(int pin1,int pin2);
    bool getCoords(sim868_coords& coords);
    void btConnect();


};

#endif
