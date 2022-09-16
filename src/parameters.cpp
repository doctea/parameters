char NEXT_PARAMETER_NAME = 'A';

#ifdef ENABLE_SCREEN 

#include "parameters/Parameter.h"

#include "parameter_inputs/ParameterInput.h"

//template<class TargetClass, class DataType>
/*void DataParameter::on_unbound(BaseParameterInput *input) {
    this->setParamValue(0.0f);
}*/

#include "menu.h"

//#include "Parameter.h"
#include "parameters/ToggleParameter.h"
#include "mymenu_items/ToggleMenuItems.h"

MenuItem *DoubleParameter::makeControl() {
    Serial.printf("DataParameter#makeControl for %s\n", this->label);
    ParameterMenuItem *mi = new ParameterMenuItem(this->label, this);
    Serial.printf("makeControl() in %s:- getCurrentValue()=", this->label);
    Serial.print(this->getCurrentNormalValue());
    Serial.printf(", maximum_value=");
    Serial.print(this->maximumNormalValue);
    Serial.println("<<<<<<<<<<<<<");
    mi->minimum_value = this->minimumNormalValue;
    mi->maximum_value = this->maximumNormalValue;
    mi->internal_value = this->getCurrentNormalValue() * 100.0; // * this->maximum_value;
    Serial.print("\tInitialised internal_value to ");
    Serial.println(mi->internal_value);

    return mi;
}

#endif
