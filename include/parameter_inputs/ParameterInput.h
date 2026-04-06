#pragma once

#include "compatibility.h"

#include "ParameterTypes.h"

#include <Arduino.h>
#include <String>

#include "ads.h"
#include "debug.h"

#include "LinkedList.h"

#ifdef ENABLE_SCREEN
  #include "menu.h"
  #include "submenuitem_bar.h"
  class ParameterInputDisplay;  // so that we can re-use the parameterinputdisplay for combined overview...
#endif

#ifdef ENABLE_STORAGE
    #include "saveload_settings.h"
#endif

#define MAX_INPUT_NAME 10

class ParameterInputCallbackReceiver {
  public:
  virtual void receive_value_update(float value) = 0;
};

class BaseParameterInput : public SHStorage<0, 10> {  // no children; settings for input config
  public:

    bool debug = false;
    FLASHMEM void setDebug(bool state) {
      this->debug = state;
    }
    FLASHMEM void toggleDebug() {
      this->debug =! this->debug;
    }

    #ifdef ENABLE_SCREEN
      ParameterInputDisplay *parameter_input_display = nullptr;  // so that we can re-use the parameterinputdisplay for combined overview...
    #endif

    const char *group_name = "General";
    char name[MAX_INPUT_NAME+1] = "Unnamed";
    char group_and_name[MAX_INPUT_NAME*2+2] = "General:Unnamed";

    bool inverted = false;

    VALUE_TYPE input_type = BIPOLAR;

    uint16_t colour = 0xFFFF;
    BaseParameterInput(char *name, const char *group_name = "General") {
      strncpy(this->name, name, MAX_INPUT_NAME);
      this->group_name = group_name;
      this->set_path_segment(get_group_and_name());
    }
    virtual ~BaseParameterInput() = default;

    const char *get_name() {
      return this->name;
    }

    const char *get_group_and_name() {
      snprintf(this->group_and_name, MAX_INPUT_NAME*2+2, "%s:%s", this->group_name, this->name);
      return this->group_and_name;
    }

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
    virtual bool matches_group_and_label(const char *group_and_label) {
      return (strcmp(group_and_label, this->get_group_and_name())==0);
    }

    #ifdef ENABLE_STORAGE
      virtual void setup_saveable_settings() override {
        ISaveableSettingHost::setup_saveable_settings();

        // @@TODO: probably want to use unpolar/bipolar strings (similar to how we do in Parameter)
        register_setting(
          new LSaveableSetting<VALUE_TYPE>(
            "Input Type",
            "ParameterInput",
            &this->input_type
          ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT
        );
      }
    #endif

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
    ParameterInput(char *name, const char* group_name = "General") 
        : BaseParameterInput(name, group_name) {}

    virtual void read() {};
    virtual void loop() {
      read();
    }
};

