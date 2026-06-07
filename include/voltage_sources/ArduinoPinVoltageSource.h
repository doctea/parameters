#ifndef ARDUINOPINVOLTAGESOURCE__INCLUDED
#define ARDUINOPINVOLTAGESOURCE__INCLUDED

#include <Arduino.h>

// ADSVoltageSourceBase (no ADS1X15 dependency) is in ADSVoltageSource.h.
#include "ADSVoltageSource.h"

//#include "ADS1X15.h"

// for reading from Arduino analog pin
class ArduinoPinVoltageSource : public ADSVoltageSourceBase {
    public:
        byte pin = 0;
        // correction_value_1 / correction_value_2 are inherited from ADSVoltageSourceBase

        bool invert = false;

        // get_default_calib_start/end/step inherited from ADSVoltageSourceBase
        // (returns minimum_input_voltage..maximum_input_voltage @ 1V step).

        // Constructor with explicit minimum and maximum voltage range.
        ArduinoPinVoltageSource(int global_slot, byte pin, float minimum_input_voltage, float maximum_input_voltage = 5.0, bool invert = false, bool supports_pitch = false)
            : ADSVoltageSourceBase(global_slot, minimum_input_voltage, maximum_input_voltage, supports_pitch) {
            this->pin = pin;
            this->invert = invert;
            this->correction_value_1 = 1.0f;
            this->correction_value_2 = 0.0f;
            pinMode(pin, INPUT);
        }

        ArduinoPinVoltageSource(int global_slot, byte pin, float maximum_input_voltage = 5.0, bool invert = false, bool supports_pitch = false) 
            : ADSVoltageSourceBase(global_slot, maximum_input_voltage, supports_pitch) {
            this->pin = pin;
            this->invert = invert;
            // Set inherited correction defaults to identity (no correction)
            this->correction_value_1 = 1.0f;
            this->correction_value_2 = 0.0f;
            pinMode(pin,INPUT);
        }

        // ask the ADC for its current voltage
        virtual float fetch_current_voltage() override {
            //int16_t value = ads_source->readADC(channel);
            int16_t value1 = analogRead(this->pin); //ads_source->readADC(channel);
            int16_t value2 = analogRead(this->pin); 
            int16_t value3 = analogRead(this->pin); 

            int value = (value1+value2+value3) / 3;

            if (this->invert) 
                value = 1023 - value;

            float voltage_range = this->maximum_input_voltage - this->minimum_input_voltage;

            //float voltageFromAdc = ((float)value/1024.0) * this->maximum_input_voltage;

            float voltageFromAdc = ((float)value/1024.0) * voltage_range + this->minimum_input_voltage;
            this->_pre_correction_sample = voltageFromAdc;  // cached for fetch_calibration_sample()
            
            return this->get_corrected_voltage(voltageFromAdc);
        }
        // fetch_calibration_sample() inherited from ADSVoltageSourceBase: returns _pre_correction_sample.
        // compute_calibration() inherited from ADSVoltageSourceBase (linear OLS model).

};

#endif