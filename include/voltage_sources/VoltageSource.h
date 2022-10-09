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

        int slot = -1;
        VoltageSourceBase(int slot) {
            this->slot = slot;
        }

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

        virtual void load_calibration() {
            Serial.printf("VoltageSourceBase: load_calibration for unknown input!");
            // todo: make VoltageSource know its name so that it knows where to load from
        }
        virtual void save_calibration() {
            Serial.printf("VoltageSourceBase: load_calibration for unknown input!");
            // todo: make VoltageSource know its name so that it knows where to save to
        }

        #ifdef ENABLE_SCREEN
            FLASHMEM virtual MenuItem *makeCalibrationControls(int i) {
                Serial.println("makeCalibrationControls() in VoltageSourceBase returning nullptr");
                return nullptr;
            }
            FLASHMEM virtual MenuItem *makeCalibrationLoadSaveControls(int i);
        #endif

};
#endif