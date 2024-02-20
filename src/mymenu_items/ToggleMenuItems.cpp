#include "mymenu_items/ToggleMenuItems.h"

bool ToggleParameterControl::action_opened() {
    Debug_println(F("ToggleParameterControl#action_opened"));
    if ((*parameter)->getCurrentNormalValue()<0.5) {
        Debug_println(F("sending value_on"));
        //parameter->setParamValue(value_on);
        (*parameter)->updateValueFromNormal(value_on);
    } else {
        Debug_println(F("sending value_off"));
        (*parameter)->updateValueFromNormal(value_off);
    }
    //this->parameter->setParamValue(((DataParameter*)parameter)->getCurrentValue() < 0.5);
    return false;   // don't 'open'
}