#pragma once

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

#include "../parameters/Parameter.h"

#include <LinkedList.h>

// direct control over a Parameter Value from menu
class ParameterValueMenuItem : public DirectNumberControl<float> {
    public:
        FloatParameter **parameter = nullptr;
        //Parameter<TargetClass, DataType> *parameter = nullptr;

        bool show_output_mode = false;  // true if this widget should show the last post-modulation output value; false if it should show the pre-modulation value

        virtual char get_float_unit() override {
            if (this->parameter==nullptr || *parameter==nullptr) {
                return this->float_unit;
            } else {
                return (*parameter)->float_unit;
            }
        }

        ParameterValueMenuItem(char *label, FloatParameter **parameter) : DirectNumberControl(label) {
            strncpy(this->label, label, 20);
            this->parameter = parameter;
            if (*parameter!=nullptr)
                //this->internal_value = (*parameter)->getCurrentNormalValue() * 100.0;
                this->internal_value = (*parameter)->getLastModulatedNormalValue() * 100.0;
            this->minimumDataValue = 0.0f; 
            this->maximumDataValue = 1.0f; 

            //this->debug = true;

            go_back_on_select = true;
            //this->minimumDataValue = parameter->minimumDataValue;
            //this->maximumDataValue = parameter->maximumDataValue;
            //this->step = 0.01;
        }

        // // true if this widget should show the last post-modulation output value; false if it should show the pre-modulation value
        virtual ParameterValueMenuItem *set_show_output_mode(bool mode = true) {
            this->show_output_mode = mode;
            this->readOnly = true;
            return this;
        }

        virtual bool action_opened() override {
            // Serial.printf("ParameterValueMenuItem#action_opened in %s ", this->label);
            // Serial.printf("internal_value is %3.3f but get_current_value() is %3.3f\n", this->internal_value, (*this->parameter)->currentNormalValue);

            // update control's internal value from the parameter's current value when we open the menu, 
            // so that it shows the current value and not the last value used by this control 
            // (which may be different if the control was last pointing at a different parameter)
            // previously did battle with this but gave up trying to solve -- i think maybe because of an
            // issue with CV Output parameters?
            this->internal_value = (*parameter)->getCurrentNormalValue();
            return true;
        }

        // normalised integer (0-100)
        virtual float get_current_value() override {
            if (this->parameter==nullptr || *parameter==nullptr)
                return 0;
            /*if (this->debug) {
                Serial.printf("ParameterValueMenuItem for %s (parameter %s) has currentValue ", this->label, this->parameter->label);
                Serial.println(parameter->getCurrentValue());
            }*/
            //return (int) (parameter->getCurrentNormalValue() * 100.0); //(float)this->maximumDataValue);    // turn into percentage
            return (*parameter)->getCurrentNormalValue();
        }

        virtual const char *getFormattedValue() override {
            static char fmt[20] = "";
            if (this->show_output_mode) {
                return this->getFormattedOutputValue();
            }
            snprintf(fmt, 20, "%3s", (*parameter)->getFormattedValue()); 
            return fmt;
        }
        virtual const char *getFormattedOutputValue() {
            static char fmt[20] = "";
            //snprintf(fmt, 20, "%s", this->parameter->getFormattedLastOutputValue());
            snprintf(fmt, 20, "%s", (*parameter)->getFormattedLastOutputValue());
            if (this->debug) Serial.printf("%s#getFormattedOutputValue() returning\t'%s'\t\t(in ParameterValueMenuItem)\n", this->label, fmt);
            return fmt;
        }

        virtual const char *getFormattedInternalValue() override {
            return (*parameter)->getFormattedValue(this->get_internal_value()); //->internal_value);
        }

        virtual const char *getFormattedExtra() override {
            if ((*parameter)->is_modulatable())
                return (*parameter)->getFormattedValue((*parameter)->getLastModulatedNormalValue());
            return nullptr;
        }

