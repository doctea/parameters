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
    FLASHMEM void setDebug(bool state) {
      this->debug = state;
    }
    FLASHMEM void toggleDebug() {
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

    // whether to show unipolar/bipolar options for this type - override in subclasses
    virtual bool supports_bipolar() {
      return true;
    }
    // whether this object type supports pitch readouts, eg 1v/oct CV
    virtual bool supports_pitch() {
      return false;
    }

    virtual const char* getFormattedValue() {
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
    // whether there is any extra info string to display in ParameterInputView controls
    virtual bool hasExtra() {
      return false;
    }

    virtual void setInverted(bool invert = true) {
      this->inverted = invert;
      #ifdef ENABLE_PRINTF
        Serial.printf(F("%s: SET INVERTED on an AnalogParameterInput!"), this->name);
      #endif
    }

    virtual void loop() {}

    virtual double get_normal_value() {
      return 0.0;
    }

    virtual bool matches_label(const char *label) {
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