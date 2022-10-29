#ifndef ADCDEVICE__INCLUDED
#define ADCDEVICE__INCLUDED

#include "../voltage_sources/VoltageSource.h"
#include "../parameter_inputs/ParameterInput.h"

class ADCDeviceBase {
    public:
        ADCDeviceBase () {

        }

        virtual void init() {
            // initialisation
        }

        // create the next appropriate VoltageSource object; returns nullptr if there's none left
        virtual VoltageSourceBase *make_voltage_source() {
            static int count = 0;
            return this->make_voltage_source(count++);
        };
        virtual VoltageSourceBase *make_voltage_source(int i);

};

#endif