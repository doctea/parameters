#include "parameter_inputs/ParameterInput.h"
#include "parameter_inputs/VirtualParameterInput.h"

#include "menuitems_lambda.h"
#include "submenuitem_bar.h"
#include "mymenu_items/ParameterInputMenuItems.h"
#include "mymenu_items/ParameterInputViewMenuItems.h"

const char *BaseParameterInput::prefix = "parameter_input_";
const char *BaseParameterInput::input_type_suffix = "_input_type";

lfo_option_t virtual_parameter_options[lfo_option_id::NUM] = {
    { "FreeLFO", LFO_FREE },
    { "LockLFO", LFO_LOCKED },
    { "Rand",    RAND }
};

#ifdef ENABLE_SCREEN
    FLASHMEM
    SubMenuItemBar *BaseParameterInput::makeControls(int16_t memory_size, const char *label_prefix) {
        // TODO: a new ParameterInputControl that allows to set expected input ranges
        char label[MENU_C_MAX];
        snprintf(label, MENU_C_MAX, "%s%s", label_prefix, this->name);
        //char *label = param_input->name;

        menu->add(new SeparatorMenuItem(label, this->colour));

        //Debug_printf(F("\tdoing menu->add for ParameterInputDisplay with label '%s'\n"), label);
        ParameterInputDisplay *parameter_input_display = new ParameterInputDisplay(label, memory_size, this);
        #ifdef PARAMETER_INPUTS_USE_CALLBACKS
            param_input->add_parameter_input_callback_receiver(parameter_input_display);
        #endif
        menu->add(parameter_input_display);

        if (this->supports_bipolar_input()) {
            SubMenuItemBar *submenu = new SubMenuItemBar("Further options", true);
            submenu->show_header = false;
            submenu->default_fg = this->colour;
            // inputs now rely on their parameter to choose whether to use polar or bipolar version
            InputTypeSelectorControl *type_selector = new InputTypeSelectorControl("Polarity", &this->input_type);
            type_selector->default_fg = this->colour;
            //type_selector->show_header = false;
            submenu->add(type_selector);

            submenu->add(new LambdaToggleControl(
                "Invert", 
                [=](bool v) -> void { this->setInverted(v); }, 
                [=](void) -> bool { return this->inverted; }
            ));

            menu->add(submenu); // Input type

            return submenu;
        }
        // todo: maybe add options for inverted, rectified, etc?
        return nullptr;
    }

    FLASHMEM
    SubMenuItemBar *VirtualParameterInput::makeControls(int16_t memory_size, const char *label_prefix) {
        SubMenuItemBar *submenu = BaseParameterInput::makeControls(memory_size, label_prefix);

        if (lfo_mode==LFO_FREE)
            submenu->add(new DirectNumberControl<float>("Speed", &this->free_sine_divisor, this->free_sine_divisor, 0.01f, 1000.0f));
        else if (lfo_mode==LFO_LOCKED) {
            LambdaSelectorControl<float> *period_control = new LambdaSelectorControl<float>(
                "Period",
                [=] (float v) -> void { this->locked_period = v; },
                [=] (void) -> float { return this->locked_period; }
            );
            period_control->add_available_value(0.25f, "Beat");
            period_control->add_available_value(0.5f,  "2xBeat");
            period_control->add_available_value(0.75f, "3xBeat");
            period_control->add_available_value(1.0f,  "Bar");
            period_control->add_available_value(2.0f,  "2xBar");
            period_control->add_available_value(3.0f,  "3xBar");
            period_control->add_available_value(4.0f,  "4xBar"); //Phrase");
            period_control->add_available_value(8.0f,  "8xBar"); //2xPhrase");
            submenu->add(period_control);
        }
        if(lfo_mode==LFO_FREE || lfo_mode==LFO_LOCKED) {
            DirectNumberControl<float> *phase_control = new DirectNumberControl<float>(
                "Phase",
                &this->locked_phase,
                this->locked_phase,
                0.0f,
                1.0f
            );
            submenu->add(phase_control);
        }

        return submenu;
    }
#endif