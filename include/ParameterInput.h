
#include <Arduino.h>

#include "Parameter.h"

char NEXT_PARAMETER_NAME = 'W';

template<class TargetClass>
class ParameterInput {
  public:
    int inputPin;
    TargetClass *target_parameter;
    char name;
    bool debug = true;

    ParameterInput() {
      name = NEXT_PARAMETER_NAME++;
    }

    void setDebug() {
      debug = !debug;
    }

    virtual void read() {};
    virtual void loop() {
      read();
    }
};

