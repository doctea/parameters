// TODO: allow Envelopes to have their types dynamically switched... (will be very tricky due to needing different UI..)

#pragma once

#include "bpm.h"
#include "functional-vlpp.h"
#include "SinTables.h"

#define SUSTAIN_MINIMUM   0.01f   // was 32         // minimum sustain volume to use (below this becomes inaudible, so cut it off)
#define ENV_MAX_ATTACK    (PPQN*2) //48 // maximum attack stage length in ticks
#define ENV_MAX_HOLD      (PPQN*2) //48 // maximum hold stage length
#define ENV_MAX_DECAY     (PPQN*2) //48 // maximum decay stage length
#define ENV_MAX_RELEASE   (PPQN*4) //96 // maximum release stage length

//#if defined(ENABLE_SCREEN) // also need this for parameters support, so need a way to tell if that should be activated here
#include <LinkedList.h>
//#endif

#ifdef ENABLE_SCREEN
    class Menu;
#endif
#ifdef ENABLE_PARAMETERS
    class FloatParameter;
#endif

enum stage_t : int8_t {
    OFF = 0,
    ATTACK,
    HOLD, // time
    DECAY,
    SUSTAIN,
    RELEASE,
};
stage_t operator++ (stage_t& d);

class EnvelopeBase { 
    private:
    bool loop_mode = false;
    bool invert = false;
    bool dirty_graph = true;

    public:
    bool debug = false;
    const char *label = nullptr;
    using setter_func_def = vl::Func<void(float)>;
    setter_func_def setter;

    EnvelopeBase(const char *label, setter_func_def setter) {
        this->label = label;
        this->setter = setter;
    }

    stage_t stage = OFF;
    float velocity = 1.0f;         // triggered velocity
    float actual_level = 0.0f;          // right now, the level
    unsigned long stage_triggered_at = 0;
    unsigned long triggered_at = 0; 
    unsigned long last_sent_at = 0;
    int trigger_on = 0; // 0->19 = trigger #, 20 = off, 32->51 = trigger #+loop, 64->84 = trigger #+invert, 96->116 = trigger #+loop+invert
    float last_sent_lvl; // value but not inverted
    float last_sent_actual_lvl;  // actual midi value sent

    virtual bool is_dirty_graph() {
        return this->dirty_graph;
    }
    virtual void set_dirty_graph(bool v = true) {
        this->dirty_graph = true;
    }
    virtual void clear_dirty_graph() {
        EnvelopeBase::set_dirty_graph(false);
    }

    virtual bool is_invert() {
        return invert;
    }
    virtual void set_invert(bool i) {
        this->invert = i;
        this->set_dirty_graph();
    }
    virtual bool is_loop() {
        return loop_mode;
    }
    virtual void set_loop(bool i) {
        this->loop_mode = i;
        this->set_dirty_graph();
    }

    virtual void send_envelope_level(float level) {
        if (is_invert())
            level = 1.0f - level;
        if (last_sent_actual_lvl != level)
            this->setter(level);
        last_sent_actual_lvl = level;
    };

    virtual float get_envelope_level() {
        return this->last_sent_actual_lvl;
    }

    virtual void randomise() = 0;

    virtual void kill() {
        this->stage = OFF;
        this->last_state.stage = OFF;
        this->last_state.lvl_start = 0.0f;
        this->last_state.lvl_now = 0.0f;
        send_envelope_level(0.0f);
    }

    virtual void update_state (float velocity, bool state, uint32_t now = ticks) = 0;

    struct envelope_state_t {
        stage_t stage = OFF;
        float lvl_start = 0.0f;
        float lvl_now = 0.0f;
        uint32_t elapsed = 0;
    };
    virtual envelope_state_t calculate_envelope_level(stage_t stage, uint16_t stage_elapsed, float level_start, float velocity = 1.0f, bool use_caching = true) = 0;

    struct graph_t {
        float value = 0.0f;
        stage_t stage = stage_t::OFF;
    };
    static const int GRAPH_SIZE = 240;
    graph_t graph[GRAPH_SIZE];

    virtual void recalculate_graph_if_necessary() {
        if (this->is_dirty_graph())
            this->calculate_graph();
    }

    virtual void calculate_graph() {
        if (debug) Serial.printf("%s:calculate_graph starting..", this->label);
        envelope_state_t graph_state = {
            .stage = ATTACK,
            .lvl_start = 0.0f,
            .lvl_now = 0.0f
        };
        int stage_elapsed = 0;
        for (int i = 0 ; i < GRAPH_SIZE ; i++) {
            envelope_state_t result = calculate_envelope_level(graph_state.stage, stage_elapsed, graph_state.lvl_start, velocity, false);
            if (result.stage != graph_state.stage) {
                graph_state.lvl_start = result.lvl_now;
                stage_elapsed = 0;
            } else {
                stage_elapsed++;
            }
            graph[i].value = is_invert() ? 1.0f - result.lvl_now : result.lvl_now;
            graph[i].stage = result.stage;
            graph_state.stage = result.stage;
            if (result.stage == SUSTAIN && stage_elapsed >= PPQN) {
                stage_elapsed = 0;
                graph_state.stage = RELEASE;    // move to release after 1 beat, if we are calculating the graph
                graph_state.lvl_start = result.lvl_now;
            }
        }
        if (debug) Serial.printf("%s:calculate_graph finished.", this->label);
        this->clear_dirty_graph();
    }

