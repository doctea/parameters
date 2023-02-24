#ifndef PARAMETER_TOGGLEMENUITEMS__INCLUDED
#define PARAMETER_TOGGLEMENUITEMS__INCLUDED

//#include "parameters/Parameter.h"
#include "ParameterMenuItems.h"
#include "menuitems.h"

class FloatParameter;

class ToggleParameterControl : public ParameterValueMenuItem {
    public:

    float value_on = 1.0;
    float value_off = 0.0;

    ToggleParameterControl(char *label, FloatParameter *parameter) : ParameterValueMenuItem(label, parameter) {}

    virtual bool action_opened() override {
        Debug_println(F("ToggleParameterControl#action_opened"));
        if (parameter->getCurrentNormalValue()<0.5) {
            Debug_println(F("sending value_on"));
            //parameter->setParamValue(value_on);
            parameter->updateValueFromNormal(value_on);
        } else {
            Debug_println(F("sending value_off"));
            parameter->updateValueFromNormal(value_off);
        }
        //this->parameter->setParamValue(((DataParameter*)parameter)->getCurrentValue() < 0.5);
        return false;   // don't 'open'
    }

};

#endif