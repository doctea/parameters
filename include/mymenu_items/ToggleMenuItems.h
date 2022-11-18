#include "parameters/Parameter.h"
#include "ParameterMenuItems.h"
#include "menuitems.h"

class ToggleControl : public ParameterValueMenuItem {
    public:

    double value_on = 1.0;
    double value_off = 0.0;

    ToggleControl(char *label, DoubleParameter *parameter) : ParameterValueMenuItem(label, parameter) {}

    virtual bool action_opened() override {
        if (this->debug) Serial.println(F("ToggleControl#action_opened"));
        if (parameter->getCurrentNormalValue()<0.5) {
            if (this->debug) Serial.println(F("sending value_on"));
            //parameter->setParamValue(value_on);
            parameter->updateValueFromNormal(value_on);
        } else {
            if (this->debug) Serial.println(F("sending value_off"));
            parameter->updateValueFromNormal(value_off);
        }
        //this->parameter->setParamValue(((DataParameter*)parameter)->getCurrentValue() < 0.5);
        return false;   // don't 'open'
    }

};