    envelope_state_t last_state = {
        .stage = OFF,
        .lvl_start = 0.0f,
        .lvl_now = 0.0f,
        .elapsed = 0
    };
    virtual void process_envelope(unsigned long now = millis()) {
        unsigned long elapsed = now - this->stage_triggered_at;
        envelope_state_t new_state = calculate_envelope_level(last_state.stage, elapsed, last_state.lvl_start, velocity, true);
        if (new_state.stage != last_state.stage) {
            new_state.lvl_start = new_state.lvl_now;
            this->stage_triggered_at = now;
        }
        send_envelope_level(new_state.lvl_now);
        last_state.elapsed = elapsed;
        last_state.stage = new_state.stage;
        last_state.lvl_start = new_state.lvl_start;
        last_state.lvl_now = new_state.lvl_now;
    }

    #ifdef ENABLE_SCREEN
        FLASHMEM
        virtual void make_menu_items(Menu *menu, int index);
    #endif

    #ifdef ENABLE_PARAMETERS
        LinkedList<FloatParameter*> *parameters = nullptr;
        virtual LinkedList<FloatParameter*> *get_parameters() {
            return nullptr;
        }
    #endif
};

class RegularEnvelope : public EnvelopeBase {
    public:
    RegularEnvelope(const char *label, setter_func_def setter) : EnvelopeBase(label, setter) {
        this->initialise_parameters();
    }

    unsigned int  attack_length   = 0;                // A - attack  - length of stage
    unsigned int  hold_length     = (PPQN / 4) - 1;   // H - hold    - length to hold at end of attack before decay
    unsigned int  decay_length    = (PPQN / 2) - 1;   // D - decay   - length of stage
    float         sustain_ratio   = 0.90f;            // S - sustain - level to drop to after decay phase
    unsigned int  release_length  = (PPQN / 2) - 1;   // R - release - length (time to drop to 0)

    int8_t lfo_sync_ratio_hold_and_decay = 0;
    int8_t lfo_sync_ratio_sustain_and_release = 0;
    int8_t cc_value_sync_modifier = 12;

    virtual void randomise() {
        this->set_attack((float)random(0,64) / 127.0f);
        this->set_hold((float)random(0,127) / 127.0f);
        this->set_decay((float)random(0,127) / 127.0f);
        this->set_sustain((float)random(64,127) / 127.0f);
        this->set_release((float)random(0,127) / 127.0f);
        this->set_invert((int8_t)random(0,10) < 2);
    }

    virtual void initialise_parameters() {
        this->set_attack(0.0f);
        this->set_hold((float)random(0,127) / 127.0f);
        this->set_decay((float)random(0,127) / 127.0f);
        this->set_sustain((float)random(64,127) / 127.0f);
        this->set_release((float)random(0,127) / 127.0f);
        this->set_invert((int8_t)random(0,10) < 2);
    }

    // received a message that the state of the envelope should change (note on/note off etc)
    virtual void update_state (float velocity, bool state, uint32_t now = ticks) override {
        if (state == true) {
            this->velocity = velocity;
            this->actual_level = velocity;
            this->stage = ATTACK;
            last_state.stage = ATTACK;
            last_state.lvl_start = velocity;
            this->triggered_at = now;
            this->stage_triggered_at = now;
            this->last_sent_at = 0;
        } else if (state == false && this->stage != OFF) {
            switch (this->last_state.stage) {
            case RELEASE:
                return;
            case OFF:
                return;
            case ATTACK:
            case HOLD:
            case DECAY:
            case SUSTAIN:
            default:
                this->stage = last_state.stage = RELEASE;
                last_state.lvl_start = last_state.lvl_now;
                this->stage_triggered_at = now;
                this->last_sent_at = 0;
                break;
            }
        }
    }

