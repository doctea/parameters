#ifndef MENU_PARAMETER_MENUITEMS__INCLUDED
#define MENU_PARAMETER_MENUITEMS__INCLUDED

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

#include "../parameters/Parameter.h"

#include <LinkedList.h>

// direct control over a Parameter from menu
class ParameterValueMenuItem : public DirectNumberControl<double> {
    public:
        DoubleParameter *parameter = nullptr;
        //Parameter<TargetClass, DataType> *parameter = nullptr;

        bool show_output_mode = false;  // true if this widget should show the last post-modulation output value; false if it should show the pre-modulation value

        ParameterValueMenuItem(char *label, DoubleParameter *parameter) : DirectNumberControl(label) {
            strcpy(this->label, label);
            this->parameter = parameter;
            this->internal_value = parameter->getCurrentNormalValue() * 100.0;
            this->minimum_value = 0.0f; 
            this->maximum_value = 1.0f; 
            //this->minimum_value = parameter->minimum_value;
            //this->maximum_value = parameter->maximum_value;
            //this->step = 0.01;
        }

        // // true if this widget should show the last post-modulation output value; false if it should show the pre-modulation value
        virtual ParameterValueMenuItem *set_show_output_mode(bool mode = true) {
            this->show_output_mode = mode;
            this->readOnly = true;
            return this;
        }

        virtual bool action_opened() override {
            //Serial.printf("ParameterValueMenuItem#action_opened in %s ", this->label);
            //Serial.printf("get_current_value() is %f\n", this->parameter->getCurrentValue());
            //this->internal_value = this->get_current_value() / 100.0; //->parameter->getCurrentValue() * this->maximum_value; //->getCurrentValue() * this->maximum_value;
            return true;
        }

        // normalised integer (0-100)
        virtual double get_current_value() override {
            if (this->parameter==nullptr)
                return 0;
            /*if (this->debug) {
                Serial.printf("ParameterValueMenuItem for %s (parameter %s) has currentValue ", this->label, this->parameter->label);
                Serial.println(parameter->getCurrentValue());
            }*/
            //return (int) (parameter->getCurrentNormalValue() * 100.0); //(double)this->maximum_value);    // turn into percentage
            return parameter->getCurrentNormalValue();
        }

        virtual const char *getFormattedValue() override {
            static char fmt[20] = "      ";
            if (this->show_output_mode) {
                return this->getFormattedOutputValue();
            }
            // todo: can probably skip a sprintf and return it directly from parameter->getFormattedValue()?             //return this->parameter->getFormattedValue();
            sprintf(fmt, "%-3s", this->parameter->getFormattedValue()); 
            return fmt;
        }
        virtual const char *getFormattedOutputValue() {
            static char fmt[20] = "      ";
            sprintf(fmt, "%s", this->parameter->getFormattedLastOutputValue());
            return fmt;
        }

        virtual const char *getFormattedInternalValue() override {
            return this->parameter->getFormattedValue(this->internal_value);
        }

        virtual const char *getFormattedExtra() override {
            return this->parameter->getFormattedValue(parameter->getLastModulatedNormalValue());
        }

        virtual void set_current_value(double value) override { 
            if (this->debug) { Serial.printf(F("ParameterValueMenuItem#set_current_value(%f) on %s\n"), value, this->label); Serial.flush(); }

            if (this->parameter==nullptr)
                return;
            if (this->readOnly)
                return;
           
            if (this->parameter!=nullptr) {
                if (this->debug) {
                    if (this->debug) Serial.printf(F("\tParameterMenuItem#set_current_value(%f): Calling setParamValue %f (max value %i) on Parameter %s\n"), value, value, this->maximum_value, this->parameter->label); Serial.flush();
                }
                //double v = (double)((double)value / (double)this->maximum_value);
                //double v = (double)((double)value/(double)this->maximum_value); // / (double)this->maximum_value); // * (double)this->maximum_value);
                double v = value;

                if (this->debug) {
                    if (this->debug) Serial.print(F("ParameterValueMenuItem#set_current_value() got v to pass: "));                    Serial.println(v);
                }
                //this->parameter->setParamValue(v);    // turn into percentage
                if (this->debug) Serial.printf(F("ParameterValueMenuItem#set_current_value(%f) about to call updateValueFromNormal(%f) (maximum_value is %i)\n"), value, v, this->maximum_value);
                this->parameter->updateValueFromNormal(v);
            } 
        }

