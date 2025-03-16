/*#include "parameters/Parameter.h"
#include "ParameterManager.h"

extern ParameterManager parameter_manager;

void FloatParameter::set_slot_input(byte slot, char parameter_input_name) {
    this->set_slot_input(
        slot,
        parameter_manager.getInputForName(parameter_input_name)
    );
}*/

#include <String>

#include "parameters/Parameter.h"
#include "ParameterManager.h"

class BaseParameterInput;

extern ParameterManager *parameter_manager;

void FloatParameter::set_slot_input(int8_t slot, const char *name) {
    BaseParameterInput *inp = parameter_manager->getInputForName(name);
    if (inp!=nullptr)
        this->set_slot_input(slot, inp);
    else
        Debug_printf("WARNING: set_slot_input couldn't find a ParameterInput for name '%s'\n", name);
}

#ifdef ENABLE_SCREEN
    #include "mymenu_items/ParameterMenuItems.h"
#endif

void FloatParameter::set_slot_input(int8_t slot, BaseParameterInput *parameter_input) {
    Debug_printf(F("PARAMETERS\tFloatParameter#set_slot_input in '%s': asked to set slot %i on %s to point to %s\n"), this->label, slot, this->label, parameter_input->name);
    if (!this->is_valid_slot(slot)) {
        if (Serial) Serial.printf("%s#set_slot_input(%i) isn't a valid slot number!\n", this->label, slot);
        return;
    }
    if (parameter_input==nullptr) {
        if (Serial) Serial.printf("%s#set_slot_input(%i) was passed a nullptr!\n", this->label, slot);
    }
    this->connections[slot].parameter_input = parameter_input;
    /*#ifdef ENABLE_SCREEN
        if (parameter_input!=nullptr) {
            //Serial.println("calling update_slot_amount_control.."); Serial_flush();
            this->update_slot_amount_control(slot, parameter_input);
        }
    #endif*/
}

#ifdef ENABLE_SCREEN
    // TODO: this is kinda ugly!  should be a better way to do this.
    /*void FloatParameter::update_slot_amount_control(int8_t slot, BaseParameterInput *parameter_input) {
        Debug_println(F("in update_slot_amount_control (FloatParameter version)..")); Serial_flush();

        if (!is_valid_slot(slot)) return;
        
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
    }*/

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

const char *FloatParameter::get_input_name_for_slot(int8_t slot) {
    if (!is_valid_slot(slot)) return "[invalid]";
    if (this->connections[slot].parameter_input!=nullptr)
        return this->connections[slot].parameter_input->name;
    Debug_printf(F("WARNING: get_input_name_for_slot(%i) got an empty slot!"), slot);
    return "None";
}

float FloatParameter::get_amount_for_slot(int8_t slot) {
    if (!is_valid_slot(slot)) return 0.0f;
    return this->connections[slot].amount;
}

// get the lines required to save the state of this parameter mapping to a file
void FloatParameter::save_pattern_add_lines(LinkedList<String> *lines) {
    // save parameter base values (save normalised value; let's hope that this is precise enough to restore from!)
    String line = String("parameter_value_") + String(this->label) + "=" + String(this->getCurrentNormalValue());
    lines->add(line);
    //Serial.printf("PARAMETERS\t%s:\save_pattern_add_lines saving line:\t%s\n", this->label, line.c_str());

    if (this->is_modulatable()) {
        #define MAX_SAVELINE 255
        char line[MAX_SAVELINE];
        // todo: move handling of this into the Parameter class, or into a third class that can handle saving to different formats..?
        //          ^^ this could be the SaveableParameter class, used as a wrapper.  would require SaveableParameter to be able to add multiple lines to the save file
        // todo: make these mappings part of an extra type of thing (like a "preset clip"?), rather than associated with sequence?
        // todo: move these to be saved with the project instead?
        for (uint_fast8_t slot = 0 ; slot < MAX_SLOT_CONNECTIONS ; slot++) { // TODO: MAX_CONNECTION_SLOTS...?
            if (this->connections[slot].parameter_input==nullptr) continue;      // skip if no parameter_input configured in this slot
            if (this->connections[slot].amount==0.00) continue;                     // skip if no amount configured for this slot

            const char *input_name = this->get_input_name_for_slot(slot);

            // sequence save line looks like: `parameter_Filter Cutoff_0=A|1.000`
            //                                 ^^head ^^_^^param name^_slot=ParameterInputName|Amount
            snprintf(line, MAX_SAVELINE, "parameter_%s_%i=%s|%3.3f|%s", 
                this->label, 
                slot, 
                input_name,
                //'A'+slot, //TODO: implement proper saving of mapping! /*parameter->get_connection_slot_name(slot), */
                //parameter->connections[slot].parameter_input->name,
                this->connections[slot].amount,
                this->connections[slot].polar_mode==UNIPOLAR ? "unipolar" : "bipolar"
            );
            if (debug) Serial.printf("PARAMETERS\t%s#save_pattern_add_lines saving line:\t%s\n", this->label, line);
            lines->add(String(line));
        }
    }
}

