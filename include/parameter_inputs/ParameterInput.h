#ifndef PARAMETER_INPUT__INCLUDED
#define PARAMETER_INPUT__INCLUDED

#include <Arduino.h>

#include "ads.h"

#ifdef ENABLE_SCREEN
  #include "menu.h"
#endif
//#include "../parameters/Parameter.h"

enum VALUE_TYPE {
  BIPOLAR,
  UNIPOLAR
};

#define MAX_INPUT_NAME 10

class BaseParameterInput {
  public:

    bool debug = false;
    void setDebug(bool state) {
      this->debug = state;
    }
    void toggleDebug() {
      this->debug =! this->debug;
    }

    char name[MAX_INPUT_NAME] = "Unnamed";

    //BaseParameter *target_parameter = nullptr;

    bool inverted = false;
    bool map_unipolar_to_bipolar = false;

    byte input_type = BIPOLAR;
    byte output_type = UNIPOLAR;

    uint16_t colour = 0xFFFF;

    BaseParameterInput(char *name) {
      strcpy(this->name, name);
      //this->name = ++NEXT_PARAMETER_NAME;
    }
    virtual ~BaseParameterInput() = default;

    /*BaseParameterInput(BaseParameter *target_parameter) {
      this->target_parameter = target_parameter;
    }*/

    /*virtual void setTarget(BaseParameter *target) {
      if (this->target_parameter!=nullptr) {
        // already assigned to a target; notify the target that its been unbound, in case we need to set parameter back to zero
        this->target_parameter->on_unbound(this);
      }
      this->target_parameter = target;
    }*/

    virtual const char* getFormattedValue() {
      /*if (target_parameter!=nullptr)
        return target_parameter->getFormattedValue();
      else */
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

    virtual const char *getExtra() {
      return "baseExtra";
    }

    virtual void setInverted(bool invert = true) {
      this->inverted = invert;
      #ifdef ENABLE_PRINTF
        Serial.printf("%s: SET INVERTED on an AnalogParameterInput!", this->name);
      #endif
    }

    virtual void loop() {}

    virtual double get_normal_value() {
      return 0.0;
    }

    virtual bool matches_label(char *label) {
      return (strcmp(label, this->name)==0);
    }

};

class ParameterInput : public BaseParameterInput {
  public:
    ParameterInput(char *name) : BaseParameterInput(name) {}

    virtual void read() {};
    virtual void loop() {
      read();
    }
};

#endif