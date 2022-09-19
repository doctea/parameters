/*    #ifdef ENABLE_SCREEN   
        #include "parameter_inputs/AnalogParameterInputBase.h"
        #include "mymenu_items/ParameterMenuItems.h"
        #include "mymenu_items/ParameterInputViewMenuItems.h"

        virtual void AnalogParameterInputBase::addMenuItems(Menu *menu) {
            char label[20];
            sprintf(label, "Graph for %c", this->name);

            Serial.printf("\tdoing menu->add for ParameterInputDisplay with label '%s'\n", label);
            menu->add(new ParameterInputDisplay(label, this)); //, LOOP_LENGTH_TICKS));
            sprintf(label, "Input type for %c", this->name);
            menu->add(new InputTypeSelectorControl(label, this, set_input_type, get_input_type));
            sprintf(label, "Output type for %c", this->name);
            menu->add(new InputTypeSelectorControl(label, this, set_output_type, get_output_type));
        }
    #endif
    */