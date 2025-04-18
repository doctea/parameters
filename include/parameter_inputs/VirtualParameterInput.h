#pragma once

#include "ParameterInput.h"
#include "AnalogParameterInputBase.h"

#include "../voltage_sources/VoltageSource.h"

#include "bpm.h"
#include "midi_helpers.h"

#include "ads.h"

// todo: (more) options to configure LFO type, speed, etc
// todo: support MIDI pitch generation

// see ParameterInput.cpp for the string def
enum lfo_option_id {
    LFO_FREE,
    LFO_LOCKED,
    RAND,
    NUM
};

struct lfo_option_t {
    const char *name;
    lfo_option_id id;
};

extern lfo_option_t virtual_parameter_options[lfo_option_id::NUM];
#define MAX_LFO_ID (sizeof(virtual_parameter_options) / sizeof(Lfo_option_t))

class VirtualParameterInput : public AnalogParameterInputBase<float> {
    public:
        lfo_option_id lfo_mode = LFO_LOCKED;

        // wave parameters
        float free_sine_divisor = 100.0f;
        float locked_period = 4.0f;
        float locked_phase = 0.0f;

        // track sample & hold state
        uint32_t sh_ticks = 0;
        float last_sample = 0;
        uint32_t last_sample_tick = 0;

        VirtualParameterInput(char *name, const char *group_name, lfo_option_id lfo_mode = LFO_LOCKED) : AnalogParameterInputBase(name, group_name) {
            this->lfo_mode = lfo_mode;
        }

        virtual bool hasExtra() override {
            return this->supports_pitch();
        }
        virtual const char *getExtra() override {
            /*if (this->voltage_source==nullptr) {
                return "[null voltage_source]";
            }*/
            if (this->supports_pitch()) {
                static char extra_output[40];
                snprintf(
                    extra_output, 
                    40,
                    "MIDI pitch for %3.3f is %s\n", 
                    this->get_source_value() * 10.0,
                    get_note_name(get_voltage_pitch()).c_str()
                );
                return extra_output;
            }
            return virtual_parameter_options[lfo_mode].name;
            //return "";
        }

        virtual bool supports_pitch() override {
            return false;
            //return this->voltage_source->supports_pitch();
        }
        virtual uint8_t get_voltage_pitch() {
            //return get_midi_pitch_for_voltage(this->voltage_source->get_voltage_pitch());
            //if (this->voltage_source==nullptr) 
            //  Debug_printf(F("%c#get_voltage_pitch() has no voltage_source?!"), this->name); Serial_flush();
            //return this->voltage_source->get_voltage_pitch();
            //return this->current_voltage_pitch;
            return 64;
        }

        float calculate_lfo(float normal) {
            return input_type==BIPOLAR ? 
                sin(normal*2.0f*PI) :
                0.5f + ((sin(normal*2.0f*PI))/2.0);
        }

        float get_source_value() {
            if (last_sample_tick!=ticks && (sh_ticks==0 || (sh_ticks>0 && (ticks % sh_ticks == 0 || ticks >= last_sample_tick+sh_ticks)))) {
                last_sample_tick = ticks;

                switch (lfo_mode) {
                    case LFO_FREE: 
                        last_sample = calculate_lfo(locked_phase + (float)ticks/free_sine_divisor);
                        break;
                    case LFO_LOCKED: {
                        float adjusted = ((float)TICKS_PER_BAR*locked_period);
                        last_sample = calculate_lfo(locked_phase + ((float)(ticks % (int_fast16_t)(adjusted)))/(float)(adjusted));
                        break;
                    }
                    case RAND: 
                        last_sample = input_type==BIPOLAR ? 
                            (float)random(-1000, 1000)/1000.0 : 
                            (float)random(0, 1000)/1000.0;
                        break;;
                    default: return 0.0f;
                }
            }
            return last_sample;
        }

