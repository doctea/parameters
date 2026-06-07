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

#ifndef VOLTAGE_SOURCE_DEFAULT_SMOOTHING 
    #define VOLTAGE_SOURCE_DEFAULT_SMOOTHING 1.0f
    /*
    How to think about alpha:
        alpha = 1.0 — off, instant response (current behaviour)
        alpha = 0.5 — each new reading contributes 50%; settles to ~97% of a step change in ~5 updates
        alpha = 0.2 — more noise reduction; settles in ~18 updates
        alpha = 0.1 — heavy smoothing; settles in ~40 updates
    */
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

        // Exponential moving average smoothing applied in update().
        // alpha = 1.0 → no smoothing (default, instant response).
        // alpha = 0.1 → heavy smoothing (slow to respond).
        // Formula: current_value = alpha * new_sample + (1 - alpha) * current_value
        float smooth_alpha = VOLTAGE_SOURCE_DEFAULT_SMOOTHING;

        int global_slot = -1;
        VoltageSourceBase(int global_slot, float minimum_input_voltage, float maximum_input_voltage, bool supports_pitch = false, float smooth_alpha = VOLTAGE_SOURCE_DEFAULT_SMOOTHING) {
            this->global_slot = global_slot;
            this->minimum_input_voltage = minimum_input_voltage;
            this->maximum_input_voltage = maximum_input_voltage;
            this->has_pitch_capability = supports_pitch;
            this->smooth_alpha = smooth_alpha;
        }

        VoltageSourceBase(int global_slot, float maximum_input_voltage = 5.0, bool supports_pitch = false, float smooth_alpha = VOLTAGE_SOURCE_DEFAULT_SMOOTHING) 
            : VoltageSourceBase(global_slot, 0.0f, maximum_input_voltage, supports_pitch, smooth_alpha) {
            this->maximum_input_voltage = maximum_input_voltage;
        }

        VoltageSourceBase(int global_slot, bool supports_pitch = false, float smooth_alpha = VOLTAGE_SOURCE_DEFAULT_SMOOTHING) {
            this->global_slot = global_slot;
            this->has_pitch_capability = supports_pitch;
            this->smooth_alpha = smooth_alpha;
        }


        // actually fetch the current value from ADC, put it in the current_value
        virtual float fetch_current_voltage() = 0;
        // update the current voltage values

        // read a value from ADC to 'prime' it, but discard it 
        virtual void discard_update() {
            this->fetch_current_voltage();
        }

        virtual void update() {
            this->last_value = this->current_value;
            const float raw = this->fetch_current_voltage();
            if (this->smooth_alpha >= 1.0f) {
                this->current_value = raw;
            } else {
                this->current_value = this->smooth_alpha * raw
                                    + (1.0f - this->smooth_alpha) * this->current_value;
            }
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
