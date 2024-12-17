#pragma once

#include "parameters/Parameter.h"
//#include "midi/midi_out_wrapper.h"

#include "midi_helpers.h"

#ifdef ENABLE_SCREEN
    #include "menu.h"
    #include "submenuitem_bar.h"
#endif

// for applying modulation to a value before sending CC values out to the target device
// eg, a synth's cutoff value
template<class TargetClass=IMIDICCTarget, class DataType=uint8_t>
class MIDICCParameter : public DataParameter<TargetClass,DataType> {
    public:
        byte cc_number = MIDI_MIN_CC, channel = MIDI_CHANNEL_OMNI;

        bool configurable = false;

        MIDICCParameter(const char* label, TargetClass *target, byte cc_number, byte channel, bool configurable = false)
            : DataParameter<TargetClass,DataType>(label, target) {
                this->cc_number = cc_number;
                this->channel = channel;
                this->minimumDataLimit = this->minimumDataRange = MIDI_MIN_CC;
                this->maximumDataLimit = this->maximumDataRange = MIDI_MAX_CC;
                this->configurable = configurable;
                //this->debug = true;
        }

        MIDICCParameter(const char* label, TargetClass *target, byte cc_number, byte channel, byte maximumDataValue, bool configurable = false) 
            : MIDICCParameter(label, target, cc_number, channel, configurable) {
                this->maximumDataLimit = maximumDataValue;
        }

        /*virtual const char* getFormattedValue() override {
            static char fmt[MENU_C_MAX] = "              ";
            sprintf(fmt, "%i", this->getCurrentDataValue());
            //Serial.printf("getFormattedValue: '%s'\n", fmt);
            return fmt;
        };*/

        DataType last_value = -1;
        virtual void setTargetValueFromData(DataType value, bool force = false) override {
            if (this->target!=nullptr) {
                if (this->debug && Serial) 
                    Serial.printf("MIDICCParameter#setTargetValueFromData(%i, %i, %i)\n", cc_number, value, this->channel);

                if (last_value!=value || force)
                    this->target->sendControlChange(this->cc_number, value, this->channel);

                last_value = value;
            } else {
                if (this->debug) Serial.printf("WARNING: No target set in MIDICCParameter#setTargetValueFromData in '%s'!\n", this->label);
            }
        }

        virtual bool load_parse_key_value(String key, String value) override {
            if (this->configurable) {
                Serial.printf("MIDICCParameter named '%s' asked to load_parse_key_value with '%s' => '%s'..", this->label, key.c_str(), value.c_str());
                String prefix = String("parameter_value_") + String(this->label) + String("_");
                Serial.printf("Checking if starts with '%s'..", prefix.c_str());
                if (key.startsWith(prefix)) {
                    Serial.println("YES! now checking if endswith '_channel' or '_cc'..");
                    if (key.endsWith("_channel")) {
                        Serial.println("YES! Ends with _channel");
                        this->channel = value.toInt();
                        return true;
                    } else if (key.endsWith("_cc")) {
                        Serial.println("YES! Ends with _cc");
                        this->cc_number = value.toInt();
                        return true;
                    }
                }
                Serial.println("no match.");
            }
            return FloatParameter::load_parse_key_value(key, value);
        }
        virtual void save_pattern_add_lines(LinkedList<String> *lines) override {
            if (this->configurable) {
                String prefix = String("parameter_value_") + String(this->label); //+ String("_");
                lines->add(prefix + String("_channel=") + String(this->channel));
                lines->add(prefix + String("_cc=") + String(this->cc_number));
            }
            FloatParameter::save_pattern_add_lines(lines);
        }

        #ifdef USE_SCREEN
        FLASHMEM
        virtual LinkedList<MenuItem *> *addCustomTypeControls(LinkedList<MenuItem *> *controls) override { 
            if (this->configurable) {
                SubMenuItem *bar = new SubMenuItemBar("Settings", true, false);

                bar->add(new DirectNumberControl<byte>(
                    "Output CC", //label, 
                    &this->cc_number,
                    this->cc_number,
                    MIDI_MIN_VELOCITY,
                    MIDI_MAX_VELOCITY
                ));

                bar->add(new DirectNumberControl<byte>(
                    "Output MIDI Channel", 
                    &this->channel,
                    this->channel,
                    MIDI_MIN_CHANNEL,
                    MIDI_MAX_CHANNEL
                ));

                // insert controls at the top
                controls->add(bar);
            }
            return controls; 
        };
        #endif
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

        DataType proxied_last_value = -1;
        virtual void setTargetValueFromData(byte value, bool force = false) override {           
            if (this->target!=nullptr) {
                if (this->debug) 
                    Serial.printf("MIDICCProxyParameter#setTargetValueFromData(%i, %i, %i)\n", this->cc_number, value, this->channel);

                if (proxied_last_value!=value || force)
                    this->target->sendProxiedControlChange(this->cc_number, (byte)value, this->channel);

                proxied_last_value = value;
            } else {
                if (this->debug) Serial.printf("WARNING: No target set in MIDICCProxyParameter#setTargetValueFromData in '%s'!\n", this->label);
            }
        }
};