    virtual envelope_state_t calculate_envelope_level(stage_t stage, uint16_t stage_elapsed, float level_start, float velocity = 1.0f, bool use_caching = true) override {
        float ratio = (float)PPQN / (float)cc_value_sync_modifier;
        unsigned long elapsed = (float)stage_elapsed * ratio;

        float lvl;
        envelope_state_t return_state = {
            stage,
            level_start,
            level_start
        };

        if (stage == ATTACK) {
            if (attack_length == 0)
                lvl = velocity;
            else
                lvl = (float)velocity * ((float)elapsed / ((float)this->attack_length));
            return_state.lvl_now = lvl;
            if (elapsed >= this->attack_length) {
                return_state = { .stage = ++stage, .lvl_start = lvl, .lvl_now = lvl };
            }
        } else if (stage == HOLD && this->hold_length > 0) {
            lvl = velocity;
            return_state.lvl_now = lvl;
            if (elapsed >= hold_length) {
                return_state = { .stage = ++stage, .lvl_start = lvl, .lvl_now = lvl };
            }
        } else if (stage == HOLD || stage == DECAY) {
            float f_sustain_level = sustain_ratio * velocity;
            float f_original_level = level_start;

            if (stage == HOLD)
                return_state.stage = DECAY;

            if (this->decay_length > 0) {
                float decay_position = ((float)elapsed / (float)(this->decay_length));
                float diff = (f_original_level - (f_sustain_level));
                lvl = f_original_level - (diff * decay_position);
            } else {
                lvl = f_sustain_level; 
            }
            return_state.lvl_now = lvl;
            if (elapsed >= this->decay_length) {
                return_state = { .stage = SUSTAIN, .lvl_start = lvl, .lvl_now = lvl };
            }
        } else if (stage == SUSTAIN) {
            return_state.lvl_now = sustain_ratio * velocity;
            if (sustain_ratio == 0.0f || this->is_loop()) {
                return_state = { .stage = RELEASE, .lvl_start = return_state.lvl_now, .lvl_now = return_state.lvl_now };
            }
        } else if (stage == RELEASE) {
            if (this->sustain_ratio == 0.0f) {
            }
            if (this->release_length > 0) {
                float eR = (float)elapsed / (float)(this->release_length); 
                eR = constrain(eR, 0.0f, 1.0f);
                lvl = (float)level_start * (1.0f - eR);                
            } else {
                lvl = 0.0f;
            }
            return_state.lvl_now = lvl;
            if (elapsed > this->release_length || this->release_length == 0 || lvl == 0.0f) {
                return_state = { .stage = OFF, .lvl_start = lvl, .lvl_now = lvl };
            }
        } else if (stage == OFF) {
            return_state = { .stage = OFF, .lvl_start = 0.0f, .lvl_now = 0.0f };
        }

        if (stage != OFF) {
            float lvl = return_state.lvl_now;
            int sync = (stage == DECAY || stage == HOLD) 
                            ? this->lfo_sync_ratio_hold_and_decay
                            : (stage == SUSTAIN || stage == RELEASE)
                            ? this->lfo_sync_ratio_sustain_and_release 
                            : -1;
            if (sync >= 0) {            
                sync *= 4;
                float mod_amp = lvl / 4.0f;
                float mod_result = mod_amp * isin((float)elapsed * PPQN * ((float)sync / 127.0f));
                lvl = constrain(lvl + mod_result, 0.0f, 1.0f);
                return_state.lvl_now = lvl;
            }           
        } else {
            if (this->is_loop())
                return_state.stage = ATTACK;
        }

        return return_state;
    }

    float attack_value, hold_value, decay_value, sustain_value, release_value;

    virtual void set_attack(float attack) {
        this->attack_value = attack;
        this->attack_length = (ENV_MAX_ATTACK) * attack;
        this->set_dirty_graph();
    }
    virtual float get_attack() {
        return this->attack_value;
    }
    virtual void set_hold(float hold) {
        this->hold_value = hold;
        this->hold_length = (ENV_MAX_HOLD) * hold;
        this->set_dirty_graph();
    }
    virtual float get_hold() {
        return this->hold_value;
    }
    virtual void set_decay(float decay) {
        this->decay_value = decay;
        decay_length = (ENV_MAX_DECAY) * decay;
        this->set_dirty_graph();
    }
    virtual float get_decay() {
        return this->decay_value;
    }
    virtual void set_sustain(float sustain) {
        this->sustain_value = sustain;
        this->sustain_ratio = sustain;
        this->set_dirty_graph();
    }
    virtual float get_sustain() {
        return this->sustain_value;
    }
    virtual void set_release(float release) {
        this->release_value = release;
        release_length = (ENV_MAX_RELEASE) * release;
        this->set_dirty_graph();
    }
    virtual float get_release() {
        return this->release_value;
    }
    virtual int8_t get_stage() {
        return this->stage;
    }
    virtual void set_mod_hd(int8_t hd) {
        this->lfo_sync_ratio_hold_and_decay = hd;
        this->set_dirty_graph();
    }
    virtual int8_t get_mod_hd() {
        return this->lfo_sync_ratio_hold_and_decay;
    }
    virtual void set_mod_sr(int8_t sr) {
        this->lfo_sync_ratio_sustain_and_release = sr;
        this->set_dirty_graph();
    }
    virtual int8_t get_mod_sr() {
        return this->lfo_sync_ratio_sustain_and_release;
    }
    virtual void set_cc_value_sync_modifier(uint8_t sync) {
        this->cc_value_sync_modifier = sync;
    }
    virtual uint8_t get_cc_value_sync_modifier() {
        return this->cc_value_sync_modifier;
    }

    #ifdef ENABLE_PARAMETERS
        virtual LinkedList<FloatParameter*> *get_parameters() override;
    #endif

    #ifdef ENABLE_SCREEN
        FLASHMEM
        virtual void make_menu_items(Menu *menu, int index) override;
    #endif
};