#ifndef DIGITALPARAMETERINPUT__INCLUDED
#define DIGITALPARAMETERINPUT__INCLUDED

#include "parameter_inputs/ParameterInput.h"

//template<class TargetClass>
class DigitalParameterInput : public BaseParameterInput {
  //using Callback = void(callback)(float);

  bool lastValue = false;
  int inputPin = 0;
  //Callback callback;

  public:
    //using CallbackWithParam = void (*)(bool);
    void (*callback_with_param)(bool) = nullptr;

    //using CallbackNoParam = void (*)();
    void (*callback_on)()  = nullptr;
    void (*callback_off)() = nullptr;

    //BaseParameter *target_parameter = nullptr;

    DigitalParameterInput(int inputPin, const char *group_name = "General") : BaseParameterInput() { 
      this->group_name = group_name;
      this->inputPin = inputPin;
      pinMode(inputPin, INPUT);
    }

    DigitalParameterInput(int inputPin, void (*callback_with_param)(bool)) : DigitalParameterInput(inputPin) {
      this->callback_with_param = callback_with_param;
    }
    DigitalParameterInput(int inputPin, void (*callback_on)(), void (callback_off)()) : DigitalParameterInput(inputPin) {
      this->callback_on = callback_on;
      this->callback_off = callback_off;
    }
    /*DigitalParameterInput(int inputPin, BaseParameter *target) : DigitalParameterInput(inputPin) {
      target_parameter = target;
    }*/

    virtual void loop() {
      this->read();
    }
  
    virtual void read() {
      // todo: debouncing
      bool currentValue = digitalRead(inputPin);
      if (currentValue != lastValue) {
        if (callback_with_param != nullptr)
          (*this->callback_with_param)(currentValue);
        if (currentValue && callback_on != nullptr) 
          (*this->callback_on)();
        if (!currentValue && callback_off != nullptr) 
          (*this->callback_off)();
        /*if (target_parameter!=nullptr)
          this->target_parameter->setParamValue(currentValue);*/
        lastValue = currentValue;
        //return currentValue;
      }
    }
};

#endif