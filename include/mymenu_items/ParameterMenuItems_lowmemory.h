#pragma once

#include "Arduino.h"

#include <LinkedList.h>

#include "menu.h"
#include "colours.h"

#include "../parameters/Parameter.h"
#include "ParameterMenuItems.h"

// class that allows user to select from a list of parameters, and edit that parameter.  
// other controls can point their (FloatParameter**)parameter pointers here so that they pick up the currently-selected parameter's data
class ParameterMenuItemSelector : public SelectorControl<int_least16_t> { //public ObjectSelectorControl<ParameterMenuItemSelector,ParameterMenuItem*> {
    public:
    LinkedList<FloatParameter*> *parameters = nullptr;
    FloatParameter *parameter = nullptr;

    //int selected_item = -1;
    bool selecting = true;

    ParameterMenuItemSelector(const char *label, LinkedList<FloatParameter*> *parameters) : 
        SelectorControl<int_least16_t>(label) {
        this->selected_value_index = 0;
        this->parameter = parameters->get(0);
        this->actual_value_index = 0;
        this->parameters = parameters;
        this->num_values = parameters->size();
    }

    FloatParameter *last_object = nullptr;
    int_least16_t last_index = -1;
    char last_label[MENU_C_MAX];
    virtual const char *get_label_for_index(int_least16_t index) {
        if (index<0 || index >= (int_least16_t)this->parameters->size())
            return "None";

        if (last_index!=index) {
            last_object = this->parameters->get(index);
            last_index = index;
            snprintf(last_label, MENU_C_MAX, "%s", last_object->label);
        }

        //Serial.printf("got label for index %i: '%s'\n", index, last_label); Serial.flush();

        return last_label;
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        colours(selected, this->default_fg, BLACK);
        tft->setTextSize(2);
        //Serial.printf("ParameterMenuItemSelector parameter address is @%p (address of that pointer is @%p)\n", this->parameter, &this->parameter);

        if (opened) {
            colours(selected, GREEN, BLACK); 
            tft->print(">");
        }
        tft->printf((char*)"%s\n", (char*)this->get_label_for_index(this->selected_value_index));
        pos.y = tft->getCursorY();
        return pos.y;
    }

    virtual bool knob_left() override {
        // not opened, so we're scrolling on the menu
        bool v = SelectorControl::knob_left();
        parameter = this->parameters->get(this->selected_value_index);
        return v;
    }
    virtual bool knob_right() override {
        bool v = SelectorControl::knob_right();
        parameter = this->parameters->get(this->selected_value_index);
        return v;
    }

    virtual bool button_select() override {
        return SELECT_EXIT;
    }
    virtual bool button_back() override {
        return BACK_EXIT;
    }
};

#include "ParameterInputMenuItems.h"


/// controls for allowing the same parmeter menu items to be re-used on multiple pages

struct lowmemory_controls_t {
    // re-usable controls
    ParameterMenuItem *parameter_amount_controls;
    SubMenuItemBar *input_selectors_bar;
    SubMenuItemBar *polarity_submenu;
    SubMenuItemBar *range_submenu;
    // pointer to the current control being edited
    FloatParameter * parameter;
};
extern lowmemory_controls_t lowmemory_controls;

// "dummy" invisible control that switches single instances of editor controls 
// so that when this control is rendered, the correct parameter is swapped into be displayed/edited
// todo: make a version of this that doesn't allow require a ParameterMenuItem to do the switching, so can re-use
//       controls on eg the CV-to-MIDI pages where we want one set of editors per page dedicated to editing one parameter
class LowMemorySwitcherMenuItem : public MenuItem {
    public:
    LinkedList<FloatParameter*> *parameters = nullptr;
    ParameterMenuItemSelector *parameter_selector = nullptr;

    LowMemorySwitcherMenuItem(char *label, LinkedList<FloatParameter*> *parameters, ParameterMenuItemSelector *parameter_selector, int_fast16_t default_fg) 
        : MenuItem(label) {
            this->parameters = parameters;
            this->parameter_selector = parameter_selector;
            this->default_fg = default_fg;
            this->selectable = false;
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        //Serial.printf("switching lowmemory_controls.parameter pointer from @%p to @%p\n", lowmemory_controls.parameter, &this->parameter_selector->parameter);
        // dont actually display anything, just update the parameter_selector
        lowmemory_controls.parameter = this->parameter_selector->parameter;
        // todo: tell the dependent controls to update their internal values etc
        return pos.y;
    }
};

// create 'low-memory' controls for a list of parameters
// re-uses the same actual controls, inserting a dummy LowMemorySwitcherMenuItem so that the correct item is pointed at
void create_low_memory_parameter_controls(const char *label, LinkedList<FloatParameter*> *parameters, int_fast16_t default_fg = C_WHITE);