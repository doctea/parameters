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

void DoubleParameter::set_slot_input(byte slot, const char *name) {
    BaseParameterInput *inp = parameter_manager->getInputForName(name);
    if (inp!=nullptr)
        this->set_slot_input(slot, inp);
    else
        Debug_printf("WARNING: set_slot_input couldn't find a ParameterInput for name '%s'\n", name);
}

#ifdef ENABLE_SCREEN
    #include "mymenu_items/ParameterMenuItems.h"
#endif

void DoubleParameter::set_slot_input(byte slot, BaseParameterInput *parameter_input) {
    Debug_printf(F("PARAMETERS\tDoubleParameter#set_slot_input in '%s': asked to set slot %i on %s to point to %s\n"), this->label, slot, this->label, parameter_input->name);
    this->connections[slot].parameter_input = parameter_input;
    #ifdef ENABLE_SCREEN
        if (parameter_input!=nullptr) {
            Serial.println("calling update_slot_amount_control.."); Serial_flush();
            // todo: add colour, and update the colour of the widget too
            this->update_slot_amount_control(slot, parameter_input);
        }
    #endif
}

#ifdef ENABLE_SCREEN
    // TODO: this is kinda ugly!  should be a better way to do this.
    void DoubleParameter::update_slot_amount_control(byte slot, BaseParameterInput *parameter_input) {
        Debug_println(F("in update_slot_amount_control (DoubleParameter version)..")); Serial_flush();
        //this->update_slot_amount_control(slot, parameter_input->name);
        if (this->connections[slot].amount_control!=nullptr) {
            Debug_printf(F("Updating colours+label on slot %i on %s\n"), slot, this->connections[slot].amount_control->label);
            if (parameter_input!=nullptr) {
                char new_label[7];
                //sprintf(new_label, "%s", parameter_input->name); //"Amt %s"
                strncpy(new_label, parameter_input->name, 7);
                this->connections[slot].amount_control->update_label(new_label);
                this->connections[slot].amount_control->set_default_colours(parameter_input->colour);
            } else {
                this->connections[slot].amount_control->update_label("None");
                this->connections[slot].amount_control->set_default_colours(C_WHITE);   // todo: need a GREY colour for disabled items?
            }
        }
        if (this->connections[slot].input_control!=nullptr) {
            Debug_printf(F("and updating input_control '%s' actual_index to %i\n"), this->connections[slot].input_control->label, parameter_manager->getInputIndex(parameter_input));
            this->connections[slot].input_control->update_actual_index(parameter_manager->getInputIndex(parameter_input));
        }
    }

    /*void DoubleParameter::update_slot_amount_control(byte slot, char name) {
        //Serial.println("update_slot_amount_control creating label.."); Serial_flush();
        char new_label[7];
        sprintf(new_label, "Amt %c", name);
        //Serial.printf("got new label '%s'\n", new_label); Serial_flush();
        if (this->connections[slot].amount_control!=nullptr) {
            //Serial.println("calling update_label"); Serial_flush();
            this->connections[slot].amount_control->update_label(new_label);
            //this->connections[slot].amount_control->set_default_colours(parameter_input->colour);
        } 
    }*/
#endif

const char *DoubleParameter::get_input_name_for_slot(byte slot) {
    if (this->connections[slot].parameter_input!=nullptr)
        return this->connections[slot].parameter_input->name;
    Debug_printf(F("WARNING: get_input_name_for_slot(%i) got an empty slot!"), slot);
    return "None";
}

double DoubleParameter::get_amount_for_slot(byte slot) {
    return this->connections[slot].amount;
}