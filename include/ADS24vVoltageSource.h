#include "VoltageSource.h"

// for the Pimoroni +/- 24v 1015 module
template<class ADS1X15Type>
class ADS24vVoltageSource : public VoltageSourceBase {
    public:
        byte channel = 0;
        ADS1X15Type *ads_source;

        float correction_value_1 = 1188.0;
        float correction_value_2 = 0.041;

        double current_value = 0.0;
        double last_value = 0.0;

        double maximum_input_voltage = 10.0;

        ADS24vVoltageSource(ADS1X15Type *ads_source, byte channel, float maximum_input_voltage = 10.0) {
            this->ads_source = ads_source;
            this->channel = channel;
            this->maximum_input_voltage = maximum_input_voltage;
        }

        virtual double fetch_current_voltage() {
            int16_t value = ads_source->readADC(channel);

            float f = ((float)value/correction_value_1) - 1.0;
            f = f / correction_value_2;

            return f;
        }

};