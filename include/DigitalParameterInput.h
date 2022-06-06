#ifndef DIGITALPARAMETERINPUT__INCLUDED
#define DIGITALPARAMETERINPUT__INCLUDED

#include "ParameterInput.h"

template<class TargetClass>
class DigitalParameterInput : public BaseParameterInput {
  
  //using Callback = void(callback)(double);

  bool lastValue = false;
  int inputPin = 0;
  //Callback callback;

  public:
    using Callback = void (*)(bool);
    Callback callback;
    DigitalParameterInput(int in_inputPin, Callback in_callback) : BaseParameterInput() {
      inputPin = in_inputPin;
      //callback = in_callback;
      pinMode(inputPin, INPUT);
    }
    DigitalParameterInput(int in_inputPin, BaseParameter &in_target) : BaseParameterInput() {
      inputPin = in_inputPin;
      target = &in_target;
    }
  
    void read() {
      // todo: debouncing
      bool currentValue = digitalRead(inputPin);
      if (currentValue != lastValue) {
        /*if (callback != NULL)
          callback(currentValue);*/
        if (target)
          target->setParamValue(currentValue);
        lastValue = currentValue;
        //return currentValue;
      }
    }
};

#endif