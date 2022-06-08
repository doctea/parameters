#ifndef MENU_PARAMETER_MENUITEMS__INCLUDED
#define MENU_PARAMETER_MENUITEMS__INCLUDED

#include "mymenu.h"

#include "Parameter.h"
#include "AnalogParameterInput.h"
#include "DigitalParameterInput.h"

#include <LinkedList.h>

// direct control over a Parameter from menu
class ParameterMenuItem : public DirectNumberControl {
    BaseParameter *parameter = nullptr;

    double internal_value;
    
    int minimum_value = 0; //Parameter->minimum_value;
    int maximum_value = 100; //Parameter->maximum_value;

    public:
        ParameterMenuItem(char*in_label, BaseParameter*parameter) : DirectNumberControl(label) {
            strcpy(label, in_label);
            this->parameter = parameter;
            internal_value = parameter->getCurrentValue();
        }

        virtual int get_current_value() override {
            return parameter->getCurrentValue() * 100.0;    // turn into percentage
        }

        virtual void set_current_value(double value) { 
            if (!this->readOnly)
                parameter->setParamValue(value / 100.0);    // turn into percentage
        }

};


// ui to configure which Parameter a ParameterInput targets
// TODO: this is based on a tweaked version of the MIDIOutputSelectorControl from the usb_midi_clocker project, merge the two functionalities to make generic
class ParameterSelectorControl : public SelectorControl {
    int actual_value_index = -1;
    //void (*setter_func)(BaseParameter *midi_output);
    BaseParameterInput *parameter_input = nullptr;
    //void(BaseParameterInput::*setter_func)(BaseParameter *target_parameter);
    BaseParameter *initial_selected_parameter;

    LinkedList<BaseParameter*> *available_parameters;

    public:

    ParameterSelectorControl(const char *label) : SelectorControl(label, 0) {};
/*        SelectorControl(label, 0) {
        //strcpy(this->label, label);
        this->setter_func = setter_func;
        //this->initial_selected_parameter = initial_selected_parameter;
    }*/

    virtual void configure (BaseParameterInput *parameter_input, LinkedList<BaseParameter*> *available_parameters) { //}, void (*setter_func)(BaseParameter*)) {
        this->available_parameters = available_parameters;
        this->parameter_input = parameter_input;
        //this->setter_func = setter_func;
        this->initial_selected_parameter = this->parameter_input->target_parameter;
        Serial.printf("ParameterSelectorControl configured control labelled '%s' with initial_selected_parameter '%s'@%p from parameter_input @ %p\n", label, initial_selected_parameter->label, initial_selected_parameter, parameter_input);
        //Serial.printf("%u and %u\n", this->initial_selected_parameter, this->setter_func);
        actual_value_index = this->find_parameter_index_for_label(initial_selected_parameter->label);
    }

    virtual int find_parameter_index_for_label(char *label) {
        int size = available_parameters->size();
        for (int i = 0 ; i < size ; i++) {
            if (!strcmp(available_parameters->get(i)->label, label))
                return i;
        }
        Serial.printf("WARNING: find_parameter_index_for_label: didn't find one for %s?\n", label);
        return -1;
    }

    virtual void on_add() {
        actual_value_index = -1;
        Serial.println("on_add()");
        Serial.printf("\tParameterSelectorControl@ %u...\n", initial_selected_parameter);
        Serial.printf("\tParameterSelectorControl looking for '%s' at %p...\n", initial_selected_parameter->label, initial_selected_parameter);

        this->actual_value_index = this->find_parameter_index_for_label(initial_selected_parameter->label);
        this->selected_value_index = this->actual_value_index;
    }

    virtual const char* get_label_for_index(int index) {
        /*char label[20];
        strcpy(available_parameters.get(index)->label, label);
        return label;*/
        return this->available_parameters->get(index)->label;
    }

