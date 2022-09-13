#ifndef ADS24v__INCLUDED
#define ADS24v__INCLUDED

#include "VoltageSource.h"
#include "ADS1X15.h"
// for the Pimoroni +/- 24v 1015 module
template<class ADS1X15Type>
class ADS24vVoltageSource : public VoltageSourceBase {
    public:
        byte channel = 0;
        ADS1X15Type *ads_source;

        float correction_value_1 = 1188.0;
        float correction_value_2 = 0.041;

        ADS24vVoltageSource(ADS1X15Type *ads_source, byte channel, float maximum_input_voltage = 10.0) {
            this->ads_source = ads_source;
            this->channel = channel;
            this->maximum_input_voltage = maximum_input_voltage;
        }

        virtual double fetch_current_voltage() {
            static bool already_succeeded = false;
            if (this->debug) {
                Serial.println("in ADS24vVoltageSource#fetch_current_voltage()..");
                Serial.printf("\tads_source is @%p, reading from channel %i\n", this->ads_source, this->channel);
            }            
            if (!already_succeeded) 
                Serial.printf("ADS24vVoltageSource#fetch_current_voltage about to read from channel %i -- if we crash at this point, check that you're using the corrrect address for your ADC board!\n", this->channel);

            //int16_t value = ads_source->readADC(channel);
            int16_t value1 = ads_source->readADC(channel);
            int16_t value2 = ads_source->readADC(channel);
            int16_t value3 = ads_source->readADC(channel);

            already_succeeded = true;
            //int16_t value1, value2, value3;

            int value = (value1+value2+value3) / 3;

            if (this->debug) {
                Serial.printf("ADS24vVoltageSource channel %i read ADC value %i\t :", channel, value); Serial.flush();
            }

            float f = ((float)value/correction_value_1) - 1.0;
            if (this->debug) {
                Serial.printf("after applying correction stage 1, got ");
                Serial.print(f);
                Serial.print("\t, ");
            }
            f = f / correction_value_2;
            if (this->debug) {
                Serial.print(" after correction stage 2 got ");
                Serial.println(f);
            }

            if (this->debug) Serial.printf("in ADS24vVoltageSource#fetch_current_voltage() finishing (and returning %f)\n", f);

            return f;
        }

};

#endif