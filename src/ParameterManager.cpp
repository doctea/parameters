/*#include "ParameterManager.h"

#ifdef ENABLE_SCREEN

    #include "parameter_inputs/ParameterInput.h"
    #include "mymenu_items/ParameterInputViewMenuItems.h"

    void ParameterManager::addAllParameterInputMenuItems(Menu *menu) {
        Serial.println("ParameterManager#addAllParameterInputMenuItems()...");
        for (int i = 0 ; i < this->available_inputs.size() ; i++) {
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