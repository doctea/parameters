#ifndef ADSVOLTAGESOURCE__INCLUDED
#define ADSVOLTAGESOURCE__INCLUDED

#include "debug.h"

#include "VoltageSource.h"
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

        ADSVoltageSourceBase(int slot, bool supports_pitch = false) : VoltageSourceBase(slot, supports_pitch) {}

        #ifdef ENABLE_SCREEN
            FLASHMEM virtual MenuItem *makeCalibrationControls(int i) override;
            //virtual MenuItem *makeCalibrationLoadSaveControls(int i) override;
        #endif

        virtual void load_calibration() override;
        virtual void save_calibration() override;
        
};

// for generic 1115 ADC modules with 5v range
template<class ADS1X15Type>
class ADSVoltageSource : public ADSVoltageSourceBase {
    public:
        byte channel = 0;
        ADS1X15Type *ads_source;

        ADSVoltageSource(int slot, ADS1X15Type *ads_source, byte channel, float maximum_input_voltage = 5.0, bool supports_pitch = false) 
        : ADSVoltageSourceBase(slot, supports_pitch) {
            this->ads_source = ads_source;
            this->channel = channel;
            this->maximum_input_voltage = maximum_input_voltage;
        }

        // ask the ADC for its current voltage
        /*virtual double fetch_current_voltage() {
            //int16_t value = ads_source->readADC(channel);
            int16_t value1 = ads_source->readADC(channel);
            int16_t value2 = ads_source->readADC(channel);
            int16_t value3 = ads_source->readADC(channel);

            int value = (value1+value2+value3) / 3;

            double voltageFromAdc = ads_source->toVoltage(value);
            if ((int)voltageFromAdc==ADS1X15_INVALID_VOLTAGE)
                return 0.0;
            
            return this->get_corrected_voltage(voltageFromAdc);
        }*/
        virtual double fetch_current_voltage() {
            static bool already_succeeded = false;
            if (this->debug) {
                Debug_println(F("in ADSVoltageSource#fetch_current_voltage().."));
                Debug_printf(F("\tads_source is @%p, reading from channel %i\n"), this->ads_source, this->channel);
            }            
            if (!already_succeeded) 
                Debug_printf(F("ADSVoltageSource#fetch_current_voltage reading from channel %i, check you're using correct address ADC board if crash here!\n"), this->channel);

            #ifndef FAST_VOLTAGE_READS
                // do three readings from ADC and average them
                //int16_t adcReading = ads_source->readADC(channel);
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

            double voltageFromAdc = this->adcread_to_voltage(adcReading);

            double voltageCorrected = this->get_corrected_voltage(voltageFromAdc);

            if (this->debug) {
                Debug_print(F(" after correction stage 2 got "));
                Debug_println(voltageCorrected);
            }

            if (this->debug) Debug_printf(F("in ADSVoltageSource#fetch_current_voltage() finishing (and returning %f)\n"), voltageCorrected);

            return voltageCorrected;
        }

        virtual double adcread_to_voltage(int16_t adcReading) {
            double voltageFromAdc = ads_source->toVoltage(adcReading);
            if ((int)voltageFromAdc==ADS1X15_INVALID_VOLTAGE)
                return 0.0;
            return voltageFromAdc;
        }

        // correct for non-linearity
        virtual double get_corrected_voltage(double voltageFromAdc) {
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