        virtual void set_current_value(float value) override { 
            //if (this->debug) { Serial.printf(F("ParameterValueMenuItem#set_current_value(%f) on %s\n"), value, this->label); Serial_flush(); }

            if (parameter==nullptr || *parameter==nullptr)
                return;
            if (this->readOnly)
                return;
           
            if (*parameter!=nullptr) {
                /*if (this->debug) {
                    Serial.printf(F("\tParameterMenuItem#set_current_value(%f): Calling setParamValue %f (max value %i) on Parameter %s\n"), value, value, this->maximumDataValue, this->parameter->label); Serial_flush();
                }*/
                float v = value;
                /*if (this->debug) {
                    Serial.print(F("ParameterValueMenuItem#set_current_value() got v to pass: "));                    
                    Serial.println(v);
                }*/
                //if (this->debug) Serial.printf(F("ParameterValueMenuItem#set_current_value(%f) about to call updateValueFromNormal(%f) (maximumDataValue is %i)\n"), value, v, this->maximumDataValue);
                (*parameter)->updateValueFromNormal(v);
            } 
        }

        // directly increase the parameter's value
        virtual void increase_value() override {
            //this->debug = true; (*parameter)->debug = true;
            (*parameter)->incrementValue();
            this->internal_value = (*parameter)->getCurrentNormalValue(); //this->maximumDataValue;
            if (this->debug) Serial.printf((const char*)F("ParameterValueMenuItem#increase_value updated internal_value to %f (from %f * 100.0)\n"), internal_value, (*parameter)->getCurrentNormalValue());
            //this->debug = false; (*parameter)->debug = false;
        }
        // directly decrease the parameter's value
        virtual void decrease_value() override {
            //this->debug = true; (*parameter)->debug = true;

            (*parameter)->decrementValue();
            this->internal_value = (*parameter)->getCurrentNormalValue(); // * 100.0; //this->maximumDataValue;
            if (this->debug) Serial.printf((const char*)F("ParameterValueMenuItem#decrease_value updated internal_value to %f (from %f * 100.0)\n"), internal_value, (*parameter)->getCurrentNormalValue());
            //this->debug = false; (*parameter)->debug = false;
        }

        virtual void change_value(int new_value) { //override { //
            float f = (float)new_value / 100.0;
            //if (this->debug) Serial.printf(F("ParameterValueMenuItem#change_value(%i) about to call change_value(%f)\n"), new_value, new_value);
            this->change_value(f);
        }

        virtual void change_value(float new_value) {    // doesn't override, implements for normalled float?
            if (readOnly) return;
            float last_value = this->get_current_value();
            //if (this->debug) Serial.printf(F("ParameterValueMenuItem#change_value(%f)\t in %s\tabout to call set_current_value(%f)\n"), new_value, this->label);
            this->set_current_value(new_value);
            //if (this->debug) Serial.printf(F("ParameterValueMenuItem#change_value(%f)\t after set_current_value(%f) get_current_value is \n"), new_value, this->get_current_value());
            if (on_change_handler!=nullptr) {
                //if (this->debug)  { Serial.println(F("NumberControl calling on_change_handler")); Serial_flush(); }
                on_change_handler(last_value, this->internal_value); //this->get_internal_value());
                //if (this->debug)  { Serial.println(F("NumberControl after on_change_handler")); Serial_flush(); }
            }
        }
};

#include "submenuitem_bar.h"
#include "parameter_inputs/ParameterInput.h"
#include "ParameterInputMenuItems.h"
#include "ParameterMenuItems_Range.h"

class ParameterMapPercentageControl : public DirectNumberControl<float> {
    public:
    FloatParameter **parameter = nullptr;
    int slot_number;
    ParameterMapPercentageControl(char *label, FloatParameter **parameter, int slot_number) :
        DirectNumberControl<float> (
            label, 
            &((*parameter)->connections[slot_number].amount), 
            (*parameter)->connections[slot_number].amount,
            -1.0f,
            1.0f,
            nullptr
        ), parameter(parameter), slot_number(slot_number)
        {
            if ((*parameter)->connections[slot_number].parameter_input!=nullptr) {
                this->default_fg = (*parameter)->connections[slot_number].parameter_input->colour;
            }
        }

    virtual int header(const char *text, Coord pos, bool selected = false, bool opened = false, int textSize = 0) override {
        if ((*parameter)->connections[slot_number].parameter_input!=nullptr) {
            return DirectNumberControl::header((*parameter)->connections[slot_number].parameter_input->name, pos, selected, opened, textSize);
        } else {
            return DirectNumberControl::header(text, pos, selected, opened, textSize);
        }
    }

