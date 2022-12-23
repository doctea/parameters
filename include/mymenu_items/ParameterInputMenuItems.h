#include "menuitems.h"

#include "parameter_inputs/ParameterInput.h"
#include "ParameterManager.h"

extern ParameterManager *parameter_manager;
class VoltageParameterInput;

// Selector to choose a ParameterInput from the available list; used by objects/parameters that can only feed from one ParameterInput at a time, eg CVInput
template<class TargetClass>
class ParameterInputSelectorControl : public SelectorControl {
    //void (*setter_func)(BaseParameter *midi_output);
    //BaseParameterInput *parameter_input = nullptr;
    //void(BaseParameterInput::*setter_func)(BaseParameter *target_parameter);
    BaseParameterInput *initial_selected_parameter_input = nullptr;
    LinkedList<BaseParameterInput*> *available_parameter_inputs = nullptr;

    TargetClass *target_object = nullptr;
    void(TargetClass::*setter_func)(BaseParameterInput*);

    bool show_values = false;   // whether to display the incoming values or not

    //MenuItem* control_to_update = nullptr;

    public:

    ParameterInputSelectorControl(
        const char *label, 
        TargetClass *target_object, 
        void(TargetClass::*setter_func)(BaseParameterInput*), 
        LinkedList<BaseParameterInput*> *available_parameter_inputs,
        BaseParameterInput *initial_parameter_input = nullptr,
        //MenuItem* control_to_update = nullptr,
        bool show_values = false
    ) : SelectorControl(label, 0) {
        this->show_values = show_values;
        this->initial_selected_parameter_input = initial_parameter_input,
        this->available_parameter_inputs = available_parameter_inputs,
        this->target_object = target_object;
        this->setter_func = setter_func;
        //this->control_to_update = control_to_update;
        this->num_values = available_parameter_inputs->size();
    };

    virtual void configure (LinkedList<BaseParameterInput*> *available_parameter_inputs) {
        this->available_parameter_inputs = available_parameter_inputs;
        //this->initial_selected_parameter_input = this->parameter_input->target_parameter;
        /*if (this->initial_selected_parameter_input!=nullptr) {
            if (this->debug) Serial.printf("ParameterSelectorControl configured control labelled '%s' with initial_selected_parameter '%s'@%p from parameter_input @ %p\n", label, initial_selected_parameter_input->label, initial_selected_parameter_input, parameter_input);
            //Serial.printf("%u and %u\n", this->initial_selected_parameter, this->setter_func);
            actual_value_index = this->find_parameter_index_for_label(initial_selected_parameter_input->label);
            if (actual_value_index>=0) return;
        }*/
        char *initial_name = (char*)"None";
        if (this->initial_selected_parameter_input!=nullptr)
            initial_name = this->initial_selected_parameter_input->name;
        actual_value_index = parameter_manager->getInputIndexForName(initial_name);
                //this->find_parameter_input_index_for_label(initial_name);
    }

    virtual int find_parameter_input_index_for_label(char *name) {
        return parameter_manager->getInputIndexForName(name);
    }

    virtual void on_add() override {
        //this->debug = true;
        //Serial.printf(F("%s#on_add...\n"), this->label); Serial_flush();
        actual_value_index = -1;
        /*if (this->debug) {
            Serial.printf("on_add() in ParameterSelectorControl @%p:\n", this); Serial_flush();
            Serial.printf("\tParameterSelectorControl with initial_selected_parameter @%p...\n", initial_selected_parameter_input); Serial_flush();
            if (initial_selected_parameter_input!=nullptr) {
                Serial.printf("\tParameterSelectorControl looking for '%s' @%p...\n", initial_selected_parameter_input->label, initial_selected_parameter_input); Serial_flush();
            } else 
                Serial.println("\tno initial_selected_parameter set");
        }*/

        if (initial_selected_parameter_input!=nullptr) {
            //Serial.printf(F("%s#on_add: got non-null initial_selected_parameter_input\n")); Serial_flush();
            //Serial.printf(F("\tand its name is %c\n"), initial_selected_parameter_input->name); Serial_flush();
            this->actual_value_index = parameter_manager->getInputIndexForName(initial_selected_parameter_input->name); ////this->find_parameter_input_index_for_label(initial_selected_parameter_input->name);
        } else {
            this->actual_value_index = -1;
        }
        this->selected_value_index = this->actual_value_index;
        //Serial.printf(F("#on_add returning"));
    }

