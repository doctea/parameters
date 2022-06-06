#include "ParameterInput.h"

template<class TargetClass, class DataType>
class AnalogParameterInput : public ParameterInput<TargetClass> {
  int inputPin;

  DataType lastValue = 0;
  byte Parameter_number = 0xff;

  public:
    using Callback = void (*)(float);
    Callback callback;

    bool inverted = false;
    int sensitivity = 4;
      
    /*AnalogParameterInput(int in_inputPin, Callback in_callback, int in_sensitivity = 3) : ParameterInput() {
      inputPin = in_inputPin;
      callback = in_callback;
      sensitivity = in_sensitivity;
      pinMode(inputPin, INPUT);
    }*/
    AnalogParameterInput() {};
    AnalogParameterInput(int in_inputPin, TargetClass &in_target, int in_sensitivity = 3) { //}: ParameterInput() {
      this->inputPin = in_inputPin;
      this->target = &in_target;
      this->sensitivity = in_sensitivity;
      pinMode(inputPin, INPUT);
    }
    /*AnalogParameterInput(int in_inputPin, byte in_Parameter_number, int in_sensitivity = 3) : ParameterInput() {
      inputPin = in_inputPin;
      Parameter_number = in_Parameter_number;
      sensitivity = in_sensitivity;
      pinMode(inputPin, INPUT);
    }*/

    virtual void setInverted(bool invert = true) {
      inverted = invert;
    }

    virtual void loop () override {
      read();
    }

    virtual float get_normal_value(int value) {
      if (inverted)
        return 1.0f - ((float)value / 1023.0f);
      else 
        return (float)value / 1023.0f;
    }

    virtual void read() override {
      DataType currentValue = analogRead(inputPin);
      if (abs(currentValue - this->lastValue) > this->sensitivity) {
        lastValue = currentValue;
        float normal = get_normal_value(currentValue);
        if (callback != NULL) {
          if (this->debug) {
            Serial.print(this->name);
            Serial.print(F(": calling callback("));
            Serial.print(normal);
            Serial.println(F(")"));
          }      
          callback(normal);
        }
        if (this->target) {
          if (this->debug) {
            Serial.print(this->name);
            Serial.print(F(": calling target setParamValueA("));
            Serial.print(normal);
            Serial.print(F(")"));
            if (inverted) Serial.print(F(" - inverted"));
            Serial.println();
          }
          this->target->setParamValue(normal);
        }
        //if (Parameter_number!=0xff)
          //Parameters[Parameter_number]->setParamValueA(get_normal_value(currentValue));
        //return currentValue;
      }
    }
};
