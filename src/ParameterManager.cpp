/*#include "ParameterManager.h"

#ifdef ENABLE_SCREEN

    #include "parameter_inputs/ParameterInput.h"
    #include "mymenu_items/ParameterInputViewMenuItems.h"

    void ParameterManager::addAllParameterInputMenuItems(Menu *menu) {
        Serial.println("ParameterManager#addAllParameterInputMenuItems()...");
        for (unsigned int i = 0 ; i < this->available_inputs.size() ; i++) {
            // TODO: probably merge these things into one new type of MenuItem?
            BaseParameterInput *input = this->available_inputs.get(i);
            //Serial.printf("\t%i: doing menu->add for makeMenuItemForParameterInput()..", i);
            //menu->add(this->makeMenuItemForParameterInput(param));

            //this->addParameterInputMenuItems(menu, input);           
            char label[20];
            sprintf(label, "Graph for %c", this->name);

            Serial.printf("\tdoing menu->add for ParameterInputDisplay with label '%s'\n", label);
            input->addMenuControls(menu);
            menu->add(new ParameterInputDisplay(label, this); //, LOOP_LENGTH_TICKS));
            sprintf(label, "Input type for %c", this->name);
            menu->add(new InputTypeSelectorControl(label, input, set_input_type, get_input_type));
            sprintf(label, "Output type for %c", this->name);
            menu->add(new InputTypeSelectorControl(label, input, set_output_type, get_output_type));
        }
    }

#endif*/
/*
#include "ParameterManager.h"

// todo: different implementations depending on whether we have SD card available or not
bool ParameterManager::load_voltage_calibration(int slot) {
    VoltageSourceBase 
}
bool ParameterManager::save_voltage_calibration(int slot) {

}*/

#include <Arduino.h>

#include "ParameterManager.h"

LinkedList<BaseParameterInput*> *ParameterManager::get_available_pitch_inputs() {
    static LinkedList<BaseParameterInput*> *available_pitch_inputs = new LinkedList<BaseParameterInput*>();
    static bool already_calculated = false;
    if (!already_calculated) {
        const uint_fast8_t size = available_inputs->size();
        for (uint_fast8_t i = 0 ; i < size ; i++) {
            if (available_inputs->get(i)->supports_pitch())
                available_pitch_inputs->add(available_inputs->get(i));
        }
        already_calculated = true;
    }
    return available_pitch_inputs;
}

FLASHMEM ADCDeviceBase *ParameterManager::addADCDevice(ADCDeviceBase *device) {
    Debug_printf(F("ParameterManager#addADCDevice(%p)\n"), device);
    this->devices->add(device);
    return device;
}

FLASHMEM VoltageSourceBase *ParameterManager::addVoltageSource(VoltageSourceBase *voltage_source) {
    Debug_printf(F("ParameterManager#addVoltageSource(%p)\n"), voltage_source);
    #if defined(LOAD_CALIBRATION_ON_BOOT) && defined(ENABLE_CALIBRATION_STORAGE)
        //Debug_printf(F("Loading calibration for %i!\n"), voltage_source->slot);
        Debug_printf("Loading calibration for VoltageSource with global_slot=%i!\n", voltage_source->global_slot);
        voltage_source->load_calibration();
    #endif

    if (voltage_source==nullptr) {
        Serial.println("WARNING: ParameterManager#addVoltageSource() was passed a nullptr!!!");
    }
    this->voltage_sources->add(voltage_source);
    return voltage_source;
}

BaseParameterInput *ParameterManager::addInput(BaseParameterInput *input) {
    Debug_printf(F("ParameterManager#addInput(%p)\n"), input);
    #ifdef ENABLE_SCREEN
        input->colour = parameter_input_colours[this->available_inputs->size() % (sizeof(parameter_input_colours)/2)];
    #endif
    this->available_inputs->add(input);
    return input;
}

FLASHMEM FloatParameter *ParameterManager::addParameter(FloatParameter *parameter) {
    Debug_printf(F("ParameterManager#addParameter(%p), labeled '%s'\n"), parameter, parameter->label);
    this->available_parameters->add(parameter);
    return parameter;
}

