#ifndef PARAMETER_INPUT_MENUITEMS__INCLUDED
#define PARAMETER_INPUT_MENUITEMS__INCLUDED

#include "menuitems.h"

#include "parameter_inputs/ParameterInput.h"
#include "ParameterManager.h"

extern ParameterManager *parameter_manager;
class VoltageParameterInput;

// TODO: allow to deselect a selector (ie set None)

// Selector to choose a ParameterInput from the available list to use a Source; 
// Also used more directly by objects/parameters that can only feed from one ParameterInput at a time, eg CVInput
template<class TargetClass>
class ParameterInputSelectorControl : public SelectorControl<int> {
    BaseParameterInput *initial_selected_parameter_input = nullptr;
    LinkedList<BaseParameterInput*> *available_parameter_inputs = nullptr;

    TargetClass *target_object = nullptr;
    TargetClass **proxy_target_object = &target_object;

    void(TargetClass::*setter_func)(BaseParameterInput*);
    BaseParameterInput*(TargetClass::*getter_func)();

    bool show_values = false;   // whether to display the incoming values or not

    public:

    ParameterInputSelectorControl(
        const char *label, 
        TargetClass *target_object, 
        void(TargetClass::*setter_func)(BaseParameterInput*), 
        BaseParameterInput*(TargetClass::*getter_func)(), 
        LinkedList<BaseParameterInput*> *available_parameter_inputs,
        BaseParameterInput *initial_parameter_input = nullptr,
        bool show_values = false
    ) : SelectorControl(label, 0) {
        this->show_values = show_values;
        this->initial_selected_parameter_input = initial_parameter_input;
        this->available_parameter_inputs = available_parameter_inputs;
        this->target_object = target_object;
        this->setter_func = setter_func;
        this->getter_func = getter_func;
        this->num_values = available_parameter_inputs->size() + 1;  // + 1 for None optino .. 
    };
    ParameterInputSelectorControl(
        const char *label, 
        TargetClass **proxy_target_object, 
        void(TargetClass::*setter_func)(BaseParameterInput*), 
        BaseParameterInput*(TargetClass::*getter_func)(),
        LinkedList<BaseParameterInput*> *available_parameter_inputs,
        BaseParameterInput *initial_parameter_input = nullptr,
        bool show_values = false
    ) : ParameterInputSelectorControl(label, *proxy_target_object, setter_func, getter_func, available_parameter_inputs, initial_parameter_input, show_values) {
        this->proxy_target_object = proxy_target_object;
    };

    virtual void configure (LinkedList<BaseParameterInput*> *available_parameter_inputs) {
        this->available_parameter_inputs = available_parameter_inputs;
        char *initial_name = (char*)"None";
        if (this->initial_selected_parameter_input!=nullptr)
            initial_name = this->initial_selected_parameter_input->name;
        actual_value_index = this->find_parameter_input_index_for_label(initial_name);
    }

    virtual int find_parameter_input_index_for_label(char *name) {
        if (strcmp(name, "None")==0)
            return num_values;
        if (this->available_parameter_inputs==nullptr)
            return -1;
        const uint_fast16_t size = this->available_parameter_inputs->size();
        for (uint_fast16_t i = 0 ; i < size ; i++) {
            if (available_parameter_inputs->get(i)->matches_label(name))
                return i;
        }
        return -1;
    }

