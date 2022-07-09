#include "VoltageSource.h"

//#include "ADS1X15.h"

// for 1115 ADC modules with 5v range
template<class ADS1X15Type>
class ADSVoltageSource : public VoltageSourceBase {
    public:
        byte channel = 0;
        ADS1X15Type *ads_source;

        float correction_value_1 = 0.976937;
        float correction_value_2 = 0.0123321;

        ADSVoltageSource(ADS1X15Type *ads_source, byte channel, float maximum_input_voltage = 5.0) {
            this->ads_source = ads_source;
            this->channel = channel;
            this->maximum_input_voltage = maximum_input_voltage;
        }

        // ask the ADC for its current voltage
        virtual double fetch_current_voltage() {
            int16_t value = ads_source->readADC(channel);
            double voltageFromAdc = ads_source->toVoltage(value);
            if ((int)voltageFromAdc==ADS1X15_INVALID_VOLTAGE)
                return 0.0;
            
            return this->get_corrected_voltage(voltageFromAdc);
        }

        // correct for non-linearity
        double get_corrected_voltage (double voltageFromAdc) {
            // TODO: what is the maths behind this?  make configurable, etc 
            // from empirical measuring of received voltage and asked wolfram alpha to figure it out:-
            //  1v: v=1008        = 0.99206349206
            //  2v: v=2031-2034   = 0.98473658296
            //  3v: v=3060        = 0.98039215686
            //  4v: v=4086-4089   = 0.9789525208
            // https://www.wolframalpha.com/input?i=linear+fit+%7B%7B1.008%2C1%7D%2C+%7B2.034%2C2%7D%2C+%7B3.063%2C3%7D%2C+%7B4.086%2C4%7D%2C+%7B5.1%2C5%7D%7D
            return (voltageFromAdc * correction_value_1) + correction_value_2;
        };
};
