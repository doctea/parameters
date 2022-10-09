#ifndef VOLTAGE_SOURCE__INCLUDED
#define VOLTAGE_SOURCE__INCLUDED

#include "Arduino.h"

#include "ads.h"
#ifdef ENABLE_SCREEN
    class MenuItem;
    class Menu;
#endif

// base class for a voltage source, eg a wrapper around an ADC library
class VoltageSourceBase {
    public:
        bool debug = false;

        double current_value = 0.0;
        double last_value = 0.0;

        double maximum_input_voltage;

        // actually fetch the current value from ADC, put it in the current_value
        virtual double fetch_current_voltage();
        // update the current voltage values

        virtual void update() {
            //last_value = ads_source->readADC(channel);
            //if (this->debug) Serial.printf("VoltageSource@%p#update() about to fetch_current_voltage()\n", this);
            this->last_value = this->current_value;
            this->current_value = this->fetch_current_voltage();
        }

        // returns the last read raw voltage value
        virtual double get_voltage() {
            return this->current_value;
        }

        // return a normalised version of the last value (ie 0.0-1.0)
        virtual double get_voltage_normal() {
            return this->get_voltage() / this->maximum_input_voltage;
        }

        virtual uint8_t get_voltage_pitch() {
            return get_midi_pitch_for_voltage(this->get_voltage());
        }

        #ifdef ENABLE_SCREEN
            virtual MenuItem *makeCalibrationControls(int i) {
                return nullptr;
            }
            virtual MenuItem *makeCalibrationLoadSaveControls(int i) {
                return nullptr;
            }
        #endif
};
#endif