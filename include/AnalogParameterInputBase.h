#ifndef ANALOGPARAMETERINPUTBASE__INCLUDED
#define ANALOGPARAMETERINPUTBASE__INCLUDED

#include "ParameterInput.h"

template<class TargetClass, class DataType>
class AnalogParameterInputBase : public ParameterInput<TargetClass> {

  public:
    DataType lastValue = 0;
    DataType currentValue = 0;

    DataType min_input_value = 0.0d;
    DataType max_input_value = 1023.0d;

    using Callback = void (*)(float);
    Callback callback = nullptr;

    DataType sensitivity = 0.005;
      
    AnalogParameterInput() {};
    AnalogParameterInput(char name, TargetClass &in_target, DataType in_sensitivity = 0.005) { //}: ParameterInput() {
      this->name = name;
      this->target_parameter = &in_target;
      this->sensitivity = in_sensitivity;
    }

    virtual void setInverted(bool invert = true) {
      this->inverted = invert;
      Serial.printf("%s: SET INVERTED on an AnalogParameterInput!", this->name);
    }

    virtual void loop () override {
      read();
    }

    virtual DataType get_normal_value(DataType value) {
      if (this->inverted) {
        value = 1.0f - ((float)value / this->max_input_value);
      } else 
        value = (float)value / max_input_value;

      if (this->map_unipolar)
        value = -1.0 + (value*2.0);

      return value;
    }

    virtual bool is_significant_change(DataType currentValue, DataType lastValue) {
      return abs(currentValue - this->lastValue) >= this->sensitivity;
    }

    virtual const char *getInputInfo() {
      static char input_info[20] = "                ";

      sprintf(input_info, "Anlg %s%s", (this->inverted?"I":""), (this->map_unipolar?"U":""));
      return input_info;
    }

    virtual const char *getInputValue() override {
      /*Serial.printf("%c: getInputValue() working on this->currentValue ", this->name);
      Serial.println(this->currentValue);
      Serial.printf("%c: normal_value is ", this->name);
      Serial.println(this->currentValue * 100); //get_normal_value((double)this->currentValue));*/
      static char fmt[20] = "          ";
      //sprintf(fmt, "[%-3i%%]", (int)(this->get_normal_value((double)this->currentValue)*100.0));
      sprintf(fmt, "[%-3i%%]", (int)(this->currentValue*100.0));
      return fmt;
    }

    virtual void read() override {
      if (this->debug) Serial.printf("%s: read() in AnalogParameterInputBase..", this->name); 

      //DataType currentValue = analogRead(inputPin);
      currentValue = 0.5f;  // TODO: refactor this

      if (is_significant_change(currentValue, this->lastValue)) {
      //if (abs(currentValue - this->lastValue) > this->sensitivity) {
        this->lastValue = this->currentValue;
        this->currentValue = currentValue;

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
            if (this->inverted) Serial.print(F(" - inverted"));
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