    virtual const char *get_label_for_index(int index) {
        static char label_for_index[MENU_C_MAX];
        // todo: this is currently unused + untested
        if (index<0)
            return "None";
        sprintf(label_for_index, "%s", this->available_parameter_inputs->get(index)->name);
        return label_for_index;
    }

    virtual void setter (int new_value) {
        //if (this->debug) Serial.printf(F("ParameterSelectorControl changing from %i to %i\n"), this->actual_value_index, new_value);
        actual_value_index = new_value;
        selected_value_index = actual_value_index;
        if(new_value>=0) {
            (this->target_object->*this->setter_func)(this->available_parameter_inputs->get(new_value));
        }
    }
    virtual int getter () {
        return selected_value_index;
    }

    // classic fixed display version
    virtual int display(Coord pos, bool selected, bool opened) override {
        //Serial.println(F("ParameterInputSelectorControl display()!")); Serial_flush();
        tft->setTextSize(0);

        pos.y = header(label, pos, selected, opened);
      
        num_values = this->available_parameter_inputs->size(); //NUM_AVAILABLE_PARAMETERS;
        //Serial.printf(F("\tdisplay got num_values %i\n"), num_values); Serial_flush();

        if (!opened) {
            // not selected, so just show the current value on one row
            //Serial.printf("\tnot opened\n"); Serial_flush();
            colours(selected, this->default_fg, BLACK);

            if (this->actual_value_index>=0) {
                //Serial.printf(F("\tactual value index %i\n"), this->actual_value_index); Serial_flush();
                tft->printf((char*)"Selected: %s\n", (char*)this->get_label_for_index(this->actual_value_index));
                //Serial.printf(F("\tdrew selected %i\n"), this->actual_value_index); Serial_flush();
            } else {
                tft->printf((char*)"Selected: none\n");
            }
        } else {
            // opened, so show the possible values to select from
            const int current_value = actual_value_index; //this->getter();
            if (selected_value_index==-1) 
                selected_value_index = actual_value_index;
            const int start_value = tft->will_x_rows_fit_to_height(selected_value_index) ? 0 : selected_value_index;
            
            for (int i = start_value ; i < (int)num_values ; i++) {
                //Serial.printf("%s#display() looping over parameterinput number %i of %i...\n", this->label, i, this->available_parameter_inputs->size()); Serial.flush();
                const BaseParameterInput *param_input = this->available_parameter_inputs->get(i);
                //Serial.printf("%s#display() got param_input %p...", param_input); Serial.flush();
                //Serial.printf("named %s\n", param_input->name); Serial.flush();
                const bool is_current_value_selected = i==current_value;
                const int col = is_current_value_selected ? GREEN : param_input->colour;
                colours(opened && selected_value_index==i, col, BLACK);

                //tft->printf("%s\n", (char*)param_input->name);
                tft->println((const char*)param_input->name);

                if (tft->getCursorY()>tft->height()) 
                    break;
            }
            if (tft->getCursorX()>0) // if we haven't wrapped onto next line then do it manually
                tft->println(); //(char*)"\n");
        }
        return tft->getCursorY();
    }

    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        const int index_to_display = opened ? selected_value_index : actual_value_index;
        const int col = selected_value_index==this->actual_value_index && opened ? 
                GREEN : 
                (index_to_display>=0 ? this->available_parameter_inputs->get(index_to_display)->colour : YELLOW/2);
        
        colours(selected, col, BLACK);
        char txt[MENU_C_MAX];
        if (index_to_display>=0)
            // todo: sprintf to correct number of max_character_width characters
            sprintf(txt,"%6s", this->available_parameter_inputs->get(index_to_display)->name);
        else
            sprintf(txt,"None");
        tft->setTextSize((strlen(txt) < max_character_width/2) ? 2 : 1);
        tft->println(txt);
        return tft->getCursorY();
    }

    virtual bool button_select() override {
        //Serial.printf("button_select with selected_value_index %i\n", selected_value_index);
        //Serial.printf("that is available_values[%i] of %i\n", selected_value_index, available_values[selected_value_index]);
        this->setter(selected_value_index);

        char msg[255];
        //Serial.printf("about to build msg string...\n");
        const char *name = selected_value_index>=0 ? this->available_parameter_inputs->get(selected_value_index)->name : "None";
        //if (selected_value_index>=0)
        sprintf(msg, "Set %s to %s (%i)", label, name, selected_value_index);
        //Serial.printf("about to set_last_message!");
        msg[20] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);

        return go_back_on_select;
    }

};
