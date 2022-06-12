#ifndef PARAMETER_INPUT__INCLUDED
#define PARAMETER_INPUT__INCLUDED

#include <Arduino.h>

#include "Parameter.h"

class BaseParameterInput {
  public:
    DataParameter *target_parameter = nullptr;

    virtual void setTarget(DataParameter *target) {
      if (this->target_parameter!=nullptr) {
        // already assigned to a target; notify the target that its been unbound, in case we need to set parameter back to zero
        this->target_parameter->on_unbound(this);
      }
      this->target_parameter = target;
    }

    virtual const char* getFormattedValue() {
      if (target_parameter!=nullptr)
        return target_parameter->getFormattedValue();
      else 
        return "[none]";
    }

    virtual const char* getInputInfo() {
      return "BaseParameterInput";
    }

    virtual void loop();
};

template<class TargetClass>
class ParameterInput : public BaseParameterInput {
  public:
    int inputPin = 0;
    //TargetClass *target_parameter = nullptr;
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