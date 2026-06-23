#include "parameter_inputs/ParameterInput.h"
#include "parameter_inputs/VirtualParameterInput.h"
#include "parameter_inputs/BarLockParameterInputs.h"

lfo_option_t virtual_parameter_options[lfo_option_id::NUM] = {
    { "Free", LFO_FREE },
    { "Sync", LFO_LOCKED },
    { "Rand", RAND },
};

static const virtual_lfo_waveform_id virtual_waveform_option_ids[] = {
    VIRTUAL_LFO_WAVE_SINE,
    VIRTUAL_LFO_WAVE_TRIANGLE,
    VIRTUAL_LFO_WAVE_SAW,
    VIRTUAL_LFO_WAVE_SQUARE,
};

static const char *virtual_waveform_option_names[] = {
    "Sine", "Tri", "Saw", "Sqr"
};

constexpr int NUM_WAVEFORM_OPTIONS =
    sizeof(virtual_waveform_option_ids) / sizeof(virtual_waveform_option_ids[0]);

barlock_option_t barlock_options[BARLOCK_NUM_MODES] = {
    { "BR-Log",    BARLOCK_RISE_LOG       },
    { "BF-Exp",    BARLOCK_FALL_EXP       },
    { "LBR",       BARLOCK_LAST_BEAT_RISE },
    { "LBF",       BARLOCK_LAST_BEAT_FALL },
    { "PR-Lin",    BARLOCK_PHRASE_RISE    },
    { "PF-Smooth", BARLOCK_PHRASE_FALL    },
};

