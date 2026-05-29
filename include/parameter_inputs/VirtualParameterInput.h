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
    LFO_LOCKED_TRIANGLE,    // triangle wave — cheaper than sine, no sin() call
    LFO_LOCKED_SAW,         // rising ramp 0→1
    LFO_LOCKED_RSAW,        // falling ramp 1→0
    LFO_LOCKED_SQUARE,      // square wave — hard gate/ducking
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
        // todo: move this into parent class, and make it available to VoltageParameterInput etc? could be useful for eg sample&hold on CV inputs
        uint32_t sh_ticks = 0;
        float last_sample = 0;
        uint32_t last_sample_tick = 0;

        // Phase accumulator (NCO): advances per-tick so that changing locked_period
        // or free_sine_divisor mid-run causes no phase jump — the step size changes
        // from the next advance onward, preserving phase continuity.
        float phase_acc = 0.0f;
        uint32_t last_advanced_tick = 0;

        // When true, makeControls() skips creating a ParameterInputDisplay and
        // UI controls for this instance (~500 bytes saved).  The input is still
        // fully functional as a modulation source.
        bool lightweight = false;

        VirtualParameterInput(char *name, const char *group_name,
                              lfo_option_id lfo_mode = LFO_LOCKED,
                              float locked_period = 4.0f, float locked_phase = 0.0f,
                              uint32_t sh_ticks = 0, bool lightweight = false)
                : AnalogParameterInputBase(name, group_name) {
            this->lfo_mode     = lfo_mode;
            this->locked_period = locked_period;
            this->locked_phase  = locked_phase;
            this->sh_ticks      = sh_ticks;
            this->lightweight   = lightweight;
        }

        // virtual bool supports_pitch() override {
        //     return false;
        //     //return this->voltage_source->supports_pitch();
        // }
        // virtual int8_t get_voltage_pitch() override {
        //     //return get_midi_pitch_for_voltage(this->voltage_source->get_voltage_pitch());
        //     //if (this->voltage_source==nullptr) 
        //     //  Debug_printf(F("%c#get_voltage_pitch() has no voltage_source?!"), this->name); Serial_flush();
        //     //return this->voltage_source->get_voltage_pitch();
        //     //return this->current_voltage_pitch;
        //     return 64;
        // }

        float calculate_lfo(float normal) {
            return input_type==BIPOLAR ? 
                sin(normal*2.0f*PI) :
                0.5f + ((sin(normal*2.0f*PI))/2.0);
        }

        float get_source_value() {
            // Advance phase accumulator to current tick (catch-up).
            // All LFO modes share this accumulator; RAND does not use it.
            // Because loop() runs far faster than tick rate, elapsed is almost always 1,
            // but the catch-up handles cases where this is called less frequently.
            if (lfo_mode != RAND) {
                const float step = (lfo_mode == LFO_FREE)
                    ? (1.0f / free_sine_divisor)
                    : (1.0f / ((float)TICKS_PER_BAR * locked_period));
                if (last_advanced_tick != ticks) {
                    if (last_advanced_tick > ticks) {
                        // Clock reset: resync accumulator to absolute position
                        phase_acc = fmodf((float)ticks * step, 1.0f);
                    } else {
                        phase_acc = fmodf(phase_acc + (float)(ticks - last_advanced_tick) * step, 1.0f);
                    }
                    last_advanced_tick = ticks;
                }
            }

            // Update sample output, gated by S&H if configured
            if (last_sample_tick != ticks &&
                    (sh_ticks == 0 || (ticks % sh_ticks == 0) || ticks >= last_sample_tick + sh_ticks)) {
                last_sample_tick = ticks;

                // locked_phase is an absolute offset applied at read time;
                // changing it causes an intentional immediate phase shift.
                const float frac = fmodf(phase_acc + locked_phase, 1.0f);

                switch (lfo_mode) {
                    case LFO_FREE:
                    case LFO_LOCKED:
                        last_sample = calculate_lfo(frac);
                        break;
                    case RAND:
                        last_sample = input_type == BIPOLAR
                            ? (float)random(-1000, 1000) / 1000.0f
                            : (float)random(0, 1000) / 1000.0f;
                        break;
                    case LFO_LOCKED_TRIANGLE: {
                        const float v = 1.0f - 2.0f * fabsf(frac - 0.5f);  // 0 at edges, 1 at mid
                        last_sample = (input_type == BIPOLAR) ? (v * 2.0f - 1.0f) : v;
                        break;
                    }
                    case LFO_LOCKED_SAW:
                        last_sample = (input_type == BIPOLAR) ? (frac * 2.0f - 1.0f) : frac;
                        break;
                    case LFO_LOCKED_RSAW: {
                        const float v = 1.0f - frac;
                        last_sample = (input_type == BIPOLAR) ? (v * 2.0f - 1.0f) : v;
                        break;
                    }
                    case LFO_LOCKED_SQUARE: {
                        const float v = (frac < 0.5f) ? 1.0f : 0.0f;
                        last_sample = (input_type == BIPOLAR) ? (v * 2.0f - 1.0f) : v;
                        break;
                    }
                    default: return 0.0f;
                }
            }
            return last_sample;
        }

        // Route get_normal_value_*() through get_source_value() (live, tick-cached)
        // rather than the stale currentValue updated asynchronously by read() in loop().
        virtual float get_live_value() override {
            return get_source_value();
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
                    float normal = this->get_normal_value(currentValue, UNIPOLAR);
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

        #ifdef ENABLE_STORAGE
            virtual void setup_saveable_settings() override {
                AnalogParameterInputBase::setup_saveable_settings();

                // @@TODO: maybe figure out how/if we can set this up on the fly -- difficulty will be in updating the 
                // UI controls dynamically when these settings change, but would be nice
                // register_setting(
                //     new LSaveableSetting<lfo_option_id>(
                //         "LFO Mode",
                //         "VirtualParameterInput",
                //         &this->lfo_mode,
                //         [=](lfo_option_id value) -> void {
                //             this->lfo_mode = value;
                //         },
                //         [=](void) -> lfo_option_id {
                //             return this->lfo_mode;
                //         }
                //     )
                // );

                register_setting(new VarSetting<float>(
                        "Locked Period",
                        "VirtualParameterInput",
                        &this->locked_period
                    ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);
                register_setting(new VarSetting<float>(
                        "Locked Phase",
                        "VirtualParameterInput",
                        &this->locked_phase
                    ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);
                register_setting(new VarSetting<float>(
                        "Free Sine Divisor",
                        "VirtualParameterInput",
                        &this->free_sine_divisor
                    ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);
                register_setting(new VarSetting<uint32_t>(
                        "Sample&Hold Ticks",
                        "VirtualParameterInput",
                        &this->sh_ticks
                    ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);
            }
        #endif

        #ifdef ENABLE_SCREEN
            FLASHMEM
            virtual SubMenuItemBar *makeControls(const char *label_prefix = "") override;
        #endif

};