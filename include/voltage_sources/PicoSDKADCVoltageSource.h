#pragma once

#ifdef ENABLE_CV_INPUT

#include "debug.h"

#include "VoltageSource.h"

#include "computer.h"

#ifdef ENABLE_SCREEN
    //#include "menuitems.h"
    class MenuItem;
    class Menu;
#endif

class WorkshopVoltageSourceBase : public VoltageSourceBase {
    public:
        float correction_value_1 = 1.0; //0.976937;
        float correction_value_2 = 0.0; //0.0123321;

        WorkshopVoltageSourceBase(int global_slot, bool supports_pitch = false) : VoltageSourceBase(global_slot, supports_pitch) {}

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
            Serial.printf("WorkshopVoltageSource calibration data for slot %i: correction_value_1=%6.6f : correction_value_2=%6.6f\n", global_slot, this->correction_value_1, this->correction_value_2);
            Serial.printf("correction_value_1=%6.6f\n", this->correction_value_1);
            Serial.printf("correction_value_2=%6.6f\n", this->correction_value_2);
        }
        
};

class WorkshopVoltageSource : public WorkshopVoltageSourceBase {
    public:
        byte bank = 0;
        byte channel = 0;

        WorkshopVoltageSource(int global_slot, byte bank, byte channel, float maximum_input_voltage = 5.0, bool supports_pitch = false) 
            : WorkshopVoltageSourceBase(global_slot, supports_pitch) {
            this->bank = bank;
            this->channel = channel;
            this->maximum_input_voltage = maximum_input_voltage;
            this->debug = true;
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

            gpio_put(MX_A, ((bank >> 0)) & 1);
            gpio_put(MX_B, ((bank >> 1)) & 1);
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

            return voltageCorrected - maximum_input_voltage;
        }

        virtual float adcread_to_voltage(int16_t adcReading) {
            float voltageFromAdc = adcReading * (maximum_input_voltage / 4095.0); // 12 bit ADC, 0-4095
            return voltageFromAdc;
        }

        virtual float get_corrected_voltage(float voltageFromAdc) {
            return (voltageFromAdc * correction_value_1) + correction_value_2;
        };

};

#endif