        // directly increase the parameter's value
        virtual void increase_value() override {
            //this->debug = true;
            parameter->decrementValue();
            this->internal_value = parameter->getCurrentNormalValue(); //this->maximum_value;
            if (this->debug) Serial.printf(F("ParameterValueMenuItem#increase_value updated internal_value to %f (from %f * 100.0)\n"), internal_value, parameter->getCurrentNormalValue());
            //this->debug = false;
        }
        // directly decrease the parameter's value
        virtual void decrease_value() override {
            //this->debug = true;
            parameter->incrementValue();
            this->internal_value = parameter->getCurrentNormalValue(); // * 100.0; //this->maximum_value;
            if (this->debug) Serial.printf(F("ParameterValueMenuItem#decrease_value updated internal_value to %f (from %f * 100.0)\n"), internal_value, parameter->getCurrentNormalValue());
            //this->debug = false;
        }

        virtual bool knob_left() override {
            if (readOnly) return false;
            if (this->debug) Serial.printf(F("------ ParameterValueMenuItem#knob_left, internal_value=%f\n"), internal_value);
            increase_value();
            if (this->debug) Serial.printf(F("------ ParameterValueMenuItem#knob_left, about to call change_value(%f)\n"), internal_value);
            change_value(this->internal_value);
            if (this->debug) Serial.printf(F(">------<\n"));
            return true;
        }
        virtual bool knob_right() override {
            if (readOnly) return false;
            if (this->debug) Serial.printf(F("------ ParameterValueMenuItem#knob_right, internal_value=%f\n"), internal_value);
            decrease_value();
            if (this->debug) Serial.printf(F("------ ParameterValueMenuItem#knob_right, about to call change_value(%f)\n"), internal_value);
            change_value(this->internal_value);
            if (this->debug) Serial.printf(F(">------<\n"));
            return true;
        }
        virtual bool button_select() override {
            if (readOnly) return true;
            change_value(this->internal_value);
            return true;
        }

        virtual void change_value(int new_value) { //override { //
            float f = (float)new_value / 100.0;
            if (this->debug) Serial.printf(F("ParameterValueMenuItem#change_value(%i) about to call change_value(%f)\n"), new_value, new_value);
            this->change_value(f);
        }

        virtual void change_value(double new_value) {    // doesn't override, implements for normalled float?
            if (readOnly) return;
            double last_value = this->get_current_value();
            if (this->debug) Serial.printf(F("ParameterValueMenuItem#change_value(%f)\t in %s\tabout to call set_current_value(%f)\n"), new_value, this->label);
            this->set_current_value(new_value);
            if (this->debug) Serial.printf(F("ParameterValueMenuItem#change_value(%f)\t after set_current_value(%f) get_current_value is \n"), new_value, this->get_current_value());
            if (on_change_handler!=nullptr) {
                if (this->debug)  { Serial.println(F("NumberControl calling on_change_handler")); Serial.flush(); }
                on_change_handler(last_value, this->internal_value); //this->get_internal_value());
                if (this->debug)  { Serial.println(F("NumberControl after on_change_handler")); Serial.flush(); }
            }
        }
};


#include "submenuitem_bar.h"

// compound menu item that shows a direct value-setter widget, 3x modulation amount widgets, and the last post-modulation output value
class ParameterMenuItem : public SubMenuItemBar {
    public:

    DoubleParameter *parameter = nullptr;

    ParameterMenuItem(const char *label, DoubleParameter *parameter) : SubMenuItemBar(label) {
        this->parameter = parameter;

        // add the direct Value changer
        this->add(new ParameterValueMenuItem((char*)"Value", parameter));

        // add the modulation Amount % changers
        for (int i = 0 ; i < MAX_SLOT_CONNECTIONS ; i++) {
            // todo: make the modulation source part configurable too
            // todo: make the label part dynamically generated on-the-fly by the DirectNumberControl
            char labelnew[8];
            char input_name =   parameter->connections[i].parameter_input!=nullptr ? 
                                parameter->connections[i].parameter_input->name : 
                                'X';
            Serial.printf(F("\tfor %s, setting to parameter_input@%p '%c'\n"), label, parameter->connections[i].parameter_input, input_name);
            Serial.flush();
            sprintf(labelnew, "Amt %c", input_name);
            DirectNumberControl<double> *input_amount_control = new DirectNumberControl<double>(
                labelnew, 
                &parameter->connections[i].amount, 
                parameter->connections[i].amount, 
                -1.0, 
                1.0, 
                nullptr
            );
            //input_amount_control->default_fg = parameter->connections[i].parameter_input->colour;
            Serial.printf(F("ParameterMenuItem(%s) connection %i got colour %4x from '%c'!\n"), label, i, parameter->connections[i].parameter_input->colour, parameter->connections[i].parameter_input->name);
            //input_amount_control->debug = true;
            this->add(input_amount_control);
            input_amount_control->default_fg = parameter->connections[i].parameter_input->colour; // do this after add to avoid parent colours overwriting it?
        }

        // add another small widget to display the last output value (after modulation etc)
        ParameterValueMenuItem *output = new ParameterValueMenuItem((char*)"Output", parameter);
        output->set_show_output_mode();
        this->add(output); 
        
        /*this->add(this->ctrl_amt_1);
        this->add(this->ctrl_amt_2);
        this->add(this->ctrl_amt_3);*/
        //this->add(new SourceSelectorControl("S2", parameter));
        //this->add(new SourceSelectorControl("S3", parameter));
        //this->
    }
};