    virtual const char *get_label() {
        return (*parameter)->connections[slot_number].parameter_input!=nullptr ? 
               (*parameter)->connections[slot_number].parameter_input->name : 
               (char*)"None";
    }
       
    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        //Serial.printf("%s: renderValue has parameter=%p and *parameter=%p\n", get_label(), this->parameter, *this->parameter);
        // update the control with the colour from the connected parameter input, if there is one
        this->default_fg = (parameter!=nullptr && *parameter!=nullptr && (*parameter)->connections[slot_number].parameter_input!=nullptr)
                            ? (*parameter)->connections[slot_number].parameter_input->colour
                            : C_WHITE;

        if (opened) this->default_fg = GREEN;
        if (!(*parameter)->is_modulation_slot_active(slot_number))
            this->default_fg = tft->halfbright_565(this->default_fg);
            
        // todo: update label based on connected parameter input.. see thoughts on how best to do this in submenuitem_bar.h!
        //Serial.printf("renderValue in ParameterMapPercentageControl\tfor %s,\tgot default_fg colour %04x from slot_number %i colour %04x, max_character_width=%i\n", this->label, this->default_fg, slot_number, parameter->connections[slot_number].parameter_input->colour, max_character_width);
        return DirectNumberControl::renderValue(selected, opened, max_character_width);
    }

    virtual float get_current_value() override {
        return (*parameter)->connections[slot_number].amount;
    }

    virtual void set_current_value(float value) override {
        (*parameter)->connections[slot_number].amount = value;
    }

};


// ParameterMenuItem is defined below, after ParameterConnectionPolarityTypeSelectorControl and ParameterModSlotRow



// widget for editing the polarity of a parameter input's connection to parameter mixer
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
        if (index==MOD_SLOT_BI_NATIVE)
            return MOD_SLOT_LABEL_BI_NATIVE;
        if (index==MOD_SLOT_UNI_RAW)
            return MOD_SLOT_LABEL_UNI_RAW;
        if (index==MOD_SLOT_UNI_CENTERED)
            return MOD_SLOT_LABEL_UNI_CENTERED;
        //if (index==CLOCK_NONE)
        //    return "None";
        return "??";
    }

    virtual void setter (int_least8_t new_value) {
        if (this->parameter != nullptr)
            (*this->parameter)->connections[slot_number].polar_mode = new_value;
        actual_value_index = new_value;
    }
    virtual int_least8_t getter () {
        //return clock_mode; //selected_value_index;
        if (this->parameter != nullptr)
            return (*this->parameter)->connections[slot_number].polar_mode;
        return MOD_SLOT_UNI_RAW;
    }

    virtual int renderValue(bool selected, bool opened, uint16_t width) override {
        if (this->parameter != nullptr && (*this->parameter)->connections[slot_number].parameter_input != nullptr)
            this->default_fg = (*this->parameter)->connections[slot_number].parameter_input->colour;
        else
            this->default_fg = C_WHITE;
        return SelectorControl::renderValue(selected, opened, width);
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        //Serial.println("MidiOutputSelectorControl display()!");
        //int textSize = tft->get_textsize_for_width(label, tft->width()/2);
        int_fast8_t textSize = 0;
        pos.y = header(label, pos, selected, opened, textSize);
        //tft->setTextSize(2);

        num_values = 3; //NUM_CLOCK_SOURCES;

        tft->setCursor(pos.x, pos.y);
        //tft->setTextColor((*this->parameter)->connections[slot_number].parameter_input->colour, BLACK);

        if (!opened) {
            // not opened, so just show the current value
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
        const int_least8_t current = this->getter();
        const int_least8_t value = (int_least8_t)((current + 1) % 3);
        this->setter(value);
        return false;   // don't 'open'
    }

    virtual bool button_select() override {
        this->selected_value_index = (this->selected_value_index + 1) % 3;
        this->setter(selected_value_index);

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        snprintf(msg, MENU_MESSAGE_MAX, "Set polarity to %i: %s", selected_value_index, get_label_for_value(selected_value_index));
        menu_set_last_message(msg, GREEN);

        return go_back_on_select;
    }

};


