/*#include "parameters/Parameter.h"
#include "ParameterManager.h"

extern ParameterManager parameter_manager;

void FloatParameter::set_slot_input(byte slot, char parameter_input_name) {
    this->set_slot_input(
        slot,
        parameter_manager.getInputForName(parameter_input_name)
    );
}*/

#include "parameters/Parameter.h"
#include "ParameterManager.h"

class BaseParameterInput;

extern ParameterManager *parameter_manager;

void FloatParameter::set_slot_input(byte slot, const char *name) {
    BaseParameterInput *inp = parameter_manager->getInputForName(name);
    if (inp!=nullptr)
        this->set_slot_input(slot, inp);
    else
        Debug_printf("WARNING: set_slot_input couldn't find a ParameterInput for name '%s'\n", name);
}

#ifdef ENABLE_SCREEN
    #include "mymenu_items/ParameterMenuItems.h"
#endif

void FloatParameter::set_slot_input(byte slot, BaseParameterInput *parameter_input) {
    Debug_printf(F("PARAMETERS\tFloatParameter#set_slot_input in '%s': asked to set slot %i on %s to point to %s\n"), this->label, slot, this->label, parameter_input->name);
    this->connections[slot].parameter_input = parameter_input;
    #ifdef ENABLE_SCREEN
        if (parameter_input!=nullptr) {
            //Serial.println("calling update_slot_amount_control.."); Serial_flush();
            this->update_slot_amount_control(slot, parameter_input);
        }
    #endif
}

#ifdef ENABLE_SCREEN
    // TODO: this is kinda ugly!  should be a better way to do this.
    void FloatParameter::update_slot_amount_control(byte slot, BaseParameterInput *parameter_input) {
        Debug_println(F("in update_slot_amount_control (FloatParameter version)..")); Serial_flush();
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

    /*void FloatParameter::update_slot_amount_control(byte slot, char name) {
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

const char *FloatParameter::get_input_name_for_slot(byte slot) {
    if (this->connections[slot].parameter_input!=nullptr)
        return this->connections[slot].parameter_input->name;
    Debug_printf(F("WARNING: get_input_name_for_slot(%i) got an empty slot!"), slot);
    return "None";
}

float FloatParameter::get_amount_for_slot(byte slot) {
    return this->connections[slot].amount;
}

// get the lines required to save the state of this parameter mapping to a file
void FloatParameter::save_sequence_add_lines(LinkedList<String> *lines) {
    // save parameter base values (save normalised value; let's hope that this is precise enough to restore from!)
    lines->add(String("parameter_value_") + String(this->label) + "=" + String(this->getCurrentNormalValue()));

    if (this->is_modulatable()) {
        #define MAX_SAVELINE 255
        char line[MAX_SAVELINE];
        // todo: move handling of this into the Parameter class, or into a third class that can handle saving to different formats..?
        //          ^^ this could be the SaveableParameter class, used as a wrapper.  would require SaveableParameter to be able to add multiple lines to the save file
        // todo: make these mappings part of an extra type of thing (like a "preset clip"?), rather than associated with sequence?
        // todo: move these to be saved with the project instead?
        for (int slot = 0 ; slot < 3 ; slot++) { // TODO: MAX_CONNECTION_SLOTS...?
            if (this->connections[slot].parameter_input==nullptr) continue;      // skip if no parameter_input configured in this slot
            if (this->connections[slot].amount==0.00) continue;                     // skip if no amount configured for this slot

            const char *input_name = this->get_input_name_for_slot(slot);

            // sequence save line looks like: `parameter_Filter Cutoff_0=A|1.000`
            //                                 ^^head ^^_^^param name^_slot=ParameterInputName|Amount
            snprintf(line, MAX_SAVELINE, "parameter_%s_%i=%s|%3.3f", 
                this->label, 
                slot, 
                input_name,
                //'A'+slot, //TODO: implement proper saving of mapping! /*parameter->get_connection_slot_name(slot), */
                //parameter->connections[slot].parameter_input->name,
                this->connections[slot].amount
            );
            Debug_printf(F("PARAMETERS\t%s: save_sequence_add_lines saving line:\t%s\n"), line);
            lines->add(String(line));
        }
    }
}

// parse a key+value pair to restore the state 
bool FloatParameter::load_parse_key_value(String key, String value) {
    const char *prefix = "parameter_";
    const char *prefix_base = "parameter_value_";
    const char separator = '|';

    if (key.startsWith(prefix_base)) {
        key = key.replace(prefix_base,"");
        if (key.equals(this->label)) {
            this->updateValueFromNormal(value.toFloat());
            return true;
        }
        //Serial.printf("WARNING: got a %s%s with value %s, but found no matching Parameter!\n", prefix_base, key.c_str(), value.c_str());
        return false;
    }

    // sequence save line looks like: `parameter_Filter Cutoff_0=A|1.000`
    //                                 ^^head ^^_^^param name^_slot=ParameterInputName|Amount
    if (!key.startsWith(prefix))
        return false;
        
    key = key.replace(prefix, "");
    String parameter_name = key.substring(0, key.indexOf('_'));

    if (parameter_name.equals(this->label)) {
        int slot_number = key.substring(key.indexOf('_')+1).toInt();
        String input_name = value.substring(0, value.indexOf(separator));
        float amount = value.substring(value.indexOf(separator)+1).toFloat();

        this->set_slot_input(slot_number, input_name.c_str());
        this->set_slot_amount(slot_number, amount);

        return true;
    }

    //Serial.printf(F("PARAMETERS\tWARNING: Couldn't find a Parameter for name %s\n"), parameter_name.c_str());
    return false;
}