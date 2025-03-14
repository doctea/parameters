#pragma once

#include "parameter_inputs/VoltageParameterInput.h"

class DAC8574;
class VoltageParameterInput;

uint16_t calibrate_find_dac_value_for(DAC8574 *dac_output, int channel, VoltageParameterInput *src, float intended_voltage, bool inverted);
uint16_t calibrate_find_dac_value_for(DAC8574 *dac_output, int channel, char *input_name, float intended_voltage, bool inverted);

