#include "ParameterInput.h"

template<class TargetClass>
class DigitalParameterInput : public ParameterInput  {
  
  bool lastValue = false;

  public:
    using Callback = void (*)(bool);
    Callback callback;
    DigitalParameterInput(int in_inputPin, Callback in_callback) : ParameterInput() {
      inputPin = in_inputPin;
      callback = in_callback;
      pinMode(inputPin, INPUT);
    }
    DigitalParameterInput(int in_inputPin, Parameter &in_target) : ParameterInput() {
      inputPin = in_inputPin;
      target = &in_target;
    }
  
    void read() {
      // todo: debouncing
      bool currentValue = digitalRead(inputPin);
      if (currentValue != lastValue) {
        if (callback != NULL)
          callback(currentValue);
        if (target)
          target->setParamValueA(currentValue);
        lastValue = currentValue;
        //return currentValue;
      }
    }
};