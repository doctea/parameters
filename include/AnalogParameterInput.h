#ifndef ANALOGPARAMETERINPUT__INCLUDED
#define ANALOGPARAMETERINPUT__INCLUDED

#include "ParameterInput.h"

template<class TargetClass, class DataType>
class AnalogParameterInput : public ParameterInput<TargetClass> {
  int inputPin;

  DataType lastValue = 0;
  byte Parameter_number = 0xff;

  DataType min_input_value = 0.0d;
  DataType max_input_value = 1023.0d;

  public:
    using Callback = void (*)(float);
    Callback callback;

    bool inverted = false;
    DataType sensitivity = 0.01;
      
    /*AnalogParameterInput(int in_inputPin, Callback in_callback, int in_sensitivity = 3) : ParameterInput() {
      inputPin = in_inputPin;
      callback = in_callback;
      sensitivity = in_sensitivity;
      pinMode(inputPin, INPUT);
    }*/
    AnalogParameterInput() {};
    AnalogParameterInput(int in_inputPin, TargetClass &in_target, DataType in_sensitivity = 0.01) { //}: ParameterInput() {
      this->inputPin = in_inputPin;
      this->target_parameter = &in_target;
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
      Serial.println("SET INVERTED on an AnalogParameterInput!");
    }

    virtual void loop () override {
      read();
    }

    /*virtual DataType get_normal_value(int value) {
      if (inverted)
        return 1.0f - ((float)value / 1023.0f);
      else 
        return (float)value / 1023.0f;
    }*/
    virtual DataType get_normal_value(DataType value) {
      if (inverted) {
        /*Serial.print("in AnalogParameterInput#get_normal_value(): Inverting ");
        Serial.print(value);
        Serial.print(" to ");
        Serial.println(1.0f - ((float)value / this->max_input_value));*/
        return 1.0f - ((float)value / this->max_input_value);
      } else 
        return (float)value / max_input_value;
    }

    virtual bool is_significant_change(DataType currentValue, DataType lastValue) {
      return abs(currentValue - this->lastValue) >= this->sensitivity;
    }

    virtual void read() override {
      Serial.printf("read() in AnalogParameterInput.."); 
      DataType currentValue = analogRead(inputPin);
      if (is_significant_change(currentValue, lastValue)) {
      //if (abs(currentValue - this->lastValue) > this->sensitivity) {
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
        if (this->target_parameter!=nullptr) {
          if (this->debug) {
            Serial.print(this->name);
            Serial.print(F(": calling target setParamValueA("));
            Serial.print(normal);
            Serial.print(F(")"));
            if (inverted) Serial.print(F(" - inverted"));
            Serial.println();
          }
          this->target_parameter->setParamValue(normal);
        }
        //if (Parameter_number!=0xff)
          //Parameters[Parameter_number]->setParamValueA(get_normal_value(currentValue));
        //return currentValue;
      }
    }
};

#endif