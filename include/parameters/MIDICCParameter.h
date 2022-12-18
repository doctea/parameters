#ifndef MIDICCPARAMETER__INCLUDED
#define MIDICCPARAMETER__INCLUDED

#include "parameters/Parameter.h"
//#include "midi/midi_out_wrapper.h"

#ifdef ENABLE_SCREEN
    #include "menu.h"
#endif

// interface for classes that can receive MIDI CC data
class IMIDICCTarget {
    public:
        virtual void sendControlChange(uint8_t cc_number, uint8_t value, uint8_t channel) {};
};

class IMIDIProxiedCCTarget : public IMIDICCTarget {
    public:
        virtual void sendProxiedControlChange(uint8_t cc_number, uint8_t value, uint8_t channel) {};
};

// for applying modulation to a value before sending CC values out to the target device
// eg, a synth's cutoff value
template<class TargetClass=IMIDICCTarget, class DataType=uint8_t>
class MIDICCParameter : public DataParameter<TargetClass,DataType> {
    public:
        byte cc_number = 0, channel = 0;

        MIDICCParameter(const char* label, TargetClass *target, byte cc_number, byte channel)
            : DataParameter<TargetClass,DataType>(label, target) {
                this->cc_number = cc_number;
                this->channel = channel;
                this->minimumDataValue = 0;
                this->maximumDataValue = 127;
                //this->debug = true;
        }

        MIDICCParameter(const char* label, TargetClass *target, byte cc_number, byte channel, byte maximum_value) 
            : MIDICCParameter(label, target, cc_number, channel) {
                this->maximumDataValue = maximum_value;
        }

        /*virtual const char* getFormattedValue() override {
            static char fmt[MENU_C_MAX] = "              ";
            sprintf(fmt, "%i", this->getCurrentDataValue());
            //Serial.printf("getFormattedValue: '%s'\n", fmt);
            return fmt;
        };*/

        virtual void setTargetValueFromData(DataType value, bool force = false) override {
            static byte last_value = -1;
            
            if (this->target!=nullptr) {
                if (this->debug) Serial.printf(F("MIDICCParameter#setTargetValueFromData(%i, %i, %i)\n"), cc_number, value, this->channel);
                if (last_value!=value || force)
                    this->target->sendControlChange(this->cc_number, value, this->channel);
                last_value = value;
            } else {
                if (this->debug) Serial.printf(F("WARNING: No target set in MIDICCParameter#setTargetValueFromData in '%s'!\n"), this->label);
            }
        }
};

// for parameters where we want to both accept updates from a mapping (eg a keyboard routed to the device), while also applying modulation before sending the actual value out to the target device, eg, modwheel
template<class TargetClass=IMIDIProxiedCCTarget, typename DataType=byte>
class MIDICCProxyParameter : public MIDICCParameter<TargetClass,DataType> {
    public:
        MIDICCProxyParameter(const char* label, TargetClass *target, byte cc_number, byte channel) 
            : MIDICCParameter<TargetClass,DataType>(label, target, cc_number, channel) {}

        virtual bool responds_to(byte cc_number, byte channel) {
            if (this->channel!=0 && this->channel!=channel) return false;
            if (cc_number==this->cc_number) return true;
            return false;
        }

        virtual void setTargetValueFromData(byte value, bool force = false) override {
            static byte last_value = -1;
            
            if (this->target!=nullptr) {
                if (this->debug) Serial.printf(F("MIDICCProxyParameter#setTargetValueFromData(%i, %i, %i)\n"), this->cc_number, value, this->channel);
                if (last_value!=value || force)
                    this->target->sendProxiedControlChange(this->cc_number, (byte)value, this->channel);
                last_value = value;
            } else {
                if (this->debug) Serial.printf(F("WARNING: No target set in MIDICCProxyParameter#setTargetValueFromData in '%s'!\n"), this->label);
            }
        }
};

#endif
