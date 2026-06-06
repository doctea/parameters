// classes to handle the multipled ADC inputs on the Music Thing Workshop Computer module

#pragma once

#ifdef ENABLE_CV_INPUT

#include "debug.h"

// ADSVoltageSource.h provides ADSVoltageSourceBase (linear calibration base,
// no ADS1X15 dependency) and the ENABLE_SCREEN forward declarations.
#include "ADSVoltageSource.h"

// WorkshopVoltageSourceBase: calibration base for Music Thing Workshop Computer.
// Inherits from ADSVoltageSourceBase to get correction_value_1/2, get_corrected_voltage
// (linear model), compute_calibration (OLS), fetch_calibration_sample, and as_ads_source().
class WorkshopVoltageSourceBase : public ADSVoltageSourceBase {
    public:
        // correction_value_1/2 inherited from ADSVoltageSourceBase (defaults: 1.0, 0.0 set below)

        WorkshopVoltageSourceBase(int global_slot, float maximum_input_voltage = 5.0, bool supports_pitch = false) 
            : ADSVoltageSourceBase(global_slot, maximum_input_voltage, supports_pitch) {
            // Identity correction by default (no distortion on Workshop ADC assumed).
            this->correction_value_1 = 1.0f;
            this->correction_value_2 = 0.0f;
        }
        WorkshopVoltageSourceBase(int global_slot, bool supports_pitch = false) 
            : ADSVoltageSourceBase(global_slot, supports_pitch) {
            this->correction_value_1 = 1.0f;
            this->correction_value_2 = 0.0f;
        }

        #if defined(ENABLE_SCREEN)
            FLASHMEM virtual MenuItem *makeCalibrationControls(int i) override;
            //virtual MenuItem *makeCalibrationLoadSaveControls(int i) override;
        #endif

        #if defined(ENABLE_CALIBRATION_STORAGE)
            virtual void load_calibration() override;
            virtual void save_calibration() override;
        #endif

        // needs_calibration() and as_ads_source() inherited from ADSVoltageSourceBase.

        virtual void output_calibration_data() override {
            Serial.printf("WorkshopVoltageSource calibration data for slot %i: correction_value_1=%6.6f : correction_value_2=%6.6f\n", global_slot, this->correction_value_1, this->correction_value_2);
            Serial.printf("correction_value_1=%6.6f\n", this->correction_value_1);
            Serial.printf("correction_value_2=%6.6f\n", this->correction_value_2);
        }
        
};

/*class WorkshopVoltageSource : public WorkshopVoltageSourceBase {
    public:
        byte bank = 0;
        byte channel = 0;

        WorkshopVoltageSource(int global_slot, byte bank, byte channel, float maximum_input_voltage = 5.0, bool supports_pitch = false) 
            : WorkshopVoltageSourceBase(global_slot, supports_pitch) {
            this->bank = bank;
            this->channel = channel;
            this->maximum_input_voltage = maximum_input_voltage;
            //this->debug = true;
        }

        // returns the last read raw voltage value
        virtual float get_voltage() override {
            //this->update();
            return this->current_value;
        }

        bool already_succeeded = false;
        virtual float fetch_current_voltage() override {
            if (this->debug) {
                Serial.printf("in WorkshopVoltageSource#fetch_current_voltage(slot=%i, bank=%i, channel=%i)..", global_slot, bank, channel);
                //Debug_printf(F("\tads_source is @%p, reading from channel %i\n"), this->ads_source, this->channel);
            }            
            if (!already_succeeded) 
                Debug_printf("WorkshopVoltageSource#fetch_current_voltage reading from channel %i, check you're using correct address ADC board if crash here!\n", this->channel);

            gpio_put(MX_A, bank & 1);
            gpio_put(MX_B, bank & 2);

            // NB this seems to need 1us delay for pins to 'settle' before reading.
            sleep_us(1);

            adc_select_input(channel);
            int adcReading = adc_read();

            float voltageFromAdc = this->adcread_to_voltage(adcReading);

            if (this->debug) {
                Serial.printf("WorkshopVoltageSource channel %i read ADC voltageFromAdc %i\t :", channel, adcReading); Serial_flush();
            }

            float voltageCorrected = this->get_corrected_voltage(voltageFromAdc);

            if (this->debug) {
                Serial.print(F(" after correction stage 2 got "));
                Serial.println(voltageCorrected);
            }

            if (this->debug) Serial.printf("in WorkshopVoltageSource#fetch_current_voltage() finishing (and returning %f)\n", voltageCorrected);

            return maximum_input_voltage - voltageCorrected;
        }

        virtual float adcread_to_voltage(int16_t adcReading) {
            float voltageFromAdc = float(adcReading) * (maximum_input_voltage / 4095.0); // 12 bit ADC, 0-4095
            return voltageFromAdc;
        }

        virtual float get_corrected_voltage(float voltageFromAdc) {
            return (voltageFromAdc * correction_value_1) + correction_value_2;
            //return (voltageFromAdc + correction_value_2) * correction_value_1;
            //return (voltageFromAdc - 0.25) * 2.0;
        }

};*/

