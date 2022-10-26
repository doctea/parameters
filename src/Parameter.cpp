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

void DoubleParameter::set_slot_input(byte slot, BaseParameterInput *parameter_input) {
    Serial.printf(F("DoubleParameter#set_slot_input in '%s': asked to set slot %i to point to %c\n"), this->label, slot, parameter_input->name);
    this->connections[slot].parameter_input = parameter_input;
}

char DoubleParameter::get_input_name_for_slot(byte slot) {
    if (this->connections[slot].parameter_input!=nullptr)
        return this->connections[slot].parameter_input->name;
    Serial.printf("WARNING: get_input_name_for_slot(%i) got an empty slot!", slot);
    return 0;
}