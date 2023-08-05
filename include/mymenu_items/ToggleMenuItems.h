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

    virtual bool action_opened() override;

};

#endif