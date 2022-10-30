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

void DoubleParameter::set_slot_input(byte slot, char *name) {
    BaseParameterInput *inp = parameter_manager->getInputForName(name);
    if (inp!=nullptr)
        this->set_slot_input(slot, inp);
    else
        Serial.printf("WARNING: set_slot_input couldn't find a ParameterInput for name '%s'\n", name);
}

#ifdef ENABLE_SCREEN
    #include "mymenu_items/ParameterMenuItems.h"
#endif

void DoubleParameter::set_slot_input(byte slot, BaseParameterInput *parameter_input) {
    Serial.printf(F("PARAMETERS\tDoubleParameter#set_slot_input in '%s': asked to set slot %i to point to %s\n"), this->label, slot, parameter_input->name);
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
    // TODO: this is kinda ugly!  should be a better way to do this.
    void DoubleParameter::update_slot_amount_control(byte slot, BaseParameterInput *parameter_input) {
        //Serial.println("calling update_slot_amount_control (parameter_input version).."); Serial.flush();
        //this->update_slot_amount_control(slot, parameter_input->name);
        if (this->connections[slot].amount_control!=nullptr) {
            if (parameter_input!=nullptr) {
                char new_label[7];
                sprintf(new_label, "%s", parameter_input->name); //"Amt %s"
                this->connections[slot].amount_control->update_label(new_label);
                this->connections[slot].amount_control->set_default_colours(parameter_input->colour);
            } else {
                this->connections[slot].amount_control->update_label((char*)"None");
                this->connections[slot].amount_control->set_default_colours(C_WHITE);   // todo: need a GREY colour for disabled items?
            }            
        }
        if (this->connections[slot].input_control!=nullptr) {
            this->connections[slot].input_control->update_actual_index(parameter_manager->getInputIndex(parameter_input));
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

char *DoubleParameter::get_input_name_for_slot(byte slot) {
    if (this->connections[slot].parameter_input!=nullptr)
        return this->connections[slot].parameter_input->name;
    Serial.printf("WARNING: get_input_name_for_slot(%i) got an empty slot!", slot);
    return (char*)"None";
}

double DoubleParameter::get_amount_for_slot(byte slot) {
    return this->connections[slot].amount;
}