// One row for a single modulation slot: [source selector | amount % | mode selector]
class ParameterModSlotRow : public SubMenuItemBarCustomProportions {
    FloatParameter **parameter;
    int slot_number;
public:
    ParameterModSlotRow(const char *label, FloatParameter **parameter, int slot_number)
        : SubMenuItemBarCustomProportions(label, 3, false, false),
          parameter(parameter), slot_number(slot_number)
    {
        // Column proportions: source=50%, amount=33%, mode=17%
        this->set_column_proportion(0, 0.50f);
        this->set_column_proportion(1, 0.33f);
        this->set_column_proportion(2, 0.17f);

        BaseParameterInput *initial_input = nullptr;
        if (parameter != nullptr && *parameter != nullptr)
            initial_input = (*parameter)->connections[slot_number].parameter_input;

        // Source selector — slot-specific setter/getter function pointers
        void(FloatParameter::*setter)(BaseParameterInput*) = nullptr;
        BaseParameterInput*(FloatParameter::*getter)() = nullptr;
        switch (slot_number) {
            case 0: setter = &FloatParameter::set_slot_0_input; getter = &FloatParameter::get_slot_0_input; break;
            case 1: setter = &FloatParameter::set_slot_1_input; getter = &FloatParameter::get_slot_1_input; break;
            default: setter = &FloatParameter::set_slot_2_input; getter = &FloatParameter::get_slot_2_input; break;
        }
        ParameterInputSelectorControl<FloatParameter> *source = new ParameterInputSelectorControl<FloatParameter>(
            "Src",
            parameter,
            setter, getter,
            parameter_manager->available_inputs,
            initial_input
        );
        source->go_back_on_select = true;
        this->add(source);

        // Amount control
        char amt_label[8];
        snprintf(amt_label, sizeof(amt_label), "Amt%i", slot_number + 1);
        ParameterMapPercentageControl *amount = new ParameterMapPercentageControl(amt_label, parameter, slot_number);
        this->add(amount);

        // Mode selector
        ParameterConnectionPolarityTypeSelectorControl *mode = new ParameterConnectionPolarityTypeSelectorControl(
            "Mode", parameter, slot_number
        );
        this->add(mode); 
    }

    virtual int small_display(int index, int x, int y, int width_in_pixels, bool is_selected, bool is_opened, bool outer_selected) override {
        int end_y = SubMenuItemBarCustomProportions::small_display(index, x, y, width_in_pixels, is_selected, is_opened, outer_selected);
        // if (is_opened) {
        //     MenuItem *ctrl = this->items->get(index);
        //     if (!this->show_sub_headers && ctrl != nullptr && !ctrl->wants_fullscreen_overlay_when_opened_in_bar()) {
        //         tft->drawFastHLine(x, y-2, width_in_pixels, GREEN);
        //         tft->drawFastHLine(x, y-1, width_in_pixels, GREEN);
        //     }
        // }
        return end_y;
    }
};


// Redesigned compound menu item: Value | Minimum | Maximum | Output in one row
// Slot rows are added separately by makeControls() so they are top-level navigable items
class ParameterMenuItem : public SubMenuItemBarCustomProportions {
    public:

    FloatParameter **proxy_parameter = nullptr;

    ParameterMenuItem(const char *label, FloatParameter **proxy_parameter, bool show_header = true)
        : SubMenuItemBarCustomProportions(label, 4, true, show_header)
    {
        this->proxy_parameter = proxy_parameter;

        this->set_column_proportion(0, 0.25f);  // Value
        this->set_column_proportion(1, 0.25f);  // Minimum
        this->set_column_proportion(2, 0.25f);  // Maximum
        this->set_column_proportion(3, 0.25f);  // Output

        this->add(new ParameterValueMenuItem((char*)"Value", this->proxy_parameter));
        this->add(new ParameterRangeMenuItem("Min", this->proxy_parameter, MINIMUM));
        this->add(new ParameterRangeMenuItem("Max", this->proxy_parameter, MAXIMUM));
        ParameterValueMenuItem *output = new ParameterValueMenuItem((char*)"Output", this->proxy_parameter);
        output->setReadOnly();
        output->selectable = false;
        output->set_show_output_mode();
        this->add(output);
    }

    virtual int small_display(int index, int x, int y, int width_in_pixels, bool is_selected, bool is_opened, bool outer_selected) override {
        int end_y = SubMenuItemBarCustomProportions::small_display(index, x, y, width_in_pixels, is_selected, is_opened, outer_selected);
        if (is_opened) {
            MenuItem *ctrl = this->items->get(index);
            if (ctrl != nullptr && !ctrl->wants_fullscreen_overlay_when_opened_in_bar())
                tft->drawFastHLine(x, y, width_in_pixels, GREEN);
        }
        return end_y;
    }
};