#ifndef PARAMETER_INPUT__INCLUDED
#define PARAMETER_INPUT__INCLUDED

#include <Arduino.h>

#include "Parameter.h"

class BaseParameterInput {
  public:
    BaseParameter *target = nullptr;

    virtual void loop();
};

template<class TargetClass>
class ParameterInput : public BaseParameterInput {
  public:
    int inputPin = 0;
    TargetClass *target_parameter = nullptr;
    char name = "#";
    bool debug = false;

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

#endif