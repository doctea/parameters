#pragma once

#include <Arduino.h>

#include "ParameterInput.h"

// base MIDI receiver class, subclassed for CC and Pitchbend
class MIDIParameterInputBase : public ParameterInput {
    public:

    MIDIParameterInputBase(char *name, const char *group_name) : ParameterInput(name, group_name) {
        this->input_type = UNIPOLAR;
    }

    virtual bool responds_to_cc(byte cc_number, byte channel) { return false; } // default to false, override in subclasses
    virtual bool responds_to_pitch_bend(byte channel) { return false; } // default to false, override in subclasses
    virtual void receive_control_change(byte cc_number, byte value, byte channel) {};   // override in subclass
    virtual void receive_pitch_bend(byte lsb, byte msb, byte channel) {};   // override in subclass
    virtual void receive_pitch_bend(int16_t value, byte channel) {};        // override in subclass

};


// for eg listening to a slider CC events
class MIDICCParameterInput : public MIDIParameterInputBase {
    public:
        int8_t cc_number = 0;     // cc number to listen to 
        int8_t channel = 0;       // channel to listen to 

        int8_t currentValue = 0;

        MIDICCParameterInput(char *name, const char *group_name, byte cc_number, byte channel = 0) 
                : MIDIParameterInputBase(name, group_name) {
            this->cc_number = cc_number;
            //this->value = value;
            this->channel = channel;
            this->input_type = UNIPOLAR;
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
            snprintf(fmt, 20, "[%-3i%%]", (int)(this->get_normal_value_unipolar()*100.0));
            return fmt;
        }

        // midi ccs are always unipolar 0-127
        virtual bool supports_bipolar_input() override {
            return false;
        }

        virtual void loop() override {
            // do nothing, since should be informed of events by a sender
        }
        virtual void read() override {
            // do nothing, since should be informed of events by a sender
        }

        // test whether this parameter input responds to a given cc number and channel
        // if the parameter input's channel is 0, it responds to all channels; otherwise it only responds to the specified channel
        virtual bool responds_to_cc(byte cc_number, byte channel) override {
            if (this->channel!=0 && this->channel!=channel) return false;
            if (cc_number==this->cc_number) return true;
            return false;
        }

        virtual void receive_control_change(byte cc_number, byte value, byte channel) override {
            if (this->cc_number==cc_number && (channel==0 || this->channel==channel || this->channel==0))
                this->currentValue = value;
        }

        virtual float get_normal_value_unipolar() override {
            return (float)this->currentValue/127.0;
        }
        virtual float get_normal_value_bipolar() override {
            float bipolar = ((float)this->currentValue - 64.0f) / 63.0f;
            return constrain(bipolar, -1.0f, 1.0f);
        }

        virtual const char *getInputInfo() {
            static char input_info[20] = "                ";

            snprintf(input_info, 20, "CC%3i Chan%2i", this->cc_number, this->channel);
            return input_info;
        }


        #ifdef ENABLE_STORAGE
            virtual void setup_saveable_settings() override {
                // inherit parent's settings
                ParameterInput::setup_saveable_settings();

                register_setting(
                    new VarSetting<int8_t>(
                        "CC Number",
                        "MIDIParameterInput",
                        &this->cc_number
                    ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT
                );
                register_setting(
                    new VarSetting<int8_t>(
                        "Channel",
                        "MIDIParameterInput",
                        &this->channel
                    ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT
                );
            }
        #endif

};


// basically the same as a MIDIParameterInput, but for pitch bend events, which are 14-bit values (0-16383) instead of 7-bit (0-127)
// TODO: add a way to specify the pitch bend range (in semitones) for this input, so that it can be converted to a bipolar value in volts/octave or Hz
class MIDIPitchBendParameterInput : public MIDIParameterInputBase {
    public:
        int8_t channel = 0;       // channel to listen to; 0 == omni
        int16_t currentValue = 8192; // pitch bend is 14-bit, so default to centre value

        MIDIPitchBendParameterInput(char *name, const char *group_name, byte channel = 0) 
                : MIDIParameterInputBase(name, group_name) {
            this->channel = channel;
            this->input_type = BIPOLAR;
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
            snprintf(fmt, 20, "[%-3i%%]", (int)(this->get_normal_value_unipolar()*100.0));
            return fmt;
        }

        virtual bool supports_bipolar_input() override {
            return true;
        }

        virtual void loop() override {
            // do nothing, since should be informed of events by a sender
        }
        virtual void read() override {
            // do nothing, since should be informed of events by a sender
        }

        virtual bool responds_to_pitch_bend(byte channel) override {
            if (this->channel!=0 && this->channel!=channel) return false;
            return true;
        }

        virtual void receive_pitch_bend(byte lsb, byte msb, byte channel) override {
            if (this->channel==channel || this->channel==0)
                this->currentValue = (msb << 7) | lsb;
        }

        virtual void receive_pitch_bend(int16_t value, byte channel) override {
            if (this->channel==channel || this->channel==0)
                this->currentValue = value;
            // Serial.printf("MIDIPitchBendParameterInput::receive_pitch_bend: value=%i, channel=%i, currentValue=%i\n", value, channel, this->currentValue);
        }

        virtual float get_normal_value_unipolar() override {
            return (float)(8192 + this->currentValue) / 16383.0;
        }
        virtual float get_normal_value_bipolar() override {
            float bipolar = ((float)this->currentValue) / 8191.0f;
            return constrain(bipolar, -1.0f, 1.0f);
        }
};
