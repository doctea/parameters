#include "../parameters/Parameter.h"
#include "ParameterMenuItems.h"
#include "menuitems.h"

class ToggleControl : public ParameterMenuItem {
    public:

    double value_on = 1.0;
    double value_off = 0.0;

    ToggleControl(char *label, DataParameter *parameter) : ParameterMenuItem(label, parameter) {}

    virtual bool action_opened() override {
        if (this->debug) Serial.println("ToggleControl#action_opened");
        if (parameter->getCurrentValue()<0.5) {
            if (this->debug) Serial.println("sending value_on");
            parameter->setParamValue(value_on);
        } else {
            if (this->debug) Serial.println("sending value_off");
            parameter->setParamValue(value_off);
        }
        //this->parameter->setParamValue(((DataParameter*)parameter)->getCurrentValue() < 0.5);
        return false;   // don't 'open'
    }

};
