#ifndef PARAMETER_INPUT__INCLUDED
#define PARAMETER_INPUT__INCLUDED

#include <Arduino.h>
#include <String>

#include "ads.h"

#include "debug.h"

#include "LinkedList.h"

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

    uint8_t input_type = BIPOLAR;
    uint8_t output_type = UNIPOLAR;

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
        Debug_printf(F("%s: SET INVERTED on an AnalogParameterInput!"), this->name);
      #endif
    }

    virtual void loop() {}

    virtual float get_normal_value() {
      return 0.0;
    }

    virtual bool matches_label(const char *label) {
      return (strcmp(label, this->name)==0);
    }

    static const char *prefix; // = "parameter_input_";  // have to initialise these in the cpp file apparently
    static const char *input_type_suffix; // = "_input_type";
    static const char *output_type_suffix; // = "_output_type";
    virtual LinkedList<String> *save_sequence_add_lines(LinkedList<String> *lines) {
      // eg,
      //  parameter_input_A_input_type=bipolar
      //  parameter_input_B_output_type=unipolar
      
      lines->add(String(prefix) + String(this->name) + String(input_type_suffix) 
        + String('=') + String(this->input_type==BIPOLAR ? "bipolar" : "unipolar"));
      lines->add(String(prefix) + String(this->name) + String(output_type_suffix) 
        + String('=') + String(this->input_type==BIPOLAR ? "bipolar" : "unipolar"));

      return lines;
    }

    virtual bool load_parse_key_value(String key, String value) {
      if(key.startsWith(prefix))
        key.replace(prefix,"");
        uint8_t *target = nullptr;
        if (key.endsWith(input_type_suffix)) {
          key.replace(input_type_suffix,"");
          target = &this->input_type;
        } else if (key.endsWith(output_type_suffix)) {
          key.replace(output_type_suffix,"");
          target = &this->output_type;
        }
        if (target!=nullptr && key.equals(this->name)) {
          if (value.equals("bipolar")) {
            this->input_type = BIPOLAR;
            return true;
          } else if (value.equals("unipolar")) {
            this->input_type = UNIPOLAR;
            return true;
          }
        }
        return false;
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