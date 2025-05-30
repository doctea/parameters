#pragma once

#ifdef ENABLE_CV_INPUT

#include "debug.h"

#include "VoltageSource.h"
#if __has_include("ADS1X15.h")
    #include "ADS1X15.h"

#ifdef ENABLE_SCREEN
    //#include "menuitems.h"
    class MenuItem;
    class Menu;
#endif

class ADSVoltageSourceBase : public VoltageSourceBase {
    public:
        float correction_value_1 = 0.976937;
        float correction_value_2 = 0.0123321;

        ADSVoltageSourceBase(int global_slot, bool supports_pitch = false) : VoltageSourceBase(global_slot, supports_pitch) {}

        #if defined(ENABLE_SCREEN)
            FLASHMEM virtual MenuItem *makeCalibrationControls(int i) override;
            //virtual MenuItem *makeCalibrationLoadSaveControls(int i) override;
        #endif

        #if defined(ENABLE_CALIBRATION_STORAGE)
            virtual void load_calibration() override;
            virtual void save_calibration() override;
        #endif

        virtual bool needs_calibration() override {
            return true;
        }

        virtual void output_calibration_data() override {
            Serial.printf("ADSVoltageSource calibration data for slot %i: correction_value_1=%6.6f : correction_value_2=%6.6f\n", global_slot, this->correction_value_1, this->correction_value_2);
            Serial.printf("correction_value_1=%6.6f\n", this->correction_value_1);
            Serial.printf("correction_value_2=%6.6f\n", this->correction_value_2);
        }
        
};

// for generic 1115 ADC modules with 5v range
template<class ADS1X15Type>
class ADSVoltageSource : public ADSVoltageSourceBase {
    public:
        byte channel = 0;
        ADS1X15Type *ads_source;

        ADSVoltageSource(int global_slot, ADS1X15Type *ads_source, byte channel, float maximum_input_voltage = 5.0, bool supports_pitch = false) 
            : ADSVoltageSourceBase(global_slot, supports_pitch) {
            this->ads_source = ads_source;
            this->channel = channel;
            this->maximum_input_voltage = maximum_input_voltage;
        }

        // ask the ADC for its current voltage
        /*virtual float fetch_current_voltage() {
            //int16_t value = ads_source->readADC(channel);
            int16_t value1 = ads_source->readADC(channel);
            int16_t value2 = ads_source->readADC(channel);
            int16_t value3 = ads_source->readADC(channel);

            int value = (value1+value2+value3) / 3;

            float voltageFromAdc = ads_source->toVoltage(value);
            if ((int)voltageFromAdc==ADS1X15_INVALID_VOLTAGE)
                return 0.0;
            
            return this->get_corrected_voltage(voltageFromAdc);
        }*/
        bool already_succeeded = false;
        virtual float fetch_current_voltage() override {
            if (this->debug) {
                Debug_println(F("in ADSVoltageSource#fetch_current_voltage().."));
                Debug_printf(F("\tads_source is @%p, reading from channel %i\n"), this->ads_source, this->channel);
            }            
            if (!already_succeeded) 
                Debug_printf(F("ADSVoltageSource#fetch_current_voltage reading from channel %i, check you're using correct address ADC board if crash here!\n"), this->channel);

            if (!ads_source->isConnected()) {
                if (this->debug) {
                    Serial.printf("ADSVoltageSource#fetch_current_voltage(): underlying ADC isn't connected!\n");
                }
                return 0.0f;
            }

            #ifndef FAST_VOLTAGE_READS
                // do three readings from ADC and average them
                int16_t value1 = ads_source->readADC(channel);
                int16_t value2 = ads_source->readADC(channel);
                int16_t value3 = ads_source->readADC(channel);

                int adcReading = (value1+value2+value3) / 3;
            #else
                int adcReading = ads_source->readADC(channel);
            #endif

            if (!already_succeeded) 
                Debug_printf(F("ADSVoltageSource#fetch_current_voltage didn't crash on first read, so address is probably ok!\n"), this->channel);
            already_succeeded = true;

            if (this->debug) {
                Debug_printf(F("ADSVoltageSource channel %i read ADC voltageFromAdc %i\t :"), channel, adcReading); Serial_flush();
            }

            float voltageFromAdc = this->adcread_to_voltage(adcReading);

            float voltageCorrected = this->get_corrected_voltage(voltageFromAdc);

            if (this->debug) {
                Debug_print(F(" after correction stage 2 got "));
                Debug_println(voltageCorrected);
            }

            if (this->debug) Debug_printf(F("in ADSVoltageSource#fetch_current_voltage() finishing (and returning %f)\n"), voltageCorrected);

            return voltageCorrected;
        }

        virtual float adcread_to_voltage(int16_t adcReading) {
            float voltageFromAdc = ads_source->toVoltage(adcReading);
            if ((int)voltageFromAdc==ADS1X15_INVALID_VOLTAGE)
                return 0.0;
            return voltageFromAdc;
        }

        // correct for non-linearity
        virtual float get_corrected_voltage(float voltageFromAdc) {
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

#endif

#endif
