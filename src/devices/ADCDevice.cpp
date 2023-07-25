#include "devices/ADCDevice.h"

//#include "voltage_sources/VoltageSource.h"
//#include "parameter_inputs/ParameterInput.h"

// for counting how many voltage sources have been initialised globally; used as a clunky way to know which slot number to load/save to
int initialised_voltage_source_count;

class VoltageSourceBase;

// create the next appropriate VoltageSource object; returns nullptr if there's none left
VoltageSourceBase *ADCDeviceBase::make_voltage_source() {
    //static int initialised_voltage_source_count = 0;
    return this->make_voltage_source(initialised_voltage_source_count++);
};

