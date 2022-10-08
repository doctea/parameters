#ifndef ADS24v__INCLUDED
#define ADS24v__INCLUDED

#include "VoltageSource.h"
#include "ADSVoltageSource.h"
#include "ADS1X15.h"

// for the Pimoroni +/- 24v 1015 module https://shop.pimoroni.com/products/ads1015-adc-breakout?variant=27859155026003
template<class ADS1X15Type>
class ADS24vVoltageSource : public ADSVoltageSource<ADS1X15Type> {
    public:
        float correction_value_1 = 1182.0;
        float correction_value_2 = 0.041;

        ADS24vVoltageSource(ADS1X15Type *ads_source, byte channel, float maximum_input_voltage = 10.0) :
            ADSVoltageSource<ADS1X15Type>(ads_source, channel, maximum_input_voltage) {
                //this->debug = true;
        }

        virtual double adcread_to_voltage(int16_t adcReading) override {
            // for some reason, seem to have to do this calculation differently (than the parent class) for this board?
            //return ads_source->toVoltage(adcReading);
            double voltageCorrected = ((double)adcReading/this->correction_value_1) - 1.0;
            /*
            // tryna implement same logic as https://github.com/pimoroni/ads1015-python/blob/9ae775bc0a4a148771ef1d36a98adb76cc6cf54e/library/ads1015/__init__.py#L375 
            double voltageCorrected = adcReading * 1000.0;
            voltageCorrected /= 2048.0;
            voltageCorrected *= this->ads_source->getGain();
            voltageCorrected /= 1000.0;
            */
            if (this->debug) {
                Serial.printf("after applying correction stage 1, got ");
                Serial.print(voltageCorrected);
                Serial.print("\t, ");
            }
            return voltageCorrected;
        }

        virtual double get_corrected_voltage(double voltageFromAdc) override {
            double corrected2 = voltageFromAdc / this->correction_value_2;
            /*
            // tryna implement same logic as https://github.com/pimoroni/ads1015-python/blob/9ae775bc0a4a148771ef1d36a98adb76cc6cf54e/library/ads1015/__init__.py#L375 
            double pin_v = voltageFromAdc;
            double input_v = pin_v * ((float)8060000+402000) / (float)8060000;
            input_v += 1.241;
            double corrected2 = input_v;
            */
            if (this->debug) {
                Serial.printf("after applying correction stage 1, got ");
                Serial.print(corrected2);
                Serial.print("\t, ");
            }
            return corrected2;
        }
};

#endif