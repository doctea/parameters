#include "mymenu.h"

#include "Parameter.h"
#include "AnalogParameterInput.h"
#include "DigitalParameterInput.h"

class ParameterMenuItem : public DirectNumberControl {
    BaseParameter *parameter = nullptr;

    double internal_value;
    
    int minimum_value = 0; //Parameter->minimum_value;
    int maximum_value = 100; //Parameter->maximum_value;

    public:
        ParameterMenuItem(char*in_label, BaseParameter*parameter) : DirectNumberControl(label) {
            strcpy(label, in_label);
            this->parameter = parameter;
            internal_value = parameter->getCurrentValue();
        }

        virtual int get_current_value() override {
            return parameter->getCurrentValue() * 100.0;    // turn into percentage
        }

        virtual void set_current_value(double value) { 
            if (!this->readOnly)
                parameter->setParamValue(value / 100.0);    // turn into percentage
        }

};