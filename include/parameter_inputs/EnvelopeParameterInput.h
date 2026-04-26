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

            #ifdef PARAMETER_INPUTS_USE_CALLBACKS
                float normal = this->get_normal_value(currentValue, UNIPOLAR);
                this->on_value_read(normal);
                if (this->callback != nullptr) {
                    if (this->debug) {
                        Debug_print(this->name);
                        Debug_print(": calling callback(");
                        Debug_print(normal);
                        Debug_println(")");
                        Serial_flush();
                    }      
                    (*this->callback)(normal);
                }
            #endif
        }

        virtual void loop() override {
            this->read();
        }

        virtual const char *getInputInfo() {
            return "Envelope";
        }
};

#endif
#endif