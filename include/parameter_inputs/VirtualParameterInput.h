#pragma once

#include "ParameterInput.h"
#include "AnalogParameterInputBase.h"

#include "../voltage_sources/VoltageSource.h"

#include "bpm.h"
#include "midi_helpers.h"

#include "ads.h"

// todo: (more) options to configure LFO type, speed, etc
// todo: support MIDI pitch generation

// Backward-compatible source selector used by current call sites and save data.
enum lfo_option_id {
    LFO_FREE,
    LFO_LOCKED,
    RAND,
    NUM
};

// New split model: source family is separate from waveform shape.
enum virtual_lfo_source_id {
    VIRTUAL_LFO_FREE,
    VIRTUAL_LFO_LOCKED,
    VIRTUAL_LFO_RAND,
};

enum virtual_lfo_waveform_id {
    VIRTUAL_LFO_WAVE_SINE,
    VIRTUAL_LFO_WAVE_TRIANGLE,
    VIRTUAL_LFO_WAVE_SAW,
    VIRTUAL_LFO_WAVE_SQUARE,
};

constexpr int NUM_WAVEFORMS = (int)VIRTUAL_LFO_WAVE_SQUARE + 1;

struct lfo_option_t {
    const char *name;
    lfo_option_id id;
};

extern lfo_option_t virtual_parameter_options[lfo_option_id::NUM];
#define MAX_LFO_ID (sizeof(virtual_parameter_options) / sizeof(lfo_option_t))

class VirtualParameterInput : public AnalogParameterInputBase<float> {
    public:
        const lfo_option_id lfo_mode;
        const virtual_lfo_source_id source_id;
        virtual_lfo_waveform_id waveform_id = VIRTUAL_LFO_WAVE_SINE;

        // wave parameters
        float free_sine_divisor = 100.0f;
        float locked_period = 4.0f;
        float locked_phase = 0.0f;

        // track sample & hold state
        // todo: move this into parent class, and make it available to VoltageParameterInput etc? could be useful for eg sample&hold on CV inputs
        uint32_t sh_ticks = 0;
        float last_sample = 0;
        uint32_t last_sample_tick = 0;

        // Phase accumulator (NCO): advances per-tick so that changing free_sine_divisor
        // mid-run causes no phase jump for FREE mode.  For LOCKED modes the accumulator
        // is resynced to the bar-aligned clock position whenever locked_period or
        // TICKS_PER_BAR (time signature) changes, or when the clock is restarted.
        float phase_acc = 0.0f;
        uint32_t last_advanced_tick = 0;
        float last_step = -1.0f;  // sentinel: force resync on first call

        // When true, makeControls() skips creating a ParameterInputDisplay and
        // UI controls for this instance (~500 bytes saved).  The input is still
        // fully functional as a modulation source.
        bool lightweight = false;

        VirtualParameterInput(char *name, const char *group_name,
                              lfo_option_id lfo_mode = LFO_LOCKED,
                              float locked_period = 4.0f, float locked_phase = 0.0f,
                              uint32_t sh_ticks = 0, bool lightweight = false)
                                : AnalogParameterInputBase(name, group_name),
                                    lfo_mode(lfo_mode),
                                    source_id(this->source_for_mode(lfo_mode)),
                                    waveform_id(this->waveform_for_mode(lfo_mode)) {
            this->locked_period = locked_period;
            this->locked_phase  = locked_phase;
            this->sh_ticks      = sh_ticks;
            this->lightweight   = lightweight;
        }

        static virtual_lfo_source_id source_for_mode(lfo_option_id mode) {
            if (mode == LFO_FREE) return VIRTUAL_LFO_FREE;
            if (mode == RAND) return VIRTUAL_LFO_RAND;
            return VIRTUAL_LFO_LOCKED;
        }

        static virtual_lfo_waveform_id waveform_for_mode(lfo_option_id mode) {
            (void)mode;
            return VIRTUAL_LFO_WAVE_SINE;
        }

