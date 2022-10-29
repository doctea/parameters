#ifndef ANALOGPARAMETERINPUTBASE__INCLUDED
#define ANALOGPARAMETERINPUTBASE__INCLUDED

#include "ParameterInput.h"

class Menu;

template<class DataType = double>
class AnalogParameterInputBase : public ParameterInput {
  public:
    DataType lastValue = 0;
    DataType currentValue = 0;

    DataType min_input_value = -1.0d;
    DataType max_input_value = 1.0d;

    using Callback = void (*)(float);
    Callback callback = nullptr;

    DataType sensitivity = 0.005;
      
    AnalogParameterInputBase() {};
    AnalogParameterInputBase(char *name, DataType in_sensitivity = 0.005, byte input_type = BIPOLAR, byte output_type = UNIPOLAR) : ParameterInput(name) {
      //this->name = name;
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
      static char fmt[20] = "          ";
      sprintf(fmt, "[%-3i%%]", (int)(this->currentValue*100.0));
      return fmt;
    }
    // for some reason, this prevents boot if uncommented?!
    virtual const char *getOutputValue() override {
      static char fmt[20] = "          ";
      //sprintf(fmt, "[%-3i%%]", (int)(this->get_normal_value((double)this->currentValue)*100.0));
      sprintf(fmt, "[%-3i%%]", (int)(this->get_normal_value()*100.0));
      return fmt;
    }

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
      }
    }
};

#endif