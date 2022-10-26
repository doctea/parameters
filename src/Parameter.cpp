/*#include "parameters/Parameter.h"
#include "ParameterManager.h"

extern ParameterManager parameter_manager;

void DoubleParameter::set_slot_input(byte slot, char parameter_input_name) {
    this->set_slot_input(
        slot,
        parameter_manager.getInputForName(parameter_input_name)
    );
}*/

#include "parameters/Parameter.h"
#include "ParameterManager.h"

class BaseParameterInput;

extern ParameterManager *parameter_manager;

void DoubleParameter::set_slot_input(byte slot, char name) {
    this->set_slot_input(slot, parameter_manager->getInputForName(name));
}

#ifdef ENABLE_SCREEN
    #include "mymenu_items/ParameterMenuItems.h"
#endif

void DoubleParameter::set_slot_input(byte slot, BaseParameterInput *parameter_input) {
    Serial.printf(F("PARAMETERS\tDoubleParameter#set_slot_input in '%s': asked to set slot %i to point to %c\n"), this->label, slot, parameter_input->name);
    this->connections[slot].parameter_input = parameter_input;
    #ifdef ENABLE_SCREEN
        if (parameter_input!=nullptr) {
            Serial.println("calling update_slot_amount_control.."); Serial.flush();
            // todo: add colour, and update the colour of the widget too
            this->update_slot_amount_control(slot, parameter_input);
        }
    #endif
}

#ifdef ENABLE_SCREEN
    void DoubleParameter::update_slot_amount_control(byte slot, BaseParameterInput *parameter_input) {
        //Serial.println("calling update_slot_amount_control (parameter_input version).."); Serial.flush();
        //this->update_slot_amount_control(slot, parameter_input->name);
        if (this->connections[slot].amount_control!=nullptr) {
            if (parameter_input!=nullptr) {
                char new_label[7];
                sprintf(new_label, "Amt %c", parameter_input->name);
                this->connections[slot].amount_control->update_label(new_label);
                this->connections[slot].amount_control->set_default_colours(parameter_input->colour);
            } else {
                this->connections[slot].amount_control->update_label("None");
                this->connections[slot].amount_control->set_default_colours(C_WHITE);   // todo: need a GREY colour for disabled items?
            }            
        }
    }

    /*void DoubleParameter::update_slot_amount_control(byte slot, char name) {
        //Serial.println("update_slot_amount_control creating label.."); Serial.flush();
        char new_label[7];
        sprintf(new_label, "Amt %c", name);
        //Serial.printf("got new label '%s'\n", new_label); Serial.flush();
        if (this->connections[slot].amount_control!=nullptr) {
            //Serial.println("calling update_label"); Serial.flush();
            this->connections[slot].amount_control->update_label(new_label);
            //this->connections[slot].amount_control->set_default_colours(parameter_input->colour);
        } 
    }*/
#endif

char DoubleParameter::get_input_name_for_slot(byte slot) {
    if (this->connections[slot].parameter_input!=nullptr)
        return this->connections[slot].parameter_input->name;
    Serial.printf("WARNING: get_input_name_for_slot(%i) got an empty slot!", slot);
    return 0;
}

double DoubleParameter::get_amount_for_slot(byte slot) {
    return this->connections[slot].amount;
}