#pragma once

#include "Arduino.h"

#include "ads.h"
#ifdef ENABLE_SCREEN
    class MenuItem;
    class Menu;
#endif

#include "icalibration.h"

// base class for a voltage source, eg a wrapper around an ADC library
class VoltageSourceBase {
    bool has_pitch_capability = false;

    public:
        bool debug = false;

        float current_value = 0.0f;
        float last_value = 0.0f;

        float maximum_input_voltage;

        int global_slot = -1;
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

        // return a normalised version of the last value (ie 0.0-1.0)
        virtual float get_voltage_normal() {
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

        #if defined(ENABLE_CALIBRATION_STORAGE)
            virtual void load_calibration() {
                Serial.printf("VoltageSourceBase: empty load_calibration for unknown input!\n");
                // todo: make VoltageSource know its name so that it knows where to load from
            }
            virtual void save_calibration() {
                Serial.printf("VoltageSourceBase: empty load_calibration for unknown input!\n");
                // todo: make VoltageSource know its name so that it knows where to save to
            }
        #endif

        #ifdef ENABLE_SCREEN
            //FLASHMEM 
            virtual MenuItem *makeCalibrationControls(int i) {
                Serial.println(F("makeCalibrationControls() in VoltageSourceBase returning nullptr"));
                return nullptr;
            }
            #if defined(ENABLE_CALIBRATION_STORAGE)
                FLASHMEM virtual MenuItem *makeCalibrationLoadSaveControls(int i);
            #endif
        #endif

};