        static const char *waveform_name(virtual_lfo_waveform_id waveform) {
            switch (waveform) {
                case VIRTUAL_LFO_WAVE_SINE: return "Sine";
                case VIRTUAL_LFO_WAVE_TRIANGLE: return "Tri";
                case VIRTUAL_LFO_WAVE_SAW: return "Saw";
                case VIRTUAL_LFO_WAVE_SQUARE: return "Sqr";
                default: return "Sine";
            }
        }

        void set_waveform_id(virtual_lfo_waveform_id waveform) {
            if (!this->is_rand_source()) {
                this->waveform_id = waveform;
            }
        }

        bool is_free_source() const { return this->source_id == VIRTUAL_LFO_FREE; }
        bool is_locked_source() const { return this->source_id == VIRTUAL_LFO_LOCKED; }
        bool is_rand_source() const { return this->source_id == VIRTUAL_LFO_RAND; }

        bool is_locked_waveform() const { return this->source_id == VIRTUAL_LFO_LOCKED; }

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

        virtual const char *getExtra() override {
            return this->is_rand_source() ? "Rand" : waveform_name(this->waveform_id);
        }
        virtual bool hasExtra() override {
            return true;
        }

        virtual bool supports_inverted() {
            return !this->is_rand_source(); // makes no sense to invert a random value since its random across the range anyway
        }

