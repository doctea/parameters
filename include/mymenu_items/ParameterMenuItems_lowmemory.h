#ifndef MENU_PARAMETER_MENUITEMS__INCLUDED
#define MENU_PARAMETER_MENUITEMS__INCLUDED

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

#include "../parameters/Parameter.h"

#include <LinkedList.h>

#include "ParameterMenuItems.h"

// couple of modes:-
//      unopened - display and select from parameter names
//      opened - edit the underlying parameter settings..

// class that allows user to select from a list of parameters, and edit that parameter.
class ParameterMenuItemSelector : public SelectorControl<int> { //public ObjectSelectorControl<ParameterMenuItemSelector,ParameterMenuItem*> {
    public:
    LinkedList<FloatParameter*> *parameters = nullptr;

    ParameterMenuItem *actual_controls = nullptr;

    //int selected_item = -1;
    bool selecting = true;

    ParameterMenuItemSelector(char *label, LinkedList<FloatParameter*> *parameters) : 
        SelectorControl<int>(label) {
        this->selected_value_index = 0;
        this->actual_controls = new ParameterMenuItem(parameters->get(0)->label, parameters->get(0));
        this->actual_value_index = 0;
        this->parameters = parameters;
        this->num_values = parameters->size();
    }

    virtual void on_add() override {
        this->actual_controls->set_tft(this->tft);
        this->actual_controls->on_add();
        SelectorControl::on_add();
    }

    FloatParameter *last_object = nullptr;
    int last_index = -1;
    char last_label[MAX_LABEL];
    virtual const char *get_label_for_index(int index) {
        if (index<0 || index >= this->parameters->size())
            return "None";

        if (last_index!=index) {
            last_object = this->parameters->get(index);
            last_index = index;
            snprintf(last_label, MAX_LABEL, "%s", last_object->label);
        }

        //Serial.printf("got label for index %i: '%s'\n", index, last_label); Serial.flush();

        return last_label;
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        if (this->selecting) {
            // display the name of the currently selected parameter
            // todo: show a summary of the existing mapping information..
            //          maybe could just even display the underlying control to achieve this?
            tft->setTextSize(0);

            colours(selected, this->default_fg, BLACK);
            tft->setTextSize(2);

            if (opened)
                tft->print(">");

            if (this->selected_value_index>=0 && this->selected_value_index < num_values) {
                //Serial.printf("\tactual value index %i\n", this->actual_value_index); Serial_flush();
                //tft->printf((char*)"Selected: %i\n", this->selected_value_index);
                tft->printf((char*)"%s\n", (char*)this->get_label_for_index(this->selected_value_index));
                //Serial.printf("\tdrew selected %i\n", this->actual_value_index); Serial_flush();
            } else {
                tft->printf((char*)"Selected: none\n");
            }
            return tft->getCursorY();
        } else {
            // display the actual parameter...
            /*Serial.println("in OPENED mode!"); Serial.flush();
            tft->println("in OPENED mode..!");
            Serial.printf("about to call actual_controls->display() on %p\n", this->actual_controls); Serial_flush();*/
            //this->actual_controls->debug = true;
            //return tft->getCursorY();
            tft->printf((char*)"%s\n", (char*)this->get_label_for_index(this->selected_value_index));
            return this->actual_controls->display(pos, selected, opened);
        }
    }

    virtual bool knob_left() override {
        if (selecting) {
            // not opened, so we're scrolling on the menu
            bool v = SelectorControl::knob_left();
            this->actual_controls->setParameter(this->parameters->get(this->selected_value_index));
            return v;
        } else {
            // opened, so pass through to the controls
            return actual_controls->knob_left();  
        }
    }
    virtual bool knob_right() override {
        if (selecting) {
            // not opened, so we're scrolling on the menu
            bool v = SelectorControl::knob_right();
            this->actual_controls->setParameter(this->parameters->get(this->selected_value_index));
            return v;
        } else {
            // opened, so pass through to the controls
            return actual_controls->knob_right();  
        }
    }
    /*virtual bool action_opened() override {
        opened = true;
    }*/
    virtual bool button_select() override {
        if (selecting) {
            Serial.println("moving into EDIT mode"); Serial.flush();
            selecting = false;
            return false;
        } else {
            Serial.println("already in EDIT mode - so sending select to the controls!"); Serial.flush();
            Serial.printf("controls is %p\n", actual_controls); Serial.flush();
            Serial.printf("controls is %s\n", actual_controls->label); Serial.flush();
            //return false;
            if (actual_controls->button_select()) {
                selecting = true;
            }
            return false;
        }
    }
    virtual bool button_back() override {
        if (selecting) {
            return false;
        } else {
            if (!actual_controls->button_back()) {
                selecting = true;
            }
            return selecting;
        }
    }
    

};

#endif