FLASHMEM void ParameterManager::addParameters(LinkedList<FloatParameter*> *parameters) {
    if (parameters==nullptr) 
        return;
    Debug_println(F("ParameterManager#addParameters()..")); Serial_flush();
    Debug_printf(F("\t\tpassed @%p, has size %i\n"), parameters, parameters->size()); Serial_flush();
    for (unsigned int i = 0 ; i < parameters->size() ; i++) {
        Debug_printf(F("\t%i: adding from @%p '%s'\n"), i, parameters->get(i), parameters->get(i)->label); Serial_flush();
        //this->available_parameters->add(parameters->get(i));
        this->addParameter(parameters->get(i));
        Debug_printf(F("..added\n")); Serial_flush();
    }
    Debug_println(F("finished in addParameters"));
}


#ifdef ENABLE_SCREEN
    #include "menuitems_quickpage.h"

    FLASHMEM void ParameterManager::addAllParameterInputMenuItems(Menu *menu, bool page_per_input) {
        const char *last_group_name = nullptr;

        menu->add_page("QuickJumpInputs");
        CustomQuickPagesMenuItem *quickjump = new CustomQuickPagesMenuItem("QuickJump to Inputs");
        menu->add(quickjump);
        page_t *started_page = menu->get_selected_page();   // for remembering what page the quickjump menu itself is

        // add the behaviours quickjump page to the 'main' menu quickjump list
        menu->remember_opened_page(menu->get_page_index_for_name(menu->get_selected_page()->title));

        for (unsigned int i = 0 ; i < available_inputs->size() ; i++) {
            BaseParameterInput *parameter_input = available_inputs->get(i);
            //Serial.printf("!!! Adding parameter menu items for %s\tfrom %s!\n", parameter_input->name, parameter_input->group_name);
            char label[MENU_C_MAX];
            if (last_group_name!=parameter_input->group_name || page_per_input) {                        
                if (page_per_input)
                    snprintf(label, MENU_C_MAX, "%s: %s", parameter_input->group_name, parameter_input->name);
                else
                    snprintf(label, MENU_C_MAX, "%s inputs", parameter_input->group_name);
                menu->add_page(label, parameter_input->colour);
                last_group_name = parameter_input->group_name;
                snprintf(label, MENU_C_MAX, "%s ", last_group_name);

                // add page to behaviour quickjump, so long as isn't itself
                if (started_page!=menu->get_selected_page())
                    quickjump->add_page(menu->get_selected_page());
            }
            //this->addParameterInputMenuItems(menu, parameter_input, label); //label_prefix);
            parameter_input->makeControls(this->memory_size, label);
        }
    }

    #include "mymenu_items/ParameterInputViewCombined.h"

    // TODO: this currently has to be done after the parameterinputdisplay controls have already been generated -- modify so that it does not rely on this!
    FLASHMEM void ParameterManager::addAllParameterInputOverviews(Menu *menu) {
        const char *last_group_name = nullptr;

        menu->add_page("QuickJumpOverview");
        CustomQuickPagesMenuItem *quickjump = new CustomQuickPagesMenuItem("QuickJump to Overviews");
        menu->add(quickjump);
        page_t *started_page = menu->get_selected_page();   // for remembering what page the quickjump menu itself is

        menu->remember_opened_page(menu->get_page_index_for_name(menu->get_selected_page()->title));

        ParameterInputCombinedDisplay *combined_display;

        for (unsigned int i = 0 ; i < available_inputs->size() ; i++) {
            BaseParameterInput *parameter_input = available_inputs->get(i);
            //Serial.printf("!!! Adding parameter menu items for %s\tfrom %s!\n", parameter_input->name, parameter_input->group_name);
            char label[MENU_C_MAX];
            if (last_group_name!=parameter_input->group_name) {                        
                snprintf(label, MENU_C_MAX, "%s Overview", parameter_input->group_name);
                menu->add_page(label, parameter_input->colour);
                last_group_name = parameter_input->group_name;
                //snprintf(label, MENU_C_MAX, "%s ", last_group_name);

                combined_display = new ParameterInputCombinedDisplay(label, menu->tft->height() - 20);
                menu->add(combined_display);

                // add page to behaviour quickjump, so long as isn't itself
                if (started_page!=menu->get_selected_page())
                    quickjump->add_page(menu->get_selected_page());
            }

            combined_display->add_parameter_input_display(parameter_input->parameter_input_display);
        }

    }
#endif