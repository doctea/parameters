#include "devices/ADCDevice.h"

//#include "voltage_sources/VoltageSource.h"
//#include "parameter_inputs/ParameterInput.h"

class VoltageSourceBase;

// create the next appropriate VoltageSource object; returns nullptr if there's none left
VoltageSourceBase *ADCDeviceBase::make_voltage_source() {
    static int count = 0;
    return this->make_voltage_source(count++);
};

