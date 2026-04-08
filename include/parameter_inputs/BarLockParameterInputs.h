#pragma once

#include "ParameterInput.h"
#include "AnalogParameterInputBase.h"
#include "bpm.h"
#include <math.h>

// BarLock Modulators — tempo- and bar-aligned modulation sources.
// All modes output a normalized value in [0.0, 1.0] (UNIPOLAR).

enum barlock_mode_id {
    BARLOCK_RISE_LOG,       // BR-Log:    0→1 over a bar, logarithmic curve (fast rise, decelerates)
    BARLOCK_FALL_EXP,       // BF-Exp:    1→0 over a bar, exponential decay (fast initial fall, long tail)
    BARLOCK_LAST_BEAT_RISE, // LBR:       0 for beats 1-(N-1), ramps 0→1 over the final beat
    BARLOCK_LAST_BEAT_FALL, // LBF:       0 for beats 1-(N-1), falls 1→0 over the final beat
    BARLOCK_PHRASE_RISE,    // PR-Lin:    linear ramp 0→1 across phrase_bars bars
    BARLOCK_PHRASE_FALL,    // PF-Smooth: smoothstep fall 1→0 across phrase_bars bars
    BARLOCK_NUM_MODES
};

struct barlock_option_t {
    const char     *name;
    barlock_mode_id id;
};
extern barlock_option_t barlock_options[BARLOCK_NUM_MODES];


class BarLockParameterInput : public AnalogParameterInputBase<float> {
    public:
        barlock_mode_id mode;

        // span in bars used by the phrase-mode sources (PR-Lin, PF-Smooth)
        uint8_t phrase_bars = BARS_PER_PHRASE;

        // per-tick cache: avoid recomputing floats when ticks hasn't advanced
        uint32_t last_computed_tick  = UINT32_MAX; // sentinel: force compute on first call
        float    last_computed_value = 0.0f;

        BarLockParameterInput(char *name, barlock_mode_id mode = BARLOCK_RISE_LOG)
                : AnalogParameterInputBase(name, "BarLock", 0.005f, UNIPOLAR) {
            this->mode = mode;
        }

        virtual const char *getExtra() override {
            return barlock_options[mode].name;
        }

        float get_source_value() {
            // Cache per tick: all float ops (logf, expf, division) are skipped when ticks
            // hasn't advanced, which is the common case between clock pulses.
            if (ticks == last_computed_tick)
                return last_computed_value;
            last_computed_tick = ticks;

            float result;
            switch (mode) {
                case BARLOCK_RISE_LOG: {
                    // log1p(t*(e-1)) maps [0,1]→[0,1] via natural log:
                    // fast rise at bar start, decelerates toward bar end
                    const float bar_pos = (float)(ticks % (uint32_t)TICKS_PER_BAR) / (float)TICKS_PER_BAR;
                    result = logf(1.0f + bar_pos * (float)(M_E - 1.0));
                    break;
                }
                case BARLOCK_FALL_EXP: {
                    // exponential decay 1→0: fast initial fall, long tail
                    // (expf(k*(1-t)) - 1) / (expf(k) - 1)  with k=4
                    static const float k     = 4.0f;
                    static const float denom = expf(4.0f) - 1.0f;
                    const float bar_pos = (float)(ticks % (uint32_t)TICKS_PER_BAR) / (float)TICKS_PER_BAR;
                    result = (expf(k * (1.0f - bar_pos)) - 1.0f) / denom;
                    break;
                }
                case BARLOCK_LAST_BEAT_RISE: {
                    // 0 until the final beat, then ramps 0→1 within that beat
                    if (BPM_CURRENT_BEAT_OF_BAR < (uint32_t)(BEATS_PER_BAR - 1)) {
                        result = 0.0f;
                    } else {
                        result = (float)(ticks % (uint32_t)TICKS_PER_BEAT) / (float)TICKS_PER_BEAT;
                    }
                    break;
                }
                case BARLOCK_LAST_BEAT_FALL: {
                    // 0 until the final beat, then falls 1→0 within that beat
                    if (BPM_CURRENT_BEAT_OF_BAR < (uint32_t)(BEATS_PER_BAR - 1)) {
                        result = 0.0f;
                    } else {
                        result = 1.0f - (float)(ticks % (uint32_t)TICKS_PER_BEAT) / (float)TICKS_PER_BEAT;
                    }
                    break;
                }
                case BARLOCK_PHRASE_RISE: {
                    // linear ramp 0→1 over phrase_bars bars (integer division for speed)
                    const uint32_t phrase_ticks = (uint32_t)TICKS_PER_BAR * phrase_bars;
                    result = (float)(ticks % phrase_ticks) / (float)phrase_ticks;
                    break;
                }
                case BARLOCK_PHRASE_FALL: {
                    // smoothstep S-curve fall 1→0 over phrase_bars bars
                    const uint32_t phrase_ticks = (uint32_t)TICKS_PER_BAR * phrase_bars;
                    const float t = (float)(ticks % phrase_ticks) / (float)phrase_ticks;
                    const float s = t * t * (3.0f - 2.0f * t); // smoothstep [0→1]
                    result = 1.0f - s;
                    break;
                }
                default:
                    result = 0.0f;
                    break;
            }

            last_computed_value = result;
            return result;
        }

        virtual void read() override {
            float currentValue = this->get_source_value();

            if (this->is_significant_change(currentValue, this->lastValue)) {
                this->lastValue = this->currentValue;
                this->currentValue = currentValue;

                #ifdef PARAMETER_INPUTS_USE_CALLBACKS
                    float normal = this->get_normal_value_unipolar(currentValue);
                    this->on_value_read(normal);
                    if (this->callback != nullptr) {
                        (*this->callback)(normal);
                    }
                #endif
            }
        }

        #ifdef ENABLE_STORAGE
            virtual void setup_saveable_settings() override {
                AnalogParameterInputBase::setup_saveable_settings();

                register_setting(new LSaveableSetting<uint8_t>(
                    "Phrase Bars",
                    "BarLockParameterInput",
                    &this->phrase_bars,
                    [=](uint8_t value) -> void { this->phrase_bars = value; },
                    [=](void) -> uint8_t       { return this->phrase_bars; }
                ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);
            }
        #endif

        #ifdef ENABLE_SCREEN
            FLASHMEM
            virtual SubMenuItemBar *makeControls(int16_t memory_size, const char *label_prefix = "") override;
        #endif
};