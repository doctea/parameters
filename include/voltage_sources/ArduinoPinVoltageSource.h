#ifndef ARDUINOPINVOLTAGESOURCE__INCLUDED
#define ARDUINOPINVOLTAGESOURCE__INCLUDED

#include <Arduino.h>

#include "VoltageSource.h"

//#include "ADS1X15.h"

// for reading from Arduino analog pin
class ArduinoPinVoltageSource : public VoltageSourceBase {
    public:
        byte pin = 0;
        float correction_value_1 = 1.0; //0.976937;
        float correction_value_2 = 0.0; //0.0123321;

        // todo: probably move this into VoltageSourceBase and make it more generic, eg have a 'calibration' struct or something that can be used by any VoltageSource, and then have a method on VoltageSourceBase like 'apply_calibration(float raw_voltage)' that is called by the fetch_current_voltage() of each VoltageSource after it has read the raw voltage, and which applies the correction values etc to return the final voltage value?
        float minimum_input_voltage = 0.0f;

        bool invert = false;

        ArduinoPinVoltageSource(int global_slot, byte pin, float minimum_input_voltage, float maximum_input_voltage = 5.0, bool invert = false, bool supports_pitch = false) 
            : ArduinoPinVoltageSource(global_slot, pin, maximum_input_voltage, invert, supports_pitch) {
            this->minimum_input_voltage = minimum_input_voltage;
        }

        ArduinoPinVoltageSource(int global_slot, byte pin, float maximum_input_voltage = 5.0, bool invert = false, bool supports_pitch = false) 
            : VoltageSourceBase(global_slot, maximum_input_voltage, supports_pitch) {
            this->pin = pin;
            this->invert = invert;
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
            
            return this->get_corrected_voltage(voltageFromAdc);
        }

        // correct for non-linearity
        float get_corrected_voltage (float voltageFromAdc) {
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