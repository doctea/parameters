// ParameterInput wrapper around midihelper library Envelopes, so can use the output of an envelope as a parameter input.

#pragma once

#ifdef ENABLE_ENVELOPES
#ifdef ENABLE_ENVELOPES_AS_PARAMETER_INPUTS

#include "ParameterInput.h"
#include "AnalogParameterInputBase.h"

#include "../voltage_sources/VoltageSource.h"

#include "bpm.h"
#include "midi_helpers.h"

#include "ads.h"

class EnvelopeParameterInput : public virtual AnalogParameterInputBase<float> {
    public:
        EnvelopeBase *envelope;

        EnvelopeParameterInput(char *name, const char *group_name, EnvelopeBase *envelope) 
            : AnalogParameterInputBase(name, group_name, 0.005, UNIPOLAR) {
            this->envelope = envelope;
        }

        virtual void read() override {
            if (this->envelope==nullptr) return;
            this->currentValue = this->envelope->get_envelope_level();
        }

        virtual void loop() override {
            this->read();
        }

        virtual const char *getInputInfo() {
            return "Envelope";
        }

        /*virtual LinkedList<String> *save_pattern_add_lines(LinkedList<String> *lines) override {
            AnalogParameterInputBase::save_pattern_add_lines(lines);

            lines->add(String(this->name) + String(".envelope=") + String(this->envelope->name));

            return lines;
        }

        virtual bool load_parse_key_value(String key, String value) override {
            if(!key.startsWith(this->name)) return false;

            key.replace(this->name,"");

            if (key.equals(".envelope")) {
                this->envelope = nullptr;
                return true;
            }

            return false;
        }*/

        /*#ifdef ENABLE_SCREEN
        virtual SubMenuItemBar *makeControls(int16_t memory_size, const char *label_prefix = "") override {
            return nullptr;
        }
        #endif*/
};

#endif
#endif