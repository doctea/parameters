#include "parameter_inputs/ParameterInput.h"

const char *BaseParameterInput::prefix = "parameter_input_";
const char *BaseParameterInput::input_type_suffix = "_input_type";
#ifdef PARAMETER_INPUTS_USE_OUTPUT_POLARITY
    const char *BaseParameterInput::output_type_suffix = "_output_type";
#endif
