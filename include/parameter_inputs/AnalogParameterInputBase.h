#ifndef ANALOGPARAMETERINPUTBASE__INCLUDED
#define ANALOGPARAMETERINPUTBASE__INCLUDED

#include <Arduino.h>

#include "ParameterInput.h"

class Menu;

template<class DataType = float>
class AnalogParameterInputBase : public ParameterInput {
  public:
    DataType lastValue = 0;
    DataType currentValue = 0;

    DataType min_input_value = -1.0d;
    DataType max_input_value = 1.0d;

    #ifdef PARAMETER_INPUTS_USE_CALLBACKS
      using Callback = void (*)(float);
      Callback callback = nullptr;
    #endif

    DataType sensitivity = 0.005;
      
    AnalogParameterInputBase() {};
    AnalogParameterInputBase(char *name, const char *group_name = "General", DataType in_sensitivity = 0.005, VALUE_TYPE input_type = BIPOLAR) : ParameterInput(name, group_name) {
      //this->name = name;
      this->sensitivity = in_sensitivity;
      this->input_type = input_type;
    }

    virtual void setInverted(bool invert = true) {
      this->inverted = invert;
      #ifdef ENABLE_PRINTF
        Debug_printf(F("%s: SET INVERTED on an AnalogParameterInput!"), this->name);
      #endif
    }

    virtual void loop () override {
      read();
    }

    virtual DataType get_normal_value_unipolar() override {
      return this->get_normal_value(this->currentValue, UNIPOLAR);
    }
    virtual DataType get_normal_value_bipolar() override {
      return this->get_normal_value(this->currentValue, BIPOLAR);
    }

    virtual DataType get_normal_value(DataType value, int output_type) {
      if (this->input_type==UNIPOLAR) {
        value = constrain(value, 0.0f, 1.0f);
      } else if (this->input_type==BIPOLAR) {
        value = constrain(value, -1.0f, 1.0f);
        //value = constrain(value, -1.0f, 1.0f) / 2.0f;  // todo: experiment with this, as currently i believe bipolar output values are twice what they should be
      }

      if (this->input_type==output_type) {
        // dont need to do anything
      } else if (this->input_type==BIPOLAR && output_type==UNIPOLAR) {
        value = 0.5f + (value/2.0f);
      } else if (this->input_type==UNIPOLAR && output_type==BIPOLAR) {
        value = -1.0 + (value * 2.0f);
        //value = -1.0 + (value); // * 2.0f; // todo: experiment with this, as currently i believe bipolar output values are twice what they should be
      }

      if (this->inverted) {
        value = 1.0f - ((float)value); // / this->max_input_value);
      }

      return value;
    }

    virtual bool is_significant_change(DataType currentValue, DataType lastValue) {
      return abs(currentValue - this->lastValue) >= this->sensitivity;
    }

    virtual const char *getInputInfo() {
      /*static char input_info[20] = "                ";

      //snprintf(input_info, 20, "Anlg %s%s", (this->inverted?"I":"")); //, (this->map_unipolar_to_bipolar?"U":""));
      return input_info;*/
      return "Analogue";
    }

    virtual const char *getInputValue() override {
      static char fmt[20] = "          ";
      snprintf(fmt, 20, "[%-3i%%]", (int)(this->currentValue*100.0));
      return fmt;
    }
    
    virtual const char *getOutputValue() override {
      static char fmt[20] = "          ";
      //sprintf(fmt, "[%-3i%%]", (int)(this->get_normal_value((float)this->currentValue)*100.0));
      snprintf(fmt, 20, "[%-3i%%]", (int)(this->get_normal_value_unipolar()*100.0));
      return fmt;
    }

    virtual void read() override {
      #ifdef ENABLE_PRINTF
        Debug_printf(F("%s: read() in AnalogParameterInputBase.."), this->name); 
      #endif

      //DataType currentValue = analogRead(inputPin);
      currentValue = 0.5f;  // TODO: refactor this

      if (is_significant_change(currentValue, this->lastValue)) {
        this->lastValue = this->currentValue;
        this->currentValue = currentValue;

        #ifdef PARAMETER_INPUTS_USE_CALLBACKS
          float normal = get_normal_value_unipolar(currentValue);
          this->on_value_read(currentValue);
          if (callback != NULL) {
            Debug_print(this->name);
            Debug_print(F(": calling callback("));
            Debug_print(normal);
            Debug_println(')');
            callback(normal);
          }
        #endif
      }
    }
};

#endif