        float get_source_value() {
            // Advance phase accumulator to current tick (catch-up).
            // All LFO modes share this accumulator; RAND does not use it.
            // Because loop() runs far faster than tick rate, elapsed is almost always 1,
            // but the catch-up handles cases where this is called less frequently.
            if (!this->is_rand_source()) {
                const float step = (this->is_free_source())
                    ? (1.0f / free_sine_divisor)
                    : (1.0f / ((float)TICKS_PER_BAR * locked_period));
                if (last_advanced_tick != ticks) {
                    if (!this->is_free_source() && step != last_step) {
                        // Period or time-signature changed (TICKS_PER_BAR is runtime-variable):
                        // resync accumulator to bar-aligned absolute position using
                        // BPM_PHASE_TICKS so the result is relative to the current time sig.
                        phase_acc = fmodf((float)BPM_PHASE_TICKS(ticks) * step, 1.0f);
                        last_step = step;
                    } else if (last_advanced_tick > ticks) {
                        // Clock reset: resync accumulator to bar-aligned position
                        phase_acc = fmodf((float)BPM_PHASE_TICKS(ticks) * step, 1.0f);
                        last_step = step;
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

                if (this->is_rand_source()) {
                    last_sample = input_type == BIPOLAR
                        ? (float)random(-1000, 1000) / 1000.0f
                        : (float)random(0, 1000) / 1000.0f;
                } else {
                    switch (this->waveform_id) {
                        case VIRTUAL_LFO_WAVE_SINE:
                            last_sample = calculate_lfo(frac);
                            break;
                        case VIRTUAL_LFO_WAVE_TRIANGLE: {
                            const float v = 1.0f - 2.0f * fabsf(frac - 0.5f);
                            last_sample = (input_type == BIPOLAR) ? (v * 2.0f - 1.0f) : v;
                            break;
                        }
                        case VIRTUAL_LFO_WAVE_SAW:
                            last_sample = (input_type == BIPOLAR) ? (frac * 2.0f - 1.0f) : frac;
                            break;
                        case VIRTUAL_LFO_WAVE_SQUARE: {
                            const float v = (frac < 0.5f) ? 1.0f : 0.0f;
                            last_sample = (input_type == BIPOLAR) ? (v * 2.0f - 1.0f) : v;
                            break;
                        }
                        default:
                            last_sample = calculate_lfo(frac);
                            break;
                    }
                }
            }
            // if (sh_ticks == PPQN) Serial.printf("!! returning %3.3f !! on beat %u\n", last_sample, ticks / PPQN);
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
            
            // if VirtualParameterInputs (almost) always update currentValue on read to ensure that callbacks get fulfilled
            // todo: maybe we should limit this a little tho to prevent churn?
            // if (this->sh_ticks > 0 || this->is_significant_change(currentValue, this->lastValue)) {
            if (true) {
                this->lastValue = this->currentValue;
                this->currentValue = currentValue;
                //Serial.printf("%c: Setting this->currentValue to ", this->name);
                //Serial.println(currentValue);
                //this->currentValue = currentValue;
                //this->currentValue = currentValue = this->get_normal_value(currentValue);
                #ifdef ENABLE_PRINTF
                    if (this->debug) {
                        Debug_printf(F("%c: VoltageParameterInput->read() got voltage "), this->name); //
                        Debug_println(currentValue);
                    }

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

                // Type is immutable after creation. Only behavior-shaping settings are saveable.
                if (!this->is_rand_source()) {
                    register_setting(new LSaveableSetting<virtual_lfo_waveform_id>(
                        "Waveform",
                        "VirtualParameterInput",
                        &this->waveform_id,
                        [=](virtual_lfo_waveform_id value) -> void { this->set_waveform_id(value); },
                        [=](void) -> virtual_lfo_waveform_id { return this->waveform_id; }
                    ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);
                }

                if (this->is_free_source()) {
                    register_setting(new VarSetting<float>(
                            "Free Sine Divisor",
                            "VirtualParameterInput",
                            &this->free_sine_divisor
                        ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);
                }

                if (this->is_locked_source()) {
                    register_setting(new VarSetting<float>(
                            "Locked Period",
                            "VirtualParameterInput",
                            &this->locked_period
                        ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);
                }

                if (!this->is_rand_source()) {
                    register_setting(new VarSetting<float>(
                            "Locked Phase",
                            "VirtualParameterInput",
                            &this->locked_phase
                        ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);
                }

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

class FreeVirtualParameterInput : public VirtualParameterInput {
    public:
        FreeVirtualParameterInput(char *name, const char *group_name,
                                  float free_sine_divisor = 100.0f,
                                  virtual_lfo_waveform_id waveform_id = VIRTUAL_LFO_WAVE_SINE,
                                  float locked_phase = 0.0f,
                                  uint32_t sh_ticks = 0,
                                  bool lightweight = false)
            : VirtualParameterInput(name, group_name, LFO_FREE, 4.0f, locked_phase, sh_ticks, lightweight) {
            this->waveform_id = waveform_id;
            this->free_sine_divisor = free_sine_divisor;
        }
};

class LockedVirtualParameterInput : public VirtualParameterInput {
    public:
        LockedVirtualParameterInput(char *name, const char *group_name,
                                    virtual_lfo_waveform_id waveform_id = VIRTUAL_LFO_WAVE_SINE,
                                    float locked_period = 4.0f,
                                    float locked_phase = 0.0f,
                                    uint32_t sh_ticks = 0,
                                    bool lightweight = false)
            : VirtualParameterInput(name, group_name, LFO_LOCKED, locked_period, locked_phase, sh_ticks, lightweight) {
            this->waveform_id = waveform_id;
        }
};

class RandomVirtualParameterInput : public VirtualParameterInput {
    public:
        RandomVirtualParameterInput(char *name, const char *group_name,
                                    uint32_t sh_ticks = 0,
                                    bool lightweight = false)
            : VirtualParameterInput(name, group_name, RAND, 4.0f, 0.0f, sh_ticks, lightweight) {
            this->waveform_id = VIRTUAL_LFO_WAVE_SINE;
        }
};

inline VirtualParameterInput *makeVirtualParameterInput(
    char *name,
    const char *group_name,
    lfo_option_id lfo_mode = LFO_LOCKED,
    float locked_period = 4.0f,
    float locked_phase = 0.0f,
    uint32_t sh_ticks = 0,
    bool lightweight = false
) {
    switch (lfo_mode) {
        case LFO_FREE:
            return new FreeVirtualParameterInput(name, group_name, 100.0f, VIRTUAL_LFO_WAVE_SINE, locked_phase, sh_ticks, lightweight);
        case RAND:
            return new RandomVirtualParameterInput(name, group_name, sh_ticks, lightweight);
        case LFO_LOCKED:
        default:
            return new LockedVirtualParameterInput(name, group_name, VIRTUAL_LFO_WAVE_SINE, locked_period, locked_phase, sh_ticks, lightweight);
    }
}