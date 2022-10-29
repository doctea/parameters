#include "menuitems.h"

#include "parameter_inputs/ParameterInput.h"
#include "ParameterManager.h"

extern ParameterManager *parameter_manager;
class VoltageParameterInput;

// Selector to choose a ParameterInput from the available list; used by objects/parameters that can only feed from one ParameterInput at a time, eg CVInput
template<class TargetClass>
class ParameterInputSelectorControl : public SelectorControl {
    int actual_value_index = -1;
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
        char initial_name = ' ';
        if (this->initial_selected_parameter_input!=nullptr)
            initial_name = this->initial_selected_parameter_input->name;
        actual_value_index = parameter_manager->getInputIndexForName(initial_name);
                //this->find_parameter_input_index_for_label(initial_name);
    }

    virtual int find_parameter_input_index_for_label(char name) {
        //Serial.printf(F("find_parameter_input_index_for_label(%c) has available_parameter_inputs @%p\n"), name, available_parameter_inputs); Serial.flush();
        int size = available_parameter_inputs->size();
        for (int i = 0 ; i < size ; i++) {
            //Serial.printf(F("find_parameter_input_index_for_label(%c) looping over '%c'\n"), name, available_parameter_inputs->get(i)->name); Serial.flush();
            if (available_parameter_inputs->get(i)->name==name)
                return i;
        }
        Serial.printf(F("WARNING: find_parameter_index_for_label: didn't find one for '%c'?\n"), name); Serial.flush();
        return -1;
    }

    virtual void on_add() override {
        //this->debug = true;
        //Serial.printf(F("%s#on_add...\n"), this->label); Serial.flush();
        actual_value_index = -1;
        /*if (this->debug) {
            Serial.printf("on_add() in ParameterSelectorControl @%p:\n", this); Serial.flush();
            Serial.printf("\tParameterSelectorControl with initial_selected_parameter @%p...\n", initial_selected_parameter_input); Serial.flush();
            if (initial_selected_parameter_input!=nullptr) {
                Serial.printf("\tParameterSelectorControl looking for '%s' @%p...\n", initial_selected_parameter_input->label, initial_selected_parameter_input); Serial.flush();
            } else 
                Serial.println("\tno initial_selected_parameter set");
        }*/

        if (initial_selected_parameter_input!=nullptr) {
            //Serial.printf(F("%s#on_add: got non-null initial_selected_parameter_input\n")); Serial.flush();
            //Serial.printf(F("\tand its name is %c\n"), initial_selected_parameter_input->name); Serial.flush();
            this->actual_value_index = parameter_manager->getInputIndexForName(initial_selected_parameter_input->name); ////this->find_parameter_input_index_for_label(initial_selected_parameter_input->name);
        } else {
            this->actual_value_index = -1;
        }
        this->selected_value_index = this->actual_value_index;
        Serial.printf(F("#on_add returning"));
    }

    virtual const char *get_label_for_index(int index) {
        static char label_for_index[MENU_C_MAX];
        // todo: this is currently unused + untested
        if (index<0)
            return "None";
        sprintf( label_for_index, "%c", this->available_parameter_inputs->get(index)->name);
        return label_for_index;
    }

    virtual void setter (int new_value) {
        //if (this->debug) Serial.printf(F("ParameterSelectorControl changing from %i to %i\n"), this->actual_value_index, new_value);
        actual_value_index = new_value;
        selected_value_index = actual_value_index;
        //if (this->parameter_input!=nullptr) {
        if(new_value>=0) {
            //this->parameter_input->setTarget(this->available_parameter_inputs->get(new_value));
            // todo: proper checking that the input is able to provide a voltage/pitch -- ie it is a VoltageParameterInput*
            //          currently works because the only subclasses of BaseParameterInput that we are using are VoltageParameterInputs, 
            //          but this won't hold true if/when we start adding other input types
            (this->target_object->*this->setter_func)(this->available_parameter_inputs->get(new_value));
            /*if (this->control_to_update!=nullptr) {
                char new_label[10];
                sprintf(new_label, "Amt %c", this->available_parameter_inputs->get(actual_value_index)->name);
                Serial.printf("setting value to %i, new_label '%s', on control already labelled '%s'!\n", actual_value_index, new_label, this->control_to_update->label);
                strcpy(this->control_to_update->label, new_label);
            } else {
                Serial.printf("setting value to %i, but no control_to_update set!\n", actual_value_index);
            }*/
        }
    }
    virtual int getter () {
        return selected_value_index;
    }

    // TODO: implement this!!
    /*virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        Serial.printf("ParameterInputMenuItems#renderValue()");
        return 10;
    }*/

    // classic fixed display version
    virtual int display(Coord pos, bool selected, bool opened) override {
        //Serial.println(F("ParameterInputSelectorControl display()!")); Serial.flush();
        tft->setTextSize(0);

        pos.y = header(label, pos, selected, opened);
        
        num_values = this->available_parameter_inputs->size(); //NUM_AVAILABLE_PARAMETERS;
        //Serial.printf(F("\tdisplay got num_values %i\n"), num_values); Serial.flush();
        //tft->setTextSize(1);

        if (!opened) {
            // not selected, so just show the current value
            //Serial.printf("\tnot opened\n"); Serial.flush();

            colours(selected, this->default_fg, BLACK);

            if (this->actual_value_index>=0) {
                //Serial.printf(F("\tactual value index %i\n"), this->actual_value_index); Serial.flush();
                tft->printf((char*)"Selected: %c\n", this->available_parameter_inputs->get(this->actual_value_index)->name);
                //Serial.printf(F("\tdrew selected %i\n"), this->actual_value_index); Serial.flush();
            } else {
                tft->printf((char*)"Selected: none\n");
            }
        } else {
            // selected, so show the possible values to select from
            int current_value = actual_value_index; //this->getter();

            if (selected_value_index==-1) selected_value_index = actual_value_index;

            int start_value = 0;
            if (!tft->will_x_rows_fit_to_height(selected_value_index)) {
                start_value = selected_value_index;
                //Serial.printf(F("\n| setting start_value to %i for selected_value_index %i: "), start_value, selected_value_index);
            } else {
                //Serial.printf(F("\n| keeping start_value to %i for selected_value_index %i: "), start_value, selected_value_index);
            }

            //int actual_count = 0;
            for (int i = start_value ; i < num_values ; i++) {
                bool is_current_value_selected = i==current_value;
                int col = is_current_value_selected ? GREEN : this->default_fg;
                colours(opened && selected_value_index==i, col, BLACK);
                //Serial.printf("\tactual_count=%i, i=%i, name=%s, invert=%i, cursorY=%i\n", actual_count, i, get_label_for_index(i), opened && selected_value_index==i, tft->getCursorY());
                //tft->printf((char*)"%s\n", (char*)get_label_for_index(i));
                //tft->printf((char*)get_label_for_index(i));
                tft->printf("%c\n", this->available_parameter_inputs->get(i)->name);
                if (tft->getCursorY()>tft->height()) break;
                //tft->println((char*)get_label_for_index(i));
                //tft->setTextColor(BLACK,BLACK);
            }
            if (tft->getCursorX()>0) // if we haven't wrapped onto next line then do it manually
                tft->println((char*)"\n");
        }
        return tft->getCursorY();
    }

    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        /*char label[MAX_LABEL_LENGTH];
        //strcpy(label, get_label_for_value(available_values[opened ? selected_value_index : this->getter()]));
        if (strlen(label) > max_character_width) {
            label[max_character_width] = '\0';
        }
        tft->printf("%s", label);*/
        //Serial.printf("ParameterInputMenuItems#renderValue() in %s\n", this->label);
        int col = selected_value_index==this->actual_value_index ? GREEN : this->default_fg;

        if (opened || (selected_value_index>=0 && selected_value_index==this->actual_value_index))
            col = this->available_parameter_inputs->get(selected_value_index)->colour;

        //colours(opened && selected_value_index==this->actual_value_index, col, BLACK);
        colours(selected, col, BLACK);
        char txt[MENU_C_MAX];
        if (selected_value_index>=0)
            sprintf(txt,"%3c", this->available_parameter_inputs->get(selected_value_index)->name);
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
        sprintf(msg, "Set %s to %c (%i)", label, this->available_parameter_inputs->get(selected_value_index)->name, selected_value_index);
        //Serial.printf("about to set_last_message!");
        msg[20] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);

        return go_back_on_select;
    }

};