    virtual int find_parameter_input_index_for_object(BaseParameterInput *input) {
        return this->find_parameter_input_index_for_label(input->name);
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
            //this->actual_value_index = parameter_manager->getInputIndexForName(initial_selected_parameter_input->name); ////this->find_parameter_input_index_for_label(initial_selected_parameter_input->name);
            this->actual_value_index = this->find_parameter_input_index_for_label(initial_selected_parameter_input->name);
        } else {
            this->actual_value_index = -1;
        }
        this->selected_value_index = this->actual_value_index;
        //Serial.printf(F("#on_add returning"));
    }

    virtual const char *get_label_for_index(int index) {
        static char label_for_index[MENU_C_MAX];
        // todo: this is currently unused + untested
        if (index<0 || index>=(int)available_parameter_inputs->size())
            return "None";
        snprintf(label_for_index, MENU_C_MAX, "%s", this->available_parameter_inputs->get(index)->name);
        return label_for_index;
    }

    // update the control to reflect changes to selection (eg, called when new value is loaded from project file)
    /*virtual void update_source(BaseParameterInput *new_source) {
        //int index = parameter_manager->getPitchInputIndex(new_source);
        //Serial.printf("update_source got index %i\n", index);
        if (new_source==nullptr) {
            this->update_actual_index(-1);
        } else {
            int index = this->find_parameter_input_index_for_object(new_source);
            this->update_actual_index(index);
        }
    }*/

    virtual void setter (int new_value) {
        //if (this->debug) Serial.printf(F("ParameterSelectorControl changing from %i to %i\n"), this->actual_value_index, new_value);
        selected_value_index = actual_value_index = new_value;
        if(new_value>=0 && (*this->proxy_target_object)!=nullptr && this->setter_func!=nullptr) {
            if (new_value < (int)this->available_parameter_inputs->size())
                ((*this->proxy_target_object)->*this->setter_func)(this->available_parameter_inputs->get(new_value));
            else
                ((*this->proxy_target_object)->*this->setter_func)(nullptr);
        }
    }
    virtual int getter () {
        return selected_value_index;
    }

    BaseParameterInput *last_object = nullptr;

    // classic fixed display version
    virtual int display(Coord pos, bool selected, bool opened) override {
        //Serial.println(F("ParameterInputSelectorControl display()!")); Serial_flush();
        tft->setTextSize(0);

        pos.y = header(label, pos, selected, opened);
      
        int num_values = this->available_parameter_inputs->size(); //NUM_AVAILABLE_PARAMETERS;
        //Serial.printf(F("\tdisplay got num_values %i\n"), num_values); Serial_flush();

        if (!opened) {
            // not selected, so just show the current value on one row
            //Serial.printf("\tnot opened\n"); Serial_flush();
            BaseParameterInput *currently_active = (*this->proxy_target_object->*getter_func)(); //this->available_parameter_inputs->get(actual_value_index);
            if (currently_active!=nullptr) {
                colours(selected, currently_active->colour, BLACK);

                tft->printf("Selected: %s\n", currently_active->name);
            } else {
                tft->printf((char*)"Selected: none\n");
            }
        } else {
            // opened, so show the possible values to select from
            const int current_value = actual_value_index; //this->getter();
            if (selected_value_index==-1) 
                selected_value_index = actual_value_index;
            const int start_value = tft->will_x_rows_fit_to_height(selected_value_index) ? 0 : selected_value_index;
            
            int i = 0;
            for (/*int */i = start_value ; i < (int)num_values ; i++) {
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
            const bool is_current_value_selected = i==current_value;
            const int col = is_current_value_selected ? GREEN : C_WHITE;
            colours(opened && selected_value_index==i, col, BLACK);
            tft->println((const char*)"None");

            if (tft->getCursorX()>0) // if we haven't wrapped onto next line then do it manually
                tft->println(); //(char*)"\n");
        }
        //Serial.println(F("ParameterInputSelectorControl display() returning!")); Serial_flush();
        return tft->getCursorY();
    }

    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        const int index_to_display = opened ? selected_value_index : actual_value_index;
        BaseParameterInput *currently_active = 
            opened ? this->available_parameter_inputs->get(index_to_display) : (*this->proxy_target_object->*getter_func)();
        const int col = selected_value_index==this->actual_value_index && opened ? 
                GREEN : 
                currently_active!=nullptr ? 
                    currently_active->colour : 
                    YELLOW/2;
        
        colours(selected, col, BLACK);
        char txt[MENU_C_MAX];
        if (currently_active!=nullptr)
            // todo: sprintf to correct number of max_character_width characters
            snprintf(txt, MENU_C_MAX, "%6s", currently_active->name);
        else
            snprintf(txt, MENU_C_MAX, "None");
        tft->setTextSize((strlen(txt) < max_character_width/2) ? 2 : 1);
        tft->println(txt);
        return tft->getCursorY();
    }

    virtual bool button_select() override {
        //Serial.printf("button_select with selected_value_index %i\n", selected_value_index);
        //Serial.printf("that is available_values[%i] of %i\n", selected_value_index, available_values[selected_value_index]);
        this->setter(selected_value_index);

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        const char *name = selected_value_index>=0 && selected_value_index < (int)this->available_parameter_inputs->size() 
                            ? 
                            this->available_parameter_inputs->get(selected_value_index)->name 
                            : 
                            "None";
        //if (selected_value_index>=0)
        snprintf(msg, MENU_MESSAGE_MAX, set_message, label, name, selected_value_index);
        //Serial.printf("about to set_last_message!");
        //msg[20] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);

        return go_back_on_select;
    }

};

#endif