#ifndef MIDIPARAMETERINPUT__INCLUDED
#define MIDIPARAMETERINPUT__INCLUDED

#include <Arduino.h>

#include "ParameterInput.h"

// for eg listening to a slider CC events
class MIDIParameterInput : public ParameterInput {
    public:
        int8_t cc_number = 0;     // cc number to listen to 
        int8_t channel = 0;       // channel to listen to 

        int8_t currentValue = 0;

        MIDIParameterInput(char *name, byte cc_number, byte channel = 0) : ParameterInput(name) {
            this->cc_number = cc_number;
            //this->value = value;
            this->channel = channel;
            this->input_type = UNIPOLAR;
        }

        // midi ccs are always unipolar 0-127
        virtual bool supports_bipolar() override {
            return false;
        }

        virtual void loop() override {
            // do nothing, since should be informed of events by a sender
        }
        virtual void read() override {
            // do nothing, since should be informed of events by a sender
        }

        virtual bool responds_to(byte cc_number, byte channel) {
            if (this->channel!=0 && this->channel!=channel) return false;
            if (cc_number==this->cc_number) return true;
            return false;
        }

        virtual void receive_control_change(byte cc_number, byte value, byte channel) {
            if (this->cc_number==cc_number && (channel==0 || this->channel==channel || this->channel==0))
                this->currentValue = value;
        }

        virtual float get_normal_value() override {
            return (float)this->currentValue/127.0;
        }

        virtual const char *getInputInfo() {
            static char input_info[20] = "                ";

            snprintf(input_info, 20, "CC%3i Chan%2i", this->cc_number, this->channel);
            return input_info;
        }
        virtual const char *getInputValue() override {
            static char fmt[20] = "          ";
            //sprintf(fmt, "[%-3i%%]", (int)(this->currentValue*100.0));
            snprintf(fmt, 20, "[%3i]", this->currentValue);
            return fmt;
        }
        // for some reason, this prevents boot if uncommented?!
        virtual const char *getOutputValue() override {
            static char fmt[20] = "          ";
            //sprintf(fmt, "[%-3i%%]", (int)(this->get_normal_value((float)this->currentValue)*100.0));
            snprintf(fmt, 20, "[%-3i%%]", (int)(this->get_normal_value()*100.0));
            return fmt;
        }

};

#endif