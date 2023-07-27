#include "ADCDevice.h"
#include "../voltage_sources/ADS24vVoltageSource.h"
#include "../parameter_inputs/VoltageParameterInput.h"

#include "debug.h"

#define MAX_INPUT_VOLTAGE_24V   10
#define ADSDeviceClass ADS1015

//ADS1015 ADS_OBJECT_24V(ENABLE_CV_INPUT);

//template<class ADSDeviceClass>
class ADCPimoroni24v : public ADCDeviceBase {
    public:
        bool initialised = false;
        uint8_t address = 0x48;
        uint8_t gain = 2;
        uint8_t max_input_voltage = MAX_INPUT_VOLTAGE_24V;
        uint8_t MAX_CHANNELS = 3;

        ADSDeviceClass *actual_device = nullptr;

        int initialised_sources = 0;

        ADCPimoroni24v () : ADCDeviceBase () {}

        ADCPimoroni24v (uint8_t address, int max_input_voltage = MAX_INPUT_VOLTAGE_24V, uint8_t gain = 2) : ADCPimoroni24v () {
            this->address = address;
            this->max_input_voltage = max_input_voltage;
            this->gain = gain;
        }

        virtual void init() override {
            Debug_println(F("ADCPimoroni24v#init() initialising!"));
            if (this->actual_device!=nullptr) {
                Debug_println(F("\t..already has actual_device set, returning without doing anything"));
                return;
            }
            if (this->initialised) {
                Debug_println(F("\t..initialised flag already set, returning without doing anything"));
                return;
            }                

            Debug_printf(F("\t..instantiating an object of ADSDeviceClass with address %02x..\n"), this->address);
            //this->actual_device = &ADS_OBJECT_24V; //new ADSDeviceClass(address);
            this->actual_device = new ADSDeviceClass(address);
            this->actual_device->begin();
            this->actual_device->setGain(gain);
            this->initialised = true;
            Debug_println(F("\t..instantiated!")); // an object of ADSDeviceClass");
        }

        virtual VoltageSourceBase *make_voltage_source(int global_slot) override {
            Debug_printf(F("ADCPimoroni24v#make_voltage_source(global_slot=%i)..\n"), global_slot);
            if (!this->initialised)
                this->init();
            if (initialised_sources<MAX_CHANNELS)
                return new ADS24vVoltageSource<ADSDeviceClass>(global_slot, this->actual_device, initialised_sources++, max_input_voltage);
            else 
                return nullptr;
        }

};