#include "parameter_inputs/VoltageParameterInput.h"


uint16_t calibrate_find_dac_value_for(int channel, VoltageParameterInput *src, float intended_voltage, bool inverted);
uint16_t calibrate_find_dac_value_for(int channel, char *input_name, float intended_voltage, bool inverted);