// deprecated version -- from when the ParameterInput was re-assignable to Parameter output, rather than the Parameter pulling from (potentially) multiple ParameterInputs
/*
class ParameterSelectorControl : public SelectorControl {
    int actual_value_index = -1;
    //void (*setter_func)(BaseParameter *midi_output);
    BaseParameterInput *parameter_input = nullptr;
    //void(BaseParameterInput::*setter_func)(BaseParameter *target_parameter);
    BaseParameter *initial_selected_parameter;

    LinkedList<DoubleParameter*> *available_parameters;

    bool show_values = false;   // whether to display the incoming values or not

    public:

    ParameterSelectorControl(const char *label, bool show_values = false) : SelectorControl(label, 0) {
        this->show_values = show_values;
    };

    virtual void configure (BaseParameterInput *parameter_input, LinkedList<DoubleParameter*> *available_parameters) { //}, void (*setter_func)(BaseParameter*)) {
        this->available_parameters = available_parameters;
        this->parameter_input = parameter_input;
        //this->setter_func = setter_func;
        this->initial_selected_parameter = this->parameter_input->target_parameter;
        if (this->initial_selected_parameter!=nullptr) {
            if (this->debug) Serial.printf("ParameterSelectorControl configured control labelled '%s' with initial_selected_parameter '%s'@%p from parameter_input @ %p\n", label, initial_selected_parameter->label, initial_selected_parameter, parameter_input);
            //Serial.printf("%u and %u\n", this->initial_selected_parameter, this->setter_func);
            actual_value_index = this->find_parameter_index_for_label(initial_selected_parameter->label);
            if (actual_value_index>=0) return;
        }
        actual_value_index = this->find_parameter_index_for_label((char*)"None");
    }

    virtual int find_parameter_index_for_label(char *label) {
        int size = available_parameters->size();
        for (int i = 0 ; i < size ; i++) {
            if (!strcmp(available_parameters->get(i)->label, label))
                return i;
        }
        Serial.printf("WARNING: find_parameter_index_for_label: didn't find one for '%s'?\n", label);
        return -1;
    }

    virtual void on_add() {
        actual_value_index = -1;
        if (this->debug) {
            Serial.printf("on_add() in ParameterSelectorControl @%p:\n", this); Serial.flush();
            Serial.printf("\tParameterSelectorControl with initial_selected_parameter @%p...\n", initial_selected_parameter); Serial.flush();
            if (initial_selected_parameter!=nullptr) {
                Serial.printf("\tParameterSelectorControl looking for '%s' @%p...\n", initial_selected_parameter->label, initial_selected_parameter); Serial.flush();
            } else 
                Serial.println("\tno initial_selected_parameter set");
        }

        if (initial_selected_parameter!=nullptr) 
            this->actual_value_index = this->find_parameter_index_for_label(initial_selected_parameter->label);
        else    
            this->actual_value_index = -1;
        this->selected_value_index = this->actual_value_index;
    }

    virtual const char* get_label_for_index(int index) {
        if (index<0)
            return "None";
        return this->available_parameters->get(index)->label;
    }

    virtual void setter (int new_value) {
        if (this->debug) Serial.printf("ParameterSelectorControl changing from %i to %i\n", this->actual_value_index, new_value);
        actual_value_index = new_value;
        selected_value_index = actual_value_index;
        if (this->parameter_input!=nullptr) {
            this->parameter_input->setTarget(this->available_parameters->get(new_value));
        }
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

            if (show_values) {
                tft->printf((char*)"Inp: %-15s\n", (char*)this->parameter_input->getInputInfo()); //i @ %p")
                tft->printf((char*)"Read: %-8s\n", (char*)this->parameter_input->getInputValue());
            }
            tft->printf((char*)"Tgt: %-15s\n", (char*)get_label_for_index(actual_value_index));
            if (show_values) {
                tft->printf((char*)"Val: %-7s\n",  (char*)this->parameter_input->getFormattedValue());
            }

            //tft->println((char*)"");
            //tft->printf("%i%\n", 100 * parameter_input->target_parameter->getCurrentValue());
            //Serial.printf("get currentvalue: %i\n", parameter_input->target_parameter->getCurrentValue());
            //Serial.printf("got formatted value in selector display: %s\n", parameter_input->getFormattedValue());
        } else {
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

            //int actual_count = 0;
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

    virtual bool button_select() override {
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
*/
#endif