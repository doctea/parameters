#include "Parameter.h"

char NEXT_PARAMETER_NAME = 'W';

#include "ParameterInput.h"

//template<class TargetClass, class DataType>
/*void DataParameter::on_unbound(BaseParameterInput *input) {
    this->setParamValue(0.0f);
}*/

#include "menu.h"

#include "Parameter.h"
#include "ToggleParameter.h"
#include "ToggleMenuItems.h"

MenuItem *DataParameter::makeControl() {
    return new ParameterMenuItem(this->label, this);
}