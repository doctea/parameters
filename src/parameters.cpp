#include "Parameter.h"

char NEXT_PARAMETER_NAME = 'W';

#include "ParameterInput.h"

void BaseParameter::on_unbound(BaseParameterInput *input) {
    this->setParamValue(0.0f);
}
