#include "ADCDevice.h"
#include "../voltage_sources/ADS24vVoltageSource.h"
#include "../parameter_inputs/VoltageParameterInput.h"

#define MAX_INPUT_VOLTAGE_24V   10
#define ADSDeviceClass ADS1015

//template<class ADSDeviceClass>
class ADCPimoroni24v : public ADCDeviceBase {
    public:
        bool initialised = false;
        uint8_t address = 0x48;
        uint8_t gain = 2;
        uint8_t max_input_voltage = MAX_INPUT_VOLTAGE_24V;
        uint8_t MAX_CHANNELS = 3;


        ADSDeviceClass *actual_device = nullptr;

        ADCPimoroni24v () : ADCDeviceBase () {}

        ADCPimoroni24v (uint8_t address, uint8_t gain = 2, int max_input_voltage = MAX_INPUT_VOLTAGE_24V) : ADCPimoroni24v () {
            this->address = address;
            this->gain = gain;
            this->max_input_voltage = max_input_voltage;
        }

        virtual void init() override {
            if (this->actual_device!=nullptr) 
                return;
            if (this->initialised)
                return;

            this->actual_device = new ADSDeviceClass(address);
            this->actual_device->begin();
            this->actual_device->setGain(gain);
        }

        virtual VoltageSourceBase *make_voltage_source(int i) override {
            if (!this->initialised)
                this->init();
            if (i<MAX_CHANNELS)
                return new ADS24vVoltageSource<ADSDeviceClass>(this->actual_device, i, max_input_voltage);
            else 
                return nullptr;
        }

        /*virtual BaseParameterInput *make_input_for_source(char name, ADS24vVoltageSource<ADSDeviceClass> *vs) {
            return new VoltageParameterInput(name, vs);
        }*/

};