#ifdef ENABLE_SCREEN
    #include "menuitems_lambda.h"
    #include "submenuitem_bar.h"
    #include "mymenu_items/ParameterInputMenuItems.h"
    #include "mymenu_items/ParameterInputViewMenuItems.h"
    #include "mymenu_items/ParameterInputTypeSelector.h"

    FLASHMEM
    SubMenuItemBar *BaseParameterInput::makeControls(const char *label_prefix) {
        // TODO: a new ParameterInputControl that allows to set expected input ranges
        char label[MENU_C_MAX];
        snprintf(label, MENU_C_MAX, "%s%s", label_prefix, this->name);
        //char *label = param_input->name;

        menu->add(new SeparatorMenuItem(label, this->colour));

        //Debug_printf(F("\tdoing menu->add for ParameterInputDisplay with label '%s'\n"), label);
        ParameterInputDisplay *parameter_input_display = new ParameterInputDisplay(label, this);
        #ifdef PARAMETER_INPUTS_USE_CALLBACKS
            this->add_parameter_input_callback_receiver(parameter_input_display);
        #endif
        this->parameter_input_display = parameter_input_display;
        menu->add(parameter_input_display);

        if (this->supports_bipolar_input()) {
            SubMenuItemBar *submenu = new SubMenuItemBar("Further options", true);
            submenu->flags.show_header = false;
            submenu->default_fg = this->colour;
            // inputs now rely on their parameter to choose whether to use polar or bipolar version
            InputTypeSelectorControl<> *type_selector = new InputTypeSelectorControl<>("Polarity", &this->input_type);
            type_selector->default_fg = this->colour;
            //type_selector->flags.show_header = false;
            submenu->add(type_selector);

            // todo: invert should probably be valid even for non-bipolar inputs?
            if (this->supports_inverted()) {
                submenu->add(new LambdaToggleControl(
                    "Invert", 
                    [=](bool v) -> void { this->setInverted(v); }, 
                    [=](void) -> bool { return this->inverted; }
                ));
            }

            menu->add(submenu); // Input type

            return submenu;
        }
        // todo: maybe add options for inverted, rectified, etc?
        return nullptr;
    }

    // for caching available_values list for period controls, saves a couple of hundred bytes
    OptionList<LambdaSelectorControl<float>::option> *period_options = nullptr;
    OptionList<LambdaSelectorControl<uint32_t>::option> *sh_period_options = nullptr;
    OptionList<LambdaSelectorControl<int>::option> *shape_options = nullptr;

    FLASHMEM
    SubMenuItemBar *VirtualParameterInput::makeControls(const char *label_prefix) {
        // Lightweight instances (pre-configured) skip display and controls to save RAM
        if (lightweight) return nullptr;

        SubMenuItemBar *submenu = BaseParameterInput::makeControls(label_prefix);

        // Shape selector: available for both FREE and LOCKED (everything except RAND).
        if (!this->is_rand_source()) {
            LambdaSelectorControl<int> *shape_control = new LambdaSelectorControl<int>(
                "Shape",
                [=](int v) -> void {
                    this->set_waveform_id((virtual_lfo_waveform_id)v);
                },
                [=](void) -> int {
                    return (int)this->waveform_id;
                }
            );
            shape_control->flags.go_back_on_select = true;
            if (shape_options == nullptr) {
                for (int i = 0; i < (int)NUM_WAVEFORM_OPTIONS; i++) {
                    shape_control->add_available_value((int)virtual_waveform_option_ids[i], virtual_waveform_option_names[i]);
                }
                shape_options = shape_control->get_available_values();
            } else {
                shape_control->set_available_values(shape_options);
            }
            submenu->add(shape_control);
        }

        if (this->is_free_source()) {
            submenu->add(new DirectNumberControl<float>("Speed", &this->free_sine_divisor, this->free_sine_divisor, 0.01f, 1000.0f));
        } else if (this->is_locked_source()) {
            // Period control applies to all locked-period waveforms.
            LambdaSelectorControl<float> *period_control = new LambdaSelectorControl<float>(
                "Period",
                [=] (float v) -> void { this->locked_period = v; },
                [=] (void) -> float { return this->locked_period; }
            );
            if (period_options==nullptr) {
                period_control->add_available_value(0.25f, "Beat");
                period_control->add_available_value(0.5f,  "2xBeat");
                period_control->add_available_value(0.75f, "3xBeat");
                period_control->add_available_value(1.0f,  "Bar");
                period_control->add_available_value(2.0f,  "2xBar");
                period_control->add_available_value(3.0f,  "3xBar");
                period_control->add_available_value(4.0f,  "4xBar"); //Phrase");
                period_control->add_available_value(8.0f,  "8xBar"); //2xPhrase");
                period_control->add_available_value(16.0f,  "16xBar"); //4xPhrase");
                period_options = period_control->get_available_values();
            } else {
                period_control->set_available_values(period_options);
            }
            submenu->add(period_control);
        }
        if (!this->is_rand_source()) {
            // Phase applies to all non-random modes
            DirectNumberControl<float> *phase_control = new DirectNumberControl<float>(
                "Phase",
                &this->locked_phase,
                this->locked_phase,
                0.0f,
                1.0f
            );
            submenu->add(phase_control);
        }
        if (this->is_rand_source()) {
            // S&H only applies to random modes
            LambdaSelectorControl<uint32_t> *sh_control = new LambdaSelectorControl<uint32_t>(
                "S&H On",
                [=] (uint32_t v) -> void { this->sh_ticks = v; },
                [=] (void) -> uint32_t { return this->sh_ticks; }
            );
            sh_control->flags.go_back_on_select = true;
            if (sh_period_options==nullptr) {
                sh_control->add_available_value(0,      "None");
                sh_control->add_available_value(PPQN/8, "32nd");
                sh_control->add_available_value(PPQN/4, "16th");
                sh_control->add_available_value(PPQN/2, "8th");
                sh_control->add_available_value(PPQN,   "Beat");
                sh_control->add_available_value(PPQN*2, "2xBeat");
                sh_control->add_available_value(PPQN*BEATS_PER_BAR,    "Bar");
                sh_control->add_available_value(PPQN*BEATS_PER_BAR*2,  "2xBar"); //Phrase");
                sh_control->add_available_value(PPQN*BEATS_PER_PHRASE, "Phrase"); //2xPhrase");
                sh_period_options = sh_control->get_available_values();
            } else {
                sh_control->set_available_values(sh_period_options);
            }
            submenu->add(sh_control);
        }

        return submenu;
    }

    FLASHMEM
    SubMenuItemBar *BarLockParameterInput::makeControls(const char *label_prefix) {
        SubMenuItemBar *submenu = BaseParameterInput::makeControls(label_prefix);

        // Phrase-spanning modes expose the phrase length control
        if (mode == BARLOCK_PHRASE_RISE || mode == BARLOCK_PHRASE_FALL) {
            static OptionList<LambdaSelectorControl<uint8_t>::option> *phrase_bar_options = nullptr;
            LambdaSelectorControl<uint8_t> *bars_control = new LambdaSelectorControl<uint8_t>(
                "Phrase Bars",
                [=](uint8_t v) -> void { this->phrase_bars = v; },
                [=](void)     -> uint8_t { return this->phrase_bars; }
            );
            if (phrase_bar_options == nullptr) {
                bars_control->add_available_value(1, "1 bar");
                bars_control->add_available_value(2, "2 bars");
                bars_control->add_available_value(4, "4 bars");
                bars_control->add_available_value(8, "8 bars");
                phrase_bar_options = bars_control->get_available_values();
            } else {
                bars_control->set_available_values(phrase_bar_options);
            }
            submenu->add(bars_control);
        }

        return submenu;
    }
#endif