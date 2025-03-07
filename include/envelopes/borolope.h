#pragma once

// Weirdolope - a minimal one-control envelope generator
// (c) 2022 Russell Borogove
// Use at your own risk

// This envelope generator uses floating point math; 
// if your MCU doesn't have floating-point hardware 
// support you're gonna have a bad time.

// Call updateEnvelope() periodically at a rate >= 1000 Hz

// Set envelopeState to ENVELOPE_STATE_ATTACK to 
// begin the envelope and ENVELOPE_STATE_RELEASE to 
// release it.

// envelopeLevel will range from 0.0 to 1.0. 

// Implement setEnvelope() to scale that value 
// appropriately and send it to your DAC. 

// Thanks to "synthesizers, yo" user meem for 
// getting me some nice clean samples of guitars 
// and pianos that I could measure some 
// representative envelope time curves from. 

// Now modified quite a bit by doctea/Tristan Rowley with full thanks and royalties ;-) to Russell Borogove!

#include "envelopes.h"

class Weirdolope : public EnvelopeBase {

    private:
    float AttackRateTable[9] = 
    {
        1.00000f,     // instant
        0.08333f,     // guitar
        0.04000f,     // bass
        0.01667f,     // piano bass
        0.10000f,     // piano treble
        0.20000f,     // chiff organ
        0.10000f,     // synth lead
        0.00250f,     // synth pad
        0.00050f,     // long ambient pad
    };

    float DecayRateTable[9] = 
    {  
        0.96555f,     // instant
        0.99929f,     // guitar
        0.98851f,     // bass
        0.99934f,     // piano bass
        0.98746f,     // piano treble
        0.98000f,     // chiff organ
        0.99644f,     // synth lead
        0.99978f,     // synth pad
        0.99988f,     // long ambient pad   
    };

    // End-of-decay level
    float SustainLevelTable[9] = 
    {
        0.70f,     // instant
        0.72f,     // guitar
        0.75f,     // bass
        0.70f,     // piano bass
        0.70f,     // piano treble
        0.70f,     // chiff organ
        0.70f,     // synth lead
        0.80f,     // synth pad
        0.90f,     // long ambient pad   
    };

    // Unlike a typical synth ADSR envelope, we use an 
    // exponentially decaying sustain. 
    float SustainRateTable[9] = 
    {
        0.99880f,     // instant
        0.99928f,     // guitar
        0.99957f,     // bass
        0.99913f,     // piano bass
        0.99015f,     // piano treble
        0.99977f,     // chiff organ
        0.99991f,     // synth lead
        1.0f,     // synth pad
        1.0f,     // long ambient pad   
    };

    // Unlike a typical synth ADSR envelope, we use an 
    // exponentially decaying sustain
    float ReleaseRateTable[9] = 
    {
        0.99312f,     // instant
        0.99421f,     // guitar
        0.99484f,     // bass
        0.99092f,     // piano bass
        0.97727f,     // piano treble
        0.99500f,     // chiff organ
        0.99500f,     // synth lead
        0.99650f,     // synth pad
        0.99800f,     // long ambient pad   
    };

    static const int RELEASE_MULTIPLIER = 3;    // multiplier for release values, because otherwise they're a bit sillylong

    int EnvA = 0;
    int EnvB = 1;
    float EnvAlpha = 0.0f;

    long long lastUpdatedClock = 0l;

    float envelopeLevel = 0.0f;
    // silence the envelope when it reaches this level, well below 12 bit dac resolution.
    float envelopeStopLevel = 0.0001f;
    unsigned long nextEnvelopeUpdate = 0;

    float base_level;

    float paramValueA = 0.0f;

    float lerp( float y0, float y1, float alpha )
    {
        return (y1-y0)*alpha + y0;
    }

    using SetterCallback = void (*)(float,bool);
    using StateChangeCallback = void (*)(int, int);

    // callback handlers
    SetterCallback setterCallback;
    StateChangeCallback stateChangeCallback;

    //bool inverted = false;
    //float idleSlew = false;
    float slewRate = 0.0f;

    unsigned long stageStartedAt;
    float stageStartLevel = 0.0f;

    public:

    Weirdolope(const char *label, setter_func_def setter) : EnvelopeBase(label, setter) {
        this->setMix(0.5);
    }
    virtual ~Weirdolope() {}

    //int envelopeState = stage_t::OFF;

    bool dirty_calc = false;
    virtual void set_dirty_graph(bool v = true) {
        EnvelopeBase::set_dirty_graph(v);
        if (v) this->dirty_calc = true;
    }
    virtual bool is_dirty_calc() {
        return this->dirty_calc;
    }
    virtual void clear_dirty_calc() {
        this->dirty_calc = false;
    }

    void setSlewRate(float in_slew = true) {
        slewRate = in_slew;
        this->set_dirty_graph();
    }

    void setMix(float v) {
        bool should_recalculate = this->paramValueA != v;
        this->paramValueA = v;
        if (should_recalculate) {
            this->set_dirty_graph();
        }
    }
    float getMix() {
        return this->paramValueA;
    }

