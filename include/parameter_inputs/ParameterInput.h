#ifndef PARAMETER_INPUT__INCLUDED
#define PARAMETER_INPUT__INCLUDED

#include <Arduino.h>

#ifdef ENABLE_SCREEN
  #include "menu.h"
#endif
#include "../parameters/Parameter.h"

enum VALUE_TYPE {
  BIPOLAR,
  UNIPOLAR
};

class BaseParameterInput {
  public:

    bool debug = false;
    void setDebug(bool state) {
      this->debug = state;
    }
    void toggleDebug() {
      this->debug =! this->debug;
    }

    char name = 'Z';

    BaseParameter *target_parameter = nullptr;

    bool inverted = false;
    bool map_unipolar_to_bipolar = false;

    byte input_type = BIPOLAR;
    byte output_type = UNIPOLAR;

    BaseParameterInput() {
      this->name = ++NEXT_PARAMETER_NAME;
    }
    BaseParameterInput(BaseParameter *target_parameter) {
      this->target_parameter = target_parameter;
    }

    virtual void setTarget(BaseParameter *target) {
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

    virtual const char *getInputValue() {
      return "?Base?";
    }
    virtual const char *getOutputValue() {
      return "?Base?";
    }

    virtual void setInverted(bool invert = true) {
      this->inverted = invert;
      #ifdef ENABLE_PRINTF
        Serial.printf("%s: SET INVERTED on an AnalogParameterInput!", this->name);
      #endif
    }

    virtual void loop();

    virtual double get_normal_value() {
      return 0.0;
    }

    virtual bool matches_label(char *label) {
      //if (strcmp(this->label,label)==0) return true;
      return (this->name==label[0] && label[1]=='\0');
    }
    virtual bool matches_label(char label) {
      return this->name==label;
    }

    /*#ifdef ENABLE_SCREEN
      virtual void addMenuItems(Menu *menu);
    #endif*/

};

template<class TargetClass>
class ParameterInput : public BaseParameterInput {
  public:
    int inputPin = 0;
    //TargetClass *target_parameter = nullptr;
    bool debug = false;

    ParameterInput() {
      this->name = ++NEXT_PARAMETER_NAME;
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