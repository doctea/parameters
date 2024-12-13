#pragma once

extern int initialised_voltage_source_count;

class VoltageSourceBase;

class ADCDeviceBase {
    public:
        ADCDeviceBase () {

        }

        virtual void init() {};
        // create the next appropriate VoltageSource object; returns nullptr if there's none left
        virtual VoltageSourceBase *make_voltage_source();
        virtual VoltageSourceBase *make_voltage_source(int global_slot) = 0;

};
