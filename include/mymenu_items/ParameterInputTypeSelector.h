#pragma once

#include "ParameterTypes.h"

template<class DataType=VALUE_TYPE>
class InputTypeSelectorControl : public SelectorControl<int_least16_t> {
    int actual_value_index;
    VALUE_TYPE *target = nullptr;

    public:

    InputTypeSelectorControl(const char *label, VALUE_TYPE *target) : SelectorControl(label) {
        this->target = target;
        actual_value_index = *target;
        this->selected_value_index = this->actual_value_index = this->getter();
        this->go_back_on_select = true;
    };

    virtual const char* get_label_for_value(int_least16_t index) override {
        if (index==BIPOLAR)
            return "Bipolar";
        if (index==UNIPOLAR)
            return "Unipolar";
        //if (index==CLOCK_NONE)
        //    return "None";
        return "??";
    }

    virtual void setter (int_least16_t new_value) {
        *target = (VALUE_TYPE)new_value;
        actual_value_index = new_value;
    }
    virtual int_least16_t getter () {
        //return clock_mode; //selected_value_index;
        return *target;
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        //Serial.println("MidiOutputSelectorControl display()!");

        //int textSize = tft->get_textsize_for_width(label, tft->width()/2);
        int_fast8_t textSize = 0;
        pos.y = header(label, pos, selected, opened, textSize);
        //tft->setTextSize(2);

        num_values = 2; //NUM_CLOCK_SOURCES;

        //tft->setTextSize(2);

        tft->setCursor(pos.x, pos.y);

        if (!opened) {
            // not opened, so just show the current value
            //colours(opened && selected_value_index==i, col, BLACK);
            colours(selected, this->default_fg, BLACK);

            tft->printf((char*)"%s", (char*)get_label_for_value(*target)); //selected_value_index));
            tft->println((char*)"");
        } else {
            const int current_value = *target; 

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
        snprintf(msg, MENU_MESSAGE_MAX, "Set type to %i: %s", selected_value_index, get_label_for_value(selected_value_index));
        //msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg, GREEN);

        return go_back_on_select;
    }

};