        virtual void read() override {
            //float currentValue = this->voltage_source->get_voltage_normal();
            float currentValue = this->get_source_value();
            
            if (this->is_significant_change(currentValue, this->lastValue)) {
                this->lastValue = this->currentValue;
                this->currentValue = currentValue;
                //Serial.printf("%c: Setting this->currentValue to ", this->name);
                //Serial.println(currentValue);
                //this->currentValue = currentValue;
                //this->currentValue = currentValue = this->get_normal_value(currentValue);
                #ifdef ENABLE_PRINTF
                if (this->debug) {
                    /*Serial.printf("%s: VoltageParameterInput->read() got intermediate %i, voltage ", this->name, intermediate);
                    Serial.print((uint32_t) this->ads->toVoltage(intermediate));
                    Serial.printf(", final %i", (uint32_t) currentValue*1000.0);
                    Serial.println();*/
                    Debug_printf(F("%c: VoltageParameterInput->read() got voltage "), this->name); //
                    Debug_println(currentValue);
                }
                #endif

                #ifdef ENABLE_PRINTF
                if (this->debug) {
                    Debug_printf(F("VoltageParameterInput#read() for '%c': got currentValue "), this->name); Serial_flush();
                    Debug_print(currentValue); Serial_flush();
                    Debug_print(F(" converted to normal ")); Serial_flush();
                    Debug_println(normal); Serial_flush();
                }
                #endif

                #ifdef PARAMETER_INPUTS_USE_CALLBACKS
                    float normal = this->get_normal_value_unipolar(currentValue);
                    this->on_value_read(normal);
                    if (this->callback != nullptr) {
                        if (this->debug) {
                            Debug_print(this->name);
                            Debug_print(F(": calling callback("));
                            Debug_print(normal);
                            Debug_println(F(")"));
                            Serial_flush();
                        }      
                        (*this->callback)(normal);
                    }
                #endif

                /*if (this->target_parameter!=nullptr) {
                    if (this->debug) {
                        Serial.println("Calling on target_parameter.."); Serial_flush();
                        Serial.print(this->name); Serial_flush();
                        Serial.print(F(": calling target from normal setParamValue(")); Serial_flush();
                        Serial.print(normal); Serial_flush();
                        Serial.print(F(")")); Serial_flush();
                        Serial.print(" from currentValue "); Serial_flush();
                        Serial.print(currentValue); Serial_flush();
                        if (this->inverted) { Serial.print(F(" - inverted")); Serial_flush(); }
                        Serial.println(); Serial_flush();

                        #ifdef ENABLE_PRINTF
                            Serial.printf("VoltageParameterInput %c calling setParamValue with maximum_input_voltage ", this->name);
                            Serial.println(this->voltage_source->maximum_input_voltage);
                        #endif
                    }
                    //this->target_parameter->setParamValue(normal, this->voltage_source->maximum_input_voltage);
                    this->target_parameter->updateValueFromNormal(normal); //
                }*/
                //Serial.println("Finishing read()"); Serial_flush();
            }
        }

        const String string__equals = String('=');
        virtual LinkedList<String> *save_pattern_add_lines(LinkedList<String> *lines) override {
            AnalogParameterInputBase::save_pattern_add_lines(lines);

            const String string__prefix_and_name = String(prefix) + String(this->name);
            
            lines->add(string__prefix_and_name + String("_period")  + string__equals + String(this->locked_period));
            lines->add(string__prefix_and_name + String("_phase")   + string__equals + String(this->locked_phase));
            lines->add(string__prefix_and_name + String("_divisor") + string__equals + String(this->free_sine_divisor));
            lines->add(string__prefix_and_name + String("_sh_ticks")+ string__equals + String(this->sh_ticks));

            return lines;
        }

        virtual bool load_parse_key_value(String key, String value) {
            if(!key.startsWith(prefix)) return false;

            key.replace(prefix,"");

            // todo: check if this is sane way to make sure that we're loading for the correct item?
            if (!key.startsWith(this->name)) {
                return AnalogParameterInputBase::load_parse_key_value(key, value);
            }

            if (key.endsWith("_period")) {
                this->locked_period = value.toFloat();
                return true;
            } else if (key.endsWith("_phase")) {
                this->locked_phase = value.toFloat();
                return true;
            } else if (key.endsWith("_divisor")) {
                this->free_sine_divisor = value.toFloat();
                return true;
            } else if (key.endsWith("_sh_ticks")) {
                this->sh_ticks = value.toInt();
                return true;
            }

            return false;
        }

        #ifdef ENABLE_SCREEN
            FLASHMEM
            virtual SubMenuItemBar *makeControls(int16_t memory_size, const char *label_prefix = "") override;
        #endif

};