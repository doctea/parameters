#ifndef MENU_PARAMETER_MENUITEMS__INCLUDED
#define MENU_PARAMETER_MENUITEMS__INCLUDED

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

#include "../parameters/Parameter.h"

#include <LinkedList.h>

#include "ParameterMenuItems.h"


class ParameterConnectionPolarityTypeSelectorControl : public SelectorControl<int_least8_t> {
    FloatParameter **parameter;
    byte slot_number;

    public:

    ParameterConnectionPolarityTypeSelectorControl(const char *label, FloatParameter **parameter, byte slot_number) 
        : SelectorControl(label) { 
        this->parameter = parameter;
        this->slot_number = slot_number;
        this->actual_value_index = (*this->parameter)->connections[slot_number].polar_mode;
        this->go_back_on_select = true;
    }

    virtual const char* get_label_for_value(int_least8_t index) override {
        if (index==BIPOLAR)
            return "Bipolar";
        if (index==UNIPOLAR)
            return "Unipolar";
        //if (index==CLOCK_NONE)
        //    return "None";
        return "??";
    }

    virtual void setter (int_least8_t new_value) {
        (*this->parameter)->connections[slot_number].polar_mode = new_value;
        actual_value_index = new_value;
    }
    virtual int_least8_t getter () {
        //return clock_mode; //selected_value_index;
        return (*this->parameter)->connections[slot_number].polar_mode;
    }

    virtual int renderValue(bool selected, bool opened, uint16_t width) override {
        tft->setTextColor((*this->parameter)->connections[slot_number].parameter_input->colour, BLACK);
        return SelectorControl::renderValue(selected, opened, width);
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        //Serial.println("MidiOutputSelectorControl display()!");

        //int textSize = tft->get_textsize_for_width(label, tft->width()/2);
        int_fast8_t textSize = 0;
        pos.y = header(label, pos, selected, opened, textSize);
        //tft->setTextSize(2);

        num_values = 2; //NUM_CLOCK_SOURCES;

        tft->setCursor(pos.x, pos.y);
        tft->setTextColor((*this->parameter)->connections[slot_number].parameter_input->colour, BLACK);

        if (!opened) {
            // not opened, so just show the current value
            //colours(opened && selected_value_index==i, col, BLACK);
            colours(selected, this->default_fg, BLACK);

            tft->printf((char*)"%s", (char*)get_label_for_value(this->getter())); //selected_value_index));
            tft->println((char*)"");
        } else {
            const int current_value = this->getter(); 

            for (int i = 0 ; i < num_values ; i++) {
                bool is_current_value_selected = (int)i==current_value;
                const int_fast16_t col = is_current_value_selected ? GREEN : this->default_fg;
                colours(selected_value_index==i, col, BLACK);
                //colours((selected /*&& !opened*/) || (opened && selected_value_index==(int)i), col, BLACK);
                tft->setCursor(pos.x, pos.y);
                const char *label = get_label_for_value(i);
                tft->setTextSize(tft->get_textsize_for_width(label, tft->width()/2));
                tft->printf((char*)"%s\n", (char*)label);
                pos.y = tft->getCursorY();
            }
            if (tft->getCursorX()>0) // if we haven't wrapped onto next line then do it manually
                tft->println((char*)"");
        }
        return tft->getCursorY();
    }

    virtual bool action_opened() override {
        //if (this->debug) Serial.printf(F("ObjectToggleControl#action_opened on %s\n"), this->label);
        bool value = !this->getter();
        //this->internal_value = !this->internal_value;

        this->setter(value); //(bool)this->internal_value);
        return false;   // don't 'open'
    }

    virtual bool button_select() override {
        this->selected_value_index = !this->selected_value_index;
        this->setter(selected_value_index);

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        snprintf(msg, MENU_MESSAGE_MAX, "Set polarity to %i: %s", selected_value_index, get_label_for_value(selected_value_index));
        //msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg, GREEN);

        return go_back_on_select;
    }

};


// couple of modes:-
//      unopened - display and select from parameter names
//      opened - edit the underlying parameter settings..

// class that allows user to select from a list of parameters, and edit that parameter.
class ParameterMenuItemSelector : public SelectorControl<int> { //public ObjectSelectorControl<ParameterMenuItemSelector,ParameterMenuItem*> {
    public:
    LinkedList<FloatParameter*> *parameters = nullptr;
    FloatParameter *parameter = nullptr;

    //int selected_item = -1;
    bool selecting = true;

    ParameterMenuItemSelector(char *label, LinkedList<FloatParameter*> *parameters) : 
        SelectorControl<int>(label) {
        this->selected_value_index = 0;
        this->parameter = parameters->get(0);
        this->actual_value_index = 0;
        this->parameters = parameters;
        this->num_values = parameters->size();
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
        colours(selected, this->default_fg, BLACK);
        tft->setTextSize(2);

        if (opened) tft->print(">>");
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

#endif