// parse a key+value pair to restore the state 
bool FloatParameter::load_parse_key_value(const String incoming_key, String value) {
    if (debug) Serial.printf("PARAMETERS\tFloatParameter#load_parse_key_value passed '%s' => '%s'...\n", incoming_key.c_str(), value.c_str());

    const char *prefix = "parameter_";
    const char *prefix_base = "parameter_value_";
    const char separator = '_', subseparator = '|';

    String key = String(incoming_key.c_str());

    if (key.startsWith(prefix_base)) {
        //key.replace(prefix_base,"");
        //key = key.substring(strlen(prefix_base));
        if (key.substring(strlen(prefix_base)).equals(this->label)) {
            if (debug) Serial.printf("NOTICE: Matched key '%s' with '%s' - setting parameter value from '%s' - returning\n", key.c_str(), this->label, value.c_str());
            this->updateValueFromNormal(value.toFloat());
            return true;
        }
        //Serial.printf("WARNING: got a %s with value %s, but found no matching Parameter!\n", prefix_base, key.c_str(), value.c_str());
        return false;
    }

    // sequence save line looks like: `parameter_Filter Cutoff_0=A|1.000`
    //                                 ^^head ^^_^^param name^_slot=ParameterInputName|Amount
    if (!key.startsWith(prefix)) {
        return false;
    }
    key.replace(prefix, "");
    //key = key.substring(strlen(prefix));
    const uint_fast8_t separator_1_position = key.indexOf(separator);

    if (separator_1_position<0) {
        //if (Serial) Serial.printf("WARNING: in %s,\t didn't find separator_1 to split in key '%s', garbled line with value '%s'?", this->label, incoming_key.c_str(), value.c_str());
        return false;
    }

    const String parameter_name = key.substring(0, separator_1_position);

    if (parameter_name.equals(this->label)) {
        if (debug) Serial.printf("NOTICE: Matched key '%s' with prefix '%s' and parameter_name '%s' - returning\n", key.c_str(), prefix, this->label);

        String next_segment = key.substring(separator_1_position+1);
        const uint_fast8_t slot_number = next_segment.toInt();

        if (!is_valid_slot(slot_number)) {
            //if (Serial) Serial.printf("ERROR: in %s,\t got invalid slot number from '%s=%s'\n", this->label, incoming_key.c_str(), value.c_str());
            return false;
        }

        const uint_fast8_t separator_2_position = value.indexOf(subseparator);
        if (separator_2_position<0) {
            //if (Serial) Serial.printf("WARNING: in %s,\t didn't find separator_2 to split in key '%s', garbled line with value '%s'?", this->label, incoming_key.c_str(), value.c_str());
            return false;
        }

        float amount;
        int mode = -1; //UNIPOLAR;
        const String input_name = value.substring(0, separator_2_position);

        // get the amount of modulation
        const uint_fast8_t separator_3_position = value.lastIndexOf(subseparator);
        if (separator_3_position==separator_2_position) {
            // old version for compatibility -- no polarity mode in the input!
            // just get the amount
            amount = value.substring(separator_2_position+1).toFloat();
        } else {
            // new version -- polarity mode in the input!
            amount = value.substring(separator_2_position+1, separator_3_position).toFloat();
            mode = value.substring(separator_3_position+1).equals("unipolar") ? UNIPOLAR : BIPOLAR;
        }

        if (Serial) Serial.printf("NOTICE: in %s,\t Got split string '%s', slot_number %i, modulation amount %3.3f and mode %s\n", this->label, input_name.c_str(), slot_number, amount, mode == -1 ? "unset" :  (mode==UNIPOLAR ? "unipolar" : "bipolar"));

        this->set_slot_input (slot_number, input_name.c_str());
        this->set_slot_amount(slot_number, amount);
        if (mode>=0)
            this->set_slot_polarity(slot_number, mode);

        return true;
    }

    //Serial.printf(F("PARAMETERS\tWARNING: Couldn't find a Parameter for name %s\n"), parameter_name.c_str());
    return false;
}

#include "parameters/CVOutputParameter.h"
#ifdef ENABLE_SCREEN
    #include "menu_messages.h"
#endif

void ICalibratable::start_calibration() {
    parameter_manager->queue_calibration(this);
}