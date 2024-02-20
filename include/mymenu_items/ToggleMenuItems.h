#ifndef PARAMETER_TOGGLEMENUITEMS__INCLUDED
#define PARAMETER_TOGGLEMENUITEMS__INCLUDED

//#include "parameters/Parameter.h"
#include "ParameterMenuItems.h"
#include "menuitems.h"

class FloatParameter;

class ToggleParameterControl : public ParameterValueMenuItem {
    public:

    FloatParameter *p_parameter;

    float value_on = 1.0;
    float value_off = 0.0;

    ToggleParameterControl(char *label, FloatParameter *parameter) : 
        p_parameter(parameter), 
        ParameterValueMenuItem(label, &p_parameter) {}

    virtual bool action_opened() override;

};

#endif