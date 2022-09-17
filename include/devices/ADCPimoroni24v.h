#include "ADCDevice.h"
#include "../voltage_sources/ADS24vVoltageSource.h"
#include "../parameter_inputs/VoltageParameterInput.h"

#define MAX_INPUT_VOLTAGE_24V   10
#define ADSDeviceClass ADS1015

ADS1015 ADS_OBJECT_24V(ENABLE_CV_INPUT);

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

        ADCPimoroni24v (uint8_t address, int max_input_voltage = MAX_INPUT_VOLTAGE_24V, uint8_t gain = 2) : ADCPimoroni24v () {
            this->address = address;
            this->max_input_voltage = max_input_voltage;
            this->gain = gain;
        }

        virtual void init() override {
            Serial.println("ADCPimoroni24v#init() initialising!");
            if (this->actual_device!=nullptr) {
                Serial.println("\t..already has actual_device set, returning without doing anything");
                return;
            }
            if (this->initialised) {
                Serial.println("\ti..nitialised flag already set, returning without doing anything");
                return;
            }                

            Serial.println("\t..instantiating an object of ADSDeviceClass..");
            this->actual_device = &ADS_OBJECT_24V; //new ADSDeviceClass(address);
            this->actual_device->begin();
            this->actual_device->setGain(gain);
            this->initialised = true;
            Serial.println("\t..instantiated!"); // an object of ADSDeviceClass");
        }

        virtual VoltageSourceBase *make_voltage_source(int i) override {
            Serial.printf("ADCPimoroni24v#make_voltage_source(%i)..\n", i);
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