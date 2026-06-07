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
        ArduinoPinVoltageSource(int global_slot, byte pin, float minimum_input_voltage, float maximum_input_voltage = 5.0, bool invert = false, bool supports_pitch = false, float smooth_alpha = VOLTAGE_SOURCE_DEFAULT_SMOOTHING)
            : ADSVoltageSourceBase(global_slot, minimum_input_voltage, maximum_input_voltage, supports_pitch, smooth_alpha) {
            this->pin = pin;
            this->invert = invert;
            this->correction_value_1 = 1.0f;
            this->correction_value_2 = 0.0f;
            pinMode(pin, INPUT);
        }

        ArduinoPinVoltageSource(int global_slot, byte pin, float maximum_input_voltage = 5.0, bool invert = false, bool supports_pitch = false, float smooth_alpha = VOLTAGE_SOURCE_DEFAULT_SMOOTHING) 
            : ADSVoltageSourceBase(global_slot, maximum_input_voltage, supports_pitch, smooth_alpha) {
            this->pin = pin;
            this->invert = invert;
            // Set inherited correction defaults to identity (no correction)
            this->correction_value_1 = 1.0f;
            this->correction_value_2 = 0.0f;
            pinMode(pin,INPUT);
        }

        // ask the ADC for its current voltage
        virtual float fetch_current_voltage() override {
            int16_t value;
            #ifdef FAST_VOLTAGE_READS
                value = analogRead(this->pin);
                #ifdef ARDUINO_ARCH_RP2350
                    // if we're on RP2350, apply correction from https://github.com/kitanokitsune/rp2040adc_correction
                    value = this->rp2350adc_correction(value);
                #endif
            #else 
                int16_t value1 = analogRead(this->pin);
                int16_t value2 = analogRead(this->pin); 
                int16_t value3 = analogRead(this->pin); 

                #ifdef ARDUINO_ARCH_RP2350
                    // if we're on RP2350, apply correction from https://github.com/kitanokitsune/rp2040adc_correction
                    value1 = this->rp2350adc_correction(value1);
                    value2 = this->rp2350adc_correction(value2);
                    value3 = this->rp2350adc_correction(value3);
                #endif

                value = (value1+value2+value3) / 3;
            #endif

            if (this->invert) 
                value = 1023 - value;

            float voltage_range = this->maximum_input_voltage - this->minimum_input_voltage;

            float voltageFromAdc = ((float)value/1023.0) * voltage_range + this->minimum_input_voltage;
            this->_pre_correction_sample = voltageFromAdc;  // cached for fetch_calibration_sample()
            
            return this->get_corrected_voltage(voltageFromAdc);
        }
        // fetch_calibration_sample() inherited from ADSVoltageSourceBase: returns _pre_correction_sample.
        // compute_calibration() inherited from ADSVoltageSourceBase (linear OLS model).
        
        int rp2350adc_correction(int d) {
            /*
            rp2350 ADC Correction
            d     : 12 bit ADC readout (0~4095)
            return: corrected ADC value
            */
            int y;
            if (d >= 4079) {
                y = 4095;
            } else if (d >= 3905) {
                y = d + 16;
            } else if (d >= 3839) {
                y = d + 15;
            } else if (d >= 3489) {
                y = d + 14;
            } else if (d >= 3327) {
                y = d + 13;
            } else if (d >= 3105) {
                y = d + 12;
            } else if (d >= 2816) {
                y = d + 11;
            } else if (d >= 2783) {
                y = d + 10;
            } else if (d >= 2559) {
                y = d + 9;
            } else if (d >= 2304) {
                y = d + 10;
            } else if (d >= 2144) {
                y = d + 9;
            } else if (d >= 2047) {
                y = d + 8;
            } else if (d >= 1824) {
                y = d + 9;
            } else if (d >= 1792) {
                y = d + 8;
            } else if (d >= 1535) {
                y = d + 7;
            } else if (d >= 1280) {
                y = d + 8;
            } else if (d >= 1089) {
                y = d + 7;
            } else if (d >= 833) {
                y = d + 6;
            } else if (d >= 767) {
                y = d + 5;
            } else if (d >= 576) {
                y = d + 4;
            } else if (d >= 319) {
                y = d + 3;
            } else if (d >= 255) {
                y = d + 2;
            } else if (d >= 160) {
                y = d + 1;
            } else {
                y = d;
            }

            return y;
        }

};

#endif