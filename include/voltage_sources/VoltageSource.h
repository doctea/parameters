#pragma once

#include "Arduino.h"

#include "ads.h"
#ifdef ENABLE_SCREEN
    class MenuItem;
    class Menu;
#endif

#include "icalibration.h"

#ifdef ENABLE_STORAGE
    #include "saveloadlib.h"
#endif

// base class for a voltage source, eg a wrapper around an ADC library
class VoltageSourceBase {
    bool has_pitch_capability = false;

    public:
        bool debug = false;

        float current_value = 0.0f;
        float last_value = 0.0f;

        float minimum_input_voltage = 0.0f;
        float maximum_input_voltage = 10.0f;

        int global_slot = -1;
        VoltageSourceBase(int global_slot, float minimum_input_voltage, float maximum_input_voltage, bool supports_pitch = false)
            : VoltageSourceBase(global_slot, supports_pitch) {
            this->minimum_input_voltage = minimum_input_voltage;
            this->maximum_input_voltage = maximum_input_voltage;
        }

        VoltageSourceBase(int global_slot, float maximum_input_voltage = 5.0, bool supports_pitch = false) 
            : VoltageSourceBase(global_slot, supports_pitch) {
            this->maximum_input_voltage = maximum_input_voltage;
        }

        VoltageSourceBase(int global_slot, bool supports_pitch = false) {
            this->global_slot = global_slot;
            this->has_pitch_capability = supports_pitch;
        }


        // actually fetch the current value from ADC, put it in the current_value
        virtual float fetch_current_voltage() = 0;
        // update the current voltage values

        // read a value from ADC to 'prime' it, but discard it 
        virtual void discard_update() {
            this->fetch_current_voltage();
        }

        virtual void update() {
            //last_value = ads_source->readADC(channel);
            //if (this->debug) Serial.printf("VoltageSource@%p#update() about to fetch_current_voltage()\n", this);
            this->last_value = this->current_value;
            this->current_value = this->fetch_current_voltage();
        }

        // returns the last read raw voltage value
        virtual float get_voltage() {
            return this->current_value;
        }

        // Returns voltage normalised by maximum_input_voltage.
        // For unipolar sources (0..max): returns 0.0 to 1.0.
        // For bipolar sources (-max..+max, minimum_input_voltage == 0): returns -1.0 to +1.0.
        // This is intentional: maximum_input_voltage is the full-scale reference, not a
        // one-sided bound.  Downstream code (VoltageParameterInput, BIPOLAR constrain) relies
        // on this convention.  Do NOT change to a (v-min)/(max-min) range mapping here.
        virtual float get_voltage_normal() {
            if (this->maximum_input_voltage == 0.0f) return 0.0f;
            return this->get_voltage() / this->maximum_input_voltage;
        }

        virtual bool supports_pitch() {
            return this->has_pitch_capability;
        }
        virtual uint8_t get_voltage_pitch() {
            return get_midi_pitch_for_voltage(this->get_voltage());
        }

        virtual bool needs_calibration() {
            return false;
        }
        virtual void output_calibration_data() {
            Serial.printf("VoltageSourceBase calibration data for slot %i: no calibration data\n", global_slot);
        }

        #ifdef ENABLE_STORAGE
            virtual ISaveableSettingHost* as_saveable_host() { return nullptr; }
        #endif

        // Returns this cast to ADSVoltageSourceBase* if this is an ADS-family source, else nullptr.
        // Avoids dynamic_cast when RTTI is disabled.
        virtual class ADSVoltageSourceBase* as_ads_source() { return nullptr; }

        #ifdef ENABLE_SCREEN
            //FLASHMEM 
            virtual MenuItem *makeCalibrationControls(int i) {
                Serial.println(F("makeCalibrationControls() in VoltageSourceBase returning nullptr"));
                return nullptr;
            }
        #endif

};