    uint16_t last_processed_elapsed = -1;
    envelope_state_t cached_state;
    virtual envelope_state_t calculate_envelope_level(stage_t stage, uint16_t stage_elapsed, float level_start, float velocity = 1.0f, bool use_caching = true) {
        if (use_caching && last_state.stage==stage && last_processed_elapsed==stage_elapsed && !this->is_dirty_calc())
            return cached_state;    // todo: need to check dirty flag!

        float x = constrain( 8.0f * (paramValueA/10.0f), 0.0f, 7.999f );
        EnvA = int(x);
        EnvB = EnvA+1;
        EnvAlpha = constrain( x - EnvA, 0.0f, 1.0f );

        float envelopeLevel = ((float)level_start);
        //float delta, damp;

        envelope_state_t return_state;
        return_state.stage = stage;
        return_state.lvl_start = level_start;
        return_state.elapsed = stage_elapsed;

        if (debug) Serial.printf("%s:\tcalculate_envelope_level(stage=%i,\tstage_elapsed=%i,\tlevel_start=%1.3f,\tvelocity=%1.3f)", this->label, stage, stage_elapsed, level_start, velocity);

        if (stage==OFF) {
            envelopeLevel = 0.0;
            if (this->is_loop())
                return_state.stage = ATTACK;
            /*envelopeLevel = lerp(
                //inverted ? 1.0f-stageStartLevel : stageStartLevel, 
                stageStartLevel,
                0.0f, 
                //constrain((bpm_clock()-stageStartedAt)/slewRate, 0.0f, 1.0f)
                constrain((float)stage_elapsed/slewRate, 0.0f, 1.0f)
            );*/
        } else if (stage==ATTACK) {
            float delta = lerp( AttackRateTable[EnvA], AttackRateTable[EnvB], EnvAlpha );
            if (debug && Serial) Serial.printf("%s: ATTACK phase, stage_elapsed=%i, envelopeLevel=%1.3f, delta=%1.3f =>", this->label, stage_elapsed, envelopeLevel, delta);
            envelopeLevel += (delta * (float)(stage_elapsed+1));
            if (debug && Serial) Serial.printf("%3.3f\n", envelopeLevel);
            if (envelopeLevel >= 1.0f) {
                envelopeLevel = 1.0f;
                return_state.lvl_start = envelopeLevel;
                return_state.stage = DECAY;
            }
        } else if (stage==DECAY) {
            float damp = lerp( DecayRateTable[EnvA], DecayRateTable[EnvB], EnvAlpha );
            //envelopeLevel *= damp;
            //envelopeLevel *= pow(damp,(float)stage_elapsed);
            for (int i = 0 ; i < stage_elapsed ; i++) {
                envelopeLevel *= damp;
            }
            float sustainLevel = lerp(SustainLevelTable[EnvA], SustainLevelTable[EnvB], EnvAlpha);
            if (envelopeLevel <= sustainLevel) {
                return_state.stage = SUSTAIN;
            }
        } else if (stage==SUSTAIN) {
            float damp = lerp( SustainRateTable[EnvA], SustainRateTable[EnvB], EnvAlpha );
            //envelopeLevel *= pow(damp,(float)stage_elapsed);
            for (int i = 0 ; i < stage_elapsed ; i++) {
                envelopeLevel *= damp;
            }
            if (envelopeLevel <= envelopeStopLevel) {
                return_state.stage = RELEASE;
            }
        } else if (stage==RELEASE) {
            float damp = lerp( ReleaseRateTable[EnvA], ReleaseRateTable[EnvB], EnvAlpha );
            //envelopeLevel *= pow(damp,(float)stage_elapsed);
            for (int i = 0 ; i < (stage_elapsed * RELEASE_MULTIPLIER) ; i++) {   // modify release time in order to shorten the release time, as its a bit ridiculous otherwise
                envelopeLevel *= damp;
            }
            if (envelopeLevel <= envelopeStopLevel) {
                if (debug && Serial) Serial.printf("%s:\t!!!! Release staged reached level %1.3f (passing threshold %3.3f) after %i stages\n", this->label, envelopeLevel, envelopeStopLevel, stage_elapsed);
                return_state.stage = OFF;
                envelopeLevel = 0.0f;
            } else {
                //if (Serial) Serial.printf("%s:\tRelease stage reached level %3.3f after %i stages\n", this->label, envelopeLevel, stage_elapsed);
            }
        }

        return_state.lvl_now = envelopeLevel;

        if (debug && Serial) Serial.printf(" => %i & %i from envelopeLevel=%1.3f\n", return_state.lvl_now, return_state.lvl_start, envelopeLevel);

        if (use_caching) {
            this->clear_dirty_calc();
            cached_state.elapsed    = return_state.elapsed;
            cached_state.lvl_now    = return_state.lvl_now;
            cached_state.lvl_start  = return_state.lvl_start;
            cached_state.stage      = return_state.stage;
            last_processed_elapsed  = stage_elapsed;
        }

        return return_state;
    }


    virtual void randomise() override {
        this->setMix(random(0.0f, 10.0f));
        this->set_invert((int8_t)random(0,10) < 2);
    }
    virtual void update_state(float velocity, bool state, uint32_t now = ticks) override {
        if (!state) {
            if (stage==ATTACK || stage==HOLD || stage==DECAY || stage==SUSTAIN) { //!=OFF && stage!=RELEASE) {
                last_state.stage = stage = RELEASE;
                last_state.lvl_start = last_state.lvl_now;
                triggered_at = stage_triggered_at = now;
                last_sent_at = 0;
            }
        } else {
            last_state.stage = stage = ATTACK;
            last_state.lvl_start = last_state.lvl_now;
            triggered_at = stage_triggered_at = now;
            last_sent_at = 0;
        }
    }

    #ifdef ENABLE_PARAMETERS
        virtual LinkedList<FloatParameter*> *get_parameters() override;
    #endif

    #ifdef ENABLE_SCREEN
        FLASHMEM
        virtual void make_menu_items(Menu *menu, int index) override;
    #endif

};