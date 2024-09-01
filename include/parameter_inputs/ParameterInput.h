#pragma once

#include "ParameterTypes.h"

#include <Arduino.h>
#include <String>

#include "ads.h"

#include "debug.h"

#include "LinkedList.h"

#ifdef ENABLE_SCREEN
  #include "menu.h"
  #include "submenuitem_bar.h"
#endif
//#include "../parameters/Parameter.h"

#define MAX_INPUT_NAME 10

class ParameterInputCallbackReceiver {
  public:
  virtual void receive_value_update(float value) = 0;
};

class BaseParameterInput {
  public:

    bool debug = false;
    FLASHMEM void setDebug(bool state) {
      this->debug = state;
    }
    FLASHMEM void toggleDebug() {
      this->debug =! this->debug;
    }

    const char *group_name = "General";
    char name[MAX_INPUT_NAME] = "Unnamed";

    bool inverted = false;

    uint8_t input_type = BIPOLAR;

    uint16_t colour = 0xFFFF;
    BaseParameterInput(char *name, const char *group_name = "General") {
      strncpy(this->name, name, MAX_INPUT_NAME);
      this->group_name = group_name;
      //this->name = ++NEXT_PARAMETER_NAME;
    }
    virtual ~BaseParameterInput() = default;

    // whether to show unipolar/bipolar options for this type - override in subclasses
    virtual bool supports_bipolar_input() {
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

    virtual float get_normal_value_unipolar() {
      return 0.0;
    }
    virtual float get_normal_value_bipolar() {
      return 0.0;
    }

    virtual bool matches_label(const char *label) {
      return (strcmp(label, this->name)==0);
    }

    static const char *prefix; // = "parameter_input_";  // have to initialise these in the cpp file apparently
    static const char *input_type_suffix; // = "_input_type";
    virtual LinkedList<String> *save_pattern_add_lines(LinkedList<String> *lines) {
      // eg,
      //  parameter_input_A_input_type=bipolar
      
      lines->add(String(prefix) + String(this->name) + String(input_type_suffix) 
        + String('=') + String(this->input_type==BIPOLAR ? "bipolar" : "unipolar"));
      
      return lines;
    }

    virtual bool load_parse_key_value(String key, String value) {
      if(!key.startsWith(prefix)) return false;

      key.replace(prefix,"");
      uint8_t *target = nullptr;

      if (key.endsWith(input_type_suffix)) {
        key.replace(input_type_suffix,"");
        target = &this->input_type;
      } 

      if (target!=nullptr && key.equals(this->name)) {
        if (value.equals("bipolar")) {        
          *target = BIPOLAR;
          return true;
        } else if (value.equals("unipolar")) {
          *target = UNIPOLAR;
          return true;
        }
      }
      return false;
    }

    #ifdef PARAMETER_INPUTS_USE_CALLBACKS
      LinkedList<ParameterInputCallbackReceiver*> *callback_receivers = new LinkedList<ParameterInputCallbackReceiver*> ();
      virtual void add_parameter_input_callback_receiver(ParameterInputCallbackReceiver *receiver) {
        this->callback_receivers->add(receiver);
      }
      float last_value;
      virtual void on_value_read(float currentValue) {
        if (currentValue==last_value)
          return;
        for (int i = 0 ; i < callback_receivers->size() ; i++) {
          callback_receivers->get(i)->receive_value_update(currentValue);
        }
      }
    #endif

    #ifdef ENABLE_SCREEN
      FLASHMEM
      virtual SubMenuItemBar *makeControls(int16_t memory_size, const char *label_prefix = "");
    #endif

};

class ParameterInput : public BaseParameterInput {
  public:
    ParameterInput(char *name, const char* group_name = "General") : BaseParameterInput(name, group_name) {}

    virtual void read() {};
    virtual void loop() {
      read();
    }
};