    virtual void setter (int new_value) {
        Serial.printf("ParameterSelectorControl changing from %i to %i\n", this->actual_value_index, new_value);
        actual_value_index = new_value;
        selected_value_index = actual_value_index;
        if (this->parameter_input!=nullptr) {
            this->parameter_input->setTarget(this->available_parameters->get(new_value));
        }
        /*if (this->setter_func!=nullptr) {
            Serial.printf("setting new output\n");
            this->setter_func(available_parameters.get(new_value));
        }*/
    }
    virtual int getter () {
        return selected_value_index;
    }

    // classic fixed display version
    virtual int display(Coord pos, bool selected, bool opened) override {
        //Serial.println("MidiOutputSelectorControl display()!");
        tft->setTextSize(0);

        pos.y = header(label, pos, selected, opened);
        
        num_values = this->available_parameters->size(); //NUM_AVAILABLE_PARAMETERS;

        //tft->setTextSize(1);

        if (!opened) {
            // not selected, so just show the current value
            colours(false, C_WHITE, BLACK);

            tft->printf((char*)"Inp: %s\n", (char*)parameter_input->getInputInfo()); //i @ %p")
            tft->printf((char*)"Tgt: %s\n", (char*)get_label_for_index(actual_value_index));
            //tft->println((char*)"");
            //tft->printf("%i%\n", 100 * parameter_input->target_parameter->getCurrentValue());
            tft->printf((char*)"Val: %s",   (char*)parameter_input->getFormattedValue());
            //Serial.printf("get currentvalue: %i\n", parameter_input->target_parameter->getCurrentValue());
            //Serial.printf("got formatted value in selector display: %s\n", parameter_input->getFormattedValue());
        } else {
            // calculate available viewport size
            /*int viewport_size = tft->height() - tft->getCursorY();
            if (num_values < viewport_size) viewport_size = num_values;
            Serial.printf("viewport_size is %i\n", viewport_size);
            int currentValue = actual_value_index; //this->getter();
            for (int i = 0 ; i < viewport_size ; i++) {
            }*/
                      
            // selected, so show the possible values to select from
            int current_value = actual_value_index; //this->getter();

            if (selected_value_index==-1) selected_value_index = actual_value_index;

            int start_value = 0;
            if (!tft->will_x_rows_fit_to_height(selected_value_index)) {
                start_value = selected_value_index;
                //Serial.printf("\n| setting start_value to %i for selected_value_index %i: ", start_value, selected_value_index);
            } else {
                //Serial.printf("\n| keeping start_value to %i for selected_value_index %i: ", start_value, selected_value_index);
            }
            Serial.printf("%s :: ", (char*)get_label_for_index(selected_value_index));
            Serial.print("\n");

            int actual_count = 0;
            for (int i = start_value ; i < num_values ; i++) {
                bool is_current_value_selected = i==current_value;
                int col = is_current_value_selected ? GREEN : C_WHITE;
                colours(opened && selected_value_index==i, col, BLACK);
                //Serial.printf("\tactual_count=%i, i=%i, name=%s, invert=%i, cursorY=%i\n", actual_count, i, get_label_for_index(i), opened && selected_value_index==i, tft->getCursorY());
                //tft->printf((char*)"%s\n", (char*)get_label_for_index(i));
                tft->printf((char*)get_label_for_index(i));
                if (tft->getCursorY()>tft->height()) break;
                //tft->println((char*)get_label_for_index(i));
                //tft->setTextColor(BLACK,BLACK);
            }
            if (tft->getCursorX()>0) // if we haven't wrapped onto next line then do it manually
                tft->println((char*)"\n");
        }
        return tft->getCursorY();
    }

    virtual bool button_select() {
        //Serial.printf("button_select with selected_value_index %i\n", selected_value_index);
        //Serial.printf("that is available_values[%i] of %i\n", selected_value_index, available_values[selected_value_index]);
        this->setter(selected_value_index);

        char msg[255];
        //Serial.printf("about to build msg string...\n");
        sprintf(msg, "Set %s to %s (%i)", label, get_label_for_index(selected_value_index), selected_value_index);
        //Serial.printf("about to set_last_message!");
        msg[20] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);
        return false;
    }

};

#endif