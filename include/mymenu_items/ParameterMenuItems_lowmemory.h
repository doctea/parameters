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
class ParameterMenuItemSelector : public ObjectSelectorControl<ParameterMenuItemSelectorTarget,ParameterMenuItem*> {
    public:
    LinkedList<FloatParameter*> parameters = nullptr;

    ParameterMenuItem *actual_controls = nullptr;

    int selected_item = -1;
    bool opened = false;

    ParameterMenuItemSelector(char *label, LinkedList<FloatParameter*> parameters) : MenuItem(label) {
        this->selected_item = 0;
        this->actual_controls = new ParameterMenuItem(label, parameters->get(0));
        this->parameters = parameters;
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
            snprintf(last_label, MAX_LABEL, "%s", last_object->label());
        }

        return last_label;
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        if (!this->opened) {
            // display the name of the currently selected parameter
            // todo: show a summary of the existing mapping information..
            //          maybe could just even display the underlying control to achieve this?
            tft->setTextSize(0);

            colours(selected, this->default_fg, BLACK);
            tft->setTextSize(2);

            if (this->actual_value_index>=0 && this->actual_value_index < num_values) {
                //Serial.printf(F("\tactual value index %i\n"), this->actual_value_index); Serial_flush();
                tft->printf((char*)"Selected: %s\n", (char*)this->get_label_for_index(this->actual_value_index));
                //Serial.printf(F("\tdrew selected %i\n"), this->actual_value_index); Serial_flush();
            } else {
                tft->printf((char*)"Selected: none\n");
            }
            return tft->getCursorY();
        } else {
            // display the actual parameter...
            return this->actual_controls->display(pos, selected, opened);
        }
    }

    virtual bool knob_left() override {
        if (!opened) {
            // not opened, so we're scrolling on the menu
            bool v = ObjectSelectorControl::knob_left();
            this->actual_controls->setParameter(this->parameters->get(this->selected_item));
            return v;
        } else {
            // opened, so pass through to the controls
            return actual_controls->knob_left();  
        }
    }
    virtual bool knob_right() override {
        if (!opened) {
            // not opened, so we're scrolling on the menu
            bool v = ObjectSelectorControl::knob_right();
            this->actual_controls->setParameter(this->parameters->get(this->selected_item));
            return v;
        } else {
            // opened, so pass through to the controls
            return actual_controls->knob_right();  
        }
    }
    virtual bool button_select() override {
        if (!opened) {
            opened = true;
        } else {
            return actual_controls->button_select();
        }
    }
    virtual bool button_back() override {
        if (!opened) {
            return true;
        } else {
            if (actual_controls->button_select())
                return true;
            else
                return false;
        }
    }

};

#endif