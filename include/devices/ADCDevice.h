#ifndef ADCDEVICE__INCLUDED
#define ADCDEVICE__INCLUDED


class VoltageSourceBase;

class ADCDeviceBase {
    public:
        ADCDeviceBase () {

        }

        virtual void init() {};
        // create the next appropriate VoltageSource object; returns nullptr if there's none left
        virtual VoltageSourceBase *make_voltage_source();
        virtual VoltageSourceBase *make_voltage_source(int i) = 0;

};

#endif