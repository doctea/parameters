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
const char *FloatParameter::get_input_group_and_name_for_slot(int8_t slot) {
    if (!is_valid_slot(slot)) return "[invalid]";
    if (this->connections[slot].parameter_input!=nullptr)
        return this->connections[slot].parameter_input->get_group_and_name();
    Debug_printf(F("WARNING: get_input_group_and_name_for_slot(%i) got an empty slot!"), slot);
    return "None";
}

float FloatParameter::get_amount_for_slot(int8_t slot) {
    if (!is_valid_slot(slot)) return 0.0f;
    return this->connections[slot].amount;
}

#ifdef ENABLE_STORAGE
    // Compact single-object save/load for all modulation slots on a FloatParameter.
    // Format: modslots=inputname0,amount0,mode0;inputname1,amount1,mode1;...
    // Empty input name means unconnected. One allocation instead of 3*MAX_SLOT_CONNECTIONS.
    class ModulationSlotsSaveableSetting : public SaveableSettingBase {
        FloatParameter* target;
    public:
        ModulationSlotsSaveableSetting(FloatParameter* t) : target(t) {
            set_label("modslots");
            set_category("ModulationSlot");
        }
        const char* get_line() override {
            char* p = linebuf;
            char* end = linebuf + sizeof(linebuf);
            p += snprintf(p, end - p, "modslots=");
            for (uint_fast8_t s = 0; s < MAX_SLOT_CONNECTIONS && p < end; ++s) {
                const char* name = target->get_input_group_and_name_for_slot(s);
                float amount = target->get_amount_for_slot(s);
                const char* mode = (target->connections[s].polar_mode == BIPOLAR) ? "bipolar" : "unipolar";
                p += snprintf(p, end - p, "%s%s,%.6g,%s",
                    s == 0 ? "" : ";",
                    name ? name : "",
                    (double)amount,
                    mode);
            }
            return linebuf;
        }
        bool parse_key_value(const char* key, const char* value) override {
            if (strcmp(key, "modslots") != 0) return false;
            // parse semicolon-separated tuples
            char buf[128];
            strncpy(buf, value, sizeof(buf) - 1);
            buf[sizeof(buf)-1] = '\0';
            char* cursor = buf;
            for (uint_fast8_t s = 0; s < MAX_SLOT_CONNECTIONS; ++s) {
                char* semi = strchr(cursor, ';');
                if (semi) *semi = '\0';
                // parse "name,amount,mode"
                char* comma1 = strchr(cursor, ',');
                if (!comma1) break;
                *comma1 = '\0';
                char* comma2 = strchr(comma1 + 1, ',');
                float amount = 0.0f;
                bool bipolar = false;
                if (comma2) {
                    *comma2 = '\0';
                    amount = atof(comma1 + 1);
                    bipolar = (strcmp(comma2 + 1, "bipolar") == 0);
                }
                // set input
                if (cursor[0] != '\0') {
                    BaseParameterInput* inp = parameter_manager->getInputForGroupAndName(cursor);
                    target->set_slot_input(s, inp);
                }
                target->connections[s].amount = amount;
                target->connections[s].polar_mode = bipolar ? BIPOLAR : UNIPOLAR;
                if (!semi) break;
                cursor = semi + 1;
            }
            return true;
        }
    };

    void FloatParameter::setup_saveable_settings() {
        BaseParameter::setup_saveable_settings();

        register_setting(new ModulationSlotsSaveableSetting(this), SL_SCOPE_SCENE, false);
    }
#endif

#include "parameters/CVOutputParameter.h"
#ifdef ENABLE_SCREEN
    #include "menu_messages.h"
#endif

void ICalibratable::start_calibration() {
    parameter_manager->queue_calibration(this);
}