#include "ComputerCard.h"

#ifdef LOCK_AROUND_ADC_READ
    #include "core_safe.h" // for acquire_lock()
#endif

class ComputerCardVoltageSource : public WorkshopVoltageSourceBase {
    public:
        byte channel = 0;
        ComputerCard *sw = nullptr;
        int16_t real_value = 0;

        ComputerCardVoltageSource(int global_slot, ComputerCard *sw, byte channel, float maximum_input_voltage = 5.0, bool supports_pitch = false) 
            : WorkshopVoltageSourceBase(global_slot, maximum_input_voltage, supports_pitch) {
            this->sw = sw;
            this->channel = channel;
            //this->debug = true;
        }

        // returns the last read raw voltage value
        virtual float get_voltage() override {
            //this->update();
            return this->current_value;
        }

        bool already_succeeded = false;
        virtual float fetch_current_voltage() override {
            if (this->debug) {
                Serial.printf("in WorkshopVoltageSource#fetch_current_voltage(slot=%i, channel=%i)..", global_slot, channel);
                //Debug_printf(F("\tads_source is @%p, reading from channel %i\n"), this->ads_source, this->channel);
            }            
            if (!already_succeeded) 
                Debug_printf("WorkshopVoltageSource#fetch_current_voltage reading from channel %i, check you're using correct address ADC board if crash here!\n", this->channel);

            
            //int adcReading = channel < 3 ? sw->KnobVal((ComputerCard::Knob)channel) : 
            //                 channel==3 ?  sw->SwitchVal() : sw->CVIn(channel-4);

            int adcReading;
            #ifdef LOCK_AROUND_ADC_READ
                //acquire_lock();
                if (!try_lock()) {
                    //if (Serial) Serial.println(F("ComputerCardVoltageSource#fetch_current_voltage() failed to acquire lock, returning 0"));
                    adcReading = real_value; // failed to acquire lock
                } else {
                
            #endif
            switch (channel) {
                case 0 ... 2:
                    adcReading = sw->KnobVal((ComputerCard::Knob)channel);
                    break;
                case 3:
                    adcReading = sw->SwitchVal();
                    break;
                case 4 ... 5:
                    adcReading = sw->CVIn(channel-4);
                    break;
                case 6 ... 7:
                    adcReading = sw->AudioIn(channel-6);
                    break;
                default:
                    if (Serial) Serial.printf("ComputerCardVoltageSource#fetch_current_voltage() unknown channel %i, returning 0\n", channel);
                    adcReading = 0;
                    break;
            }
            #ifdef LOCK_AROUND_ADC_READ
                release_lock();
                }

            #endif

            real_value = adcReading;
            if (this->debug) {
                Serial.printf("ComputerCardVoltageSource channel %i read ADC value %i\t :", channel, adcReading); Serial_flush();
            }

            if (channel > 3) {
                //adcReading *= 2; // scale up the CV inputs to match the -6 to +6V range of the CV inputs
                // todo: fix this properly?
                //adcReading += 2047; // 0-5V range
            }

            float voltageFromAdc = this->adcread_to_voltage(adcReading);

            if (this->debug && Serial) {
                Serial.printf("WorkshopVoltageSource channel %i read ADC voltageFromAdc %i\t :", channel, adcReading); Serial_flush();
            }

            float voltageCorrected = this->get_corrected_voltage(voltageFromAdc);

            if (this->debug && Serial) {
                Serial.print(F(" after correction stage 2 got "));
                Serial.println(voltageCorrected);
            }

            if (this->debug && Serial) Serial.printf("in WorkshopVoltageSource#fetch_current_voltage() finishing!! (and returning %f)\n", voltageCorrected);

            return maximum_input_voltage - voltageCorrected;
        }

        virtual float adcread_to_voltage(int16_t adcReading) {
            float voltageFromAdc = float(adcReading) * (maximum_input_voltage / 4095.0); // 12 bit ADC, 0-4095
            if (this->debug && Serial) {
                Serial.printf("ComputerCardVoltageSource channel %i read ADC voltageFromAdc %i, converted to voltageFromAdc %3.3f\t :", channel, adcReading, voltageFromAdc); Serial_flush();
            }
            return voltageFromAdc;
        }

        // get_corrected_voltage() inherited from ADSVoltageSourceBase: corrected = cv1 * raw + cv2.

        // Returns pre-correction intermediate voltage for calibration.
        // Uses real_value (cached by fetch_current_voltage) to avoid re-running the channel switch.
        // Model: corrected = cv1 * adcread_to_voltage(raw_adc) + cv2
        // compute_calibration() is inherited from ADSVoltageSourceBase (linear OLS model).
        virtual float fetch_calibration_sample() override {
            fetch_current_voltage();  // refresh real_value
            return adcread_to_voltage(real_value);
        }

};

#endif
