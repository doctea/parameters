#ifndef ANALOGPARAMETERINPUTBASE__INCLUDED
#define ANALOGPARAMETERINPUTBASE__INCLUDED

#include "ParameterInput.h"

#ifdef ENABLE_SCREEN
  /*#include "menu.h"
  #include "menuitems.h"
  #include "mymenu_items/ParameterMenuItems.h"
  #include "mymenu_items/ParameterInputViewMenuItems.h"*/
#endif

class Menu;

template<class TargetClass, class DataType>
class AnalogParameterInputBase : public ParameterInput<TargetClass> {

  public:
    DataType lastValue = 0;
    DataType currentValue = 0;

    DataType min_input_value = -1.0d;
    DataType max_input_value = 1.0d;

    using Callback = void (*)(float);
    Callback callback = nullptr;

    DataType sensitivity = 0.005;
      
    AnalogParameterInputBase() {};
    AnalogParameterInputBase(
      char name, TargetClass &in_target, DataType in_sensitivity = 0.005,
      byte input_type = BIPOLAR, byte output_type = UNIPOLAR
    ) {
      this->name = name;
      this->target_parameter = &in_target;
      this->sensitivity = in_sensitivity;
      this->input_type = input_type;
      this->output_type = output_type;
    }

    virtual void setInverted(bool invert = true) {
      this->inverted = invert;
      #ifdef ENABLE_PRINTF
        Serial.printf("%s: SET INVERTED on an AnalogParameterInput!", this->name);
      #endif
    }

    virtual void loop () override {
      read();
    }

    virtual DataType get_normal_value() override {
      return this->get_normal_value(this->currentValue);
    }

    virtual DataType get_normal_value(DataType value) {
      if (this->debug) {
        Serial.print("get_normal_value(");
        Serial.print(value);
        Serial.print(") ");
      }

      if (this->input_type==UNIPOLAR) {
        value = constrain(value, 0.0, 1.0);
      }

      if (this->input_type==this->output_type) {
        // dont need to do anything
      } else if (this->input_type==BIPOLAR && this->output_type==UNIPOLAR) {
        value = 0.5f + (value/2.0);
      } else if (this->input_type==UNIPOLAR && this->output_type==BIPOLAR) {
        value = -1.0 + (value*2.0);
      }

      if (this->debug) {
        Serial.print("re-normalised to ");
        Serial.print(value);
        Serial.print("\n");
      }

      if (this->inverted) {
        value = 1.0f - ((double)value); // / this->max_input_value);
      }

      if (this->debug) {
        Serial.print(", inverted to ");
        Serial.print(value);
        Serial.print("\n");
      }

      return value;
    }

    virtual bool is_significant_change(DataType currentValue, DataType lastValue) {
      return abs(currentValue - this->lastValue) >= this->sensitivity;
    }

    virtual const char *getInputInfo() {
      static char input_info[20] = "                ";

      sprintf(input_info, "Anlg %s%s", (this->inverted?"I":""), (this->map_unipolar_to_bipolar?"U":""));
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
    // for some reason, this prevents boot if uncommented?!
    /*virtual const char *getOutputValue() override {
      static char fmt[20] = "          ";
      //sprintf(fmt, "[%-3i%%]", (int)(this->get_normal_value((double)this->currentValue)*100.0));
      sprintf(fmt, "[%-3i%%]", (int)(this->get_normal_value()*100.0));
      return fmt;
    }*/


    virtual void read() override {
      #ifdef ENABLE_PRINTF
        if (this->debug) Serial.printf("%s: read() in AnalogParameterInputBase..", this->name); 
      #endif

      //DataType currentValue = analogRead(inputPin);
      currentValue = 0.5f;  // TODO: refactor this

      if (is_significant_change(currentValue, this->lastValue)) {
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
        /*if (this->target_parameter!=nullptr) {
          if (this->debug) {
            Serial.print(this->name);
            Serial.print(F(": calling target setParamValueA("));
            Serial.print(normal);
            Serial.print(F(")"));
            if (this->inverted) Serial.print(F(" - inverted"));
            Serial.println();
          }
          //this->target_parameter->updateValueFromNormal(normal); 
          // TODO: trigger updating of connected parameter mixers?
        }*/
      }
    }

    /*#ifdef ENABLE_SCREEN
      virtual void addMenuItems(Menu *menu) {
            char label[20];
            sprintf(label, "Graph for %c", this->name);

            Serial.printf("\tdoing menu->add for ParameterInputDisplay with label '%s'\n", label);
            menu->add(new ParameterInputDisplay(label, this)); //, LOOP_LENGTH_TICKS));
            sprintf(label, "Input type for %c", this->name);
            menu->add(new InputTypeSelectorControl(label, this, set_input_type, get_input_type));
            sprintf(label, "Output type for %c", this->name);
            menu->add(new InputTypeSelectorControl(label, this, set_output_type, get_output_type));
        }
    #endif*/
};

#endif