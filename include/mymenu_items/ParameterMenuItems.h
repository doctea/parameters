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

        ParameterValueMenuItem(char *label, FloatParameter **parameter) : DirectNumberControl(label) {
            strncpy(this->label, label, 20);
            this->parameter = parameter;
            if (*parameter!=nullptr)
                this->internal_value = (*parameter)->getCurrentNormalValue() * 100.0;
            this->minimum_value = 0.0f; 
            this->maximum_value = 1.0f; 

            go_back_on_select = true;
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
        virtual float get_current_value() override {
            if (this->parameter==nullptr || *parameter==nullptr)
                return 0;
            /*if (this->debug) {
                Serial.printf("ParameterValueMenuItem for %s (parameter %s) has currentValue ", this->label, this->parameter->label);
                Serial.println(parameter->getCurrentValue());
            }*/
            //return (int) (parameter->getCurrentNormalValue() * 100.0); //(float)this->maximum_value);    // turn into percentage
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
            snprintf(fmt, 20, (*parameter)->getFormattedLastOutputValue());
            return fmt;
        }

        virtual const char *getFormattedInternalValue() override {
            return (*parameter)->getFormattedValue(this->internal_value);
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
                    Serial.printf(F("\tParameterMenuItem#set_current_value(%f): Calling setParamValue %f (max value %i) on Parameter %s\n"), value, value, this->maximum_value, this->parameter->label); Serial_flush();
                }*/
                //float v = (float)((float)value / (float)this->maximum_value);
                //float v = (float)((float)value/(float)this->maximum_value); // / (float)this->maximum_value); // * (float)this->maximum_value);
                float v = value;

                /*if (this->debug) {
                    Serial.print(F("ParameterValueMenuItem#set_current_value() got v to pass: "));                    
                    Serial.println(v);
                }*/
                //this->parameter->setParamValue(v);    // turn into percentage
                //if (this->debug) Serial.printf(F("ParameterValueMenuItem#set_current_value(%f) about to call updateValueFromNormal(%f) (maximum_value is %i)\n"), value, v, this->maximum_value);
                (*parameter)->updateValueFromNormal(v);
            } 
        }

        // directly increase the parameter's value
        virtual void increase_value() override {
            //this->debug = true;
            (*parameter)->incrementValue();
            this->internal_value = (*parameter)->getCurrentNormalValue(); //this->maximum_value;
            //if (this->debug) Serial.printf(F("ParameterValueMenuItem#increase_value updated internal_value to %f (from %f * 100.0)\n"), internal_value, parameter->getCurrentNormalValue());
            //this->debug = false;
        }
        // directly decrease the parameter's value
        virtual void decrease_value() override {
            //this->debug = true;

            (*parameter)->decrementValue();
            this->internal_value = (*parameter)->getCurrentNormalValue(); // * 100.0; //this->maximum_value;
            //if (this->debug) Serial.printf(F("ParameterValueMenuItem#decrease_value updated internal_value to %f (from %f * 100.0)\n"), internal_value, parameter->getCurrentNormalValue());
            //this->debug = false;
        }

        /*virtual bool knob_left() override {
            if (readOnly) return false;
            //if (this->debug) Serial.printf(F("------ ParameterValueMenuItem#knob_left, internal_value=%f\n"), internal_value);
            increase_value();
            //if (this->debug) Serial.printf(F("------ ParameterValueMenuItem#knob_left, about to call change_value(%f)\n"), internal_value);
            change_value(this->internal_value);
            //if (this->debug) Serial.printf(F(">------<\n"));
            return true;
        }
        virtual bool knob_right() override {
            if (readOnly) return false;
            //if (this->debug) Serial.printf(F("------ ParameterValueMenuItem#knob_right, internal_value=%f\n"), internal_value);
            decrease_value();
            //if (this->debug) Serial.printf(F("------ ParameterValueMenuItem#knob_right, about to call change_value(%f)\n"), internal_value);
            change_value(this->internal_value);
            //if (this->debug) Serial.printf(F(">------<\n"));
            return true;
        }
        virtual bool button_select() override {
            if (readOnly) return true;

            this->internal_value = this->get_current_value();
            change_value(this->internal_value);
            
            return go_back_on_select;
        }*/

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


enum ParameterRangeType {
    MINIMUM, MAXIMUM
};

// direct control over a Parameter Range from menu
class ParameterRangeMenuItem : public DirectNumberControl<float> {
    public:
        DataParameterBase **parameter = nullptr;
        ParameterRangeType range_type;
        //Parameter<TargetClass, DataType> *parameter = nullptr;

        bool show_output_mode = false;  // true if this widget should show the last post-modulation output value; false if it should show the pre-modulation value

        ParameterRangeMenuItem(char *label, DataParameterBase **parameter, ParameterRangeType range_type) : DirectNumberControl(label) {
            strncpy(this->label, label, 20);
            this->parameter = parameter;
            if (*parameter!=nullptr)
                this->internal_value = (*parameter)->getCurrentNormalValue() * 100.0;
            this->minimum_value = parameter->minimum_value; 
            this->maximum_value = parameter->maximum_value; 
            this->range_type = range_type;

            go_back_on_select = true;
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
        virtual float get_current_value() override {
            if (this->parameter==nullptr || *parameter==nullptr)
                return 0;
            /*if (this->debug) {
                Serial.printf("ParameterValueMenuItem for %s (parameter %s) has currentValue ", this->label, this->parameter->label);
                Serial.println(parameter->getCurrentValue());
            }*/
            //return (int) (parameter->getCurrentNormalValue() * 100.0); //(float)this->maximum_value);    // turn into percentage
            //return (*parameter)->getCurrentNormalValue();
            if (this->range_type==MINIMUM)
                return parameter->get_minimum_limit();
            else
                return parameter->get_maximum_limit();
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
            snprintf(fmt, 20, (*parameter)->getFormattedLastOutputValue());
            return fmt;
        }

        virtual const char *getFormattedInternalValue() override {
            return (*parameter)->getFormattedValue(this->internal_value);
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
                    Serial.printf(F("\tParameterMenuItem#set_current_value(%f): Calling setParamValue %f (max value %i) on Parameter %s\n"), value, value, this->maximum_value, this->parameter->label); Serial_flush();
                }*/
                //float v = (float)((float)value / (float)this->maximum_value);
                //float v = (float)((float)value/(float)this->maximum_value); // / (float)this->maximum_value); // * (float)this->maximum_value);
                float v = value;

                /*if (this->debug) {
                    Serial.print(F("ParameterValueMenuItem#set_current_value() got v to pass: "));                    
                    Serial.println(v);
                }*/
                //this->parameter->setParamValue(v);    // turn into percentage
                //if (this->debug) Serial.printf(F("ParameterValueMenuItem#set_current_value(%f) about to call updateValueFromNormal(%f) (maximum_value is %i)\n"), value, v, this->maximum_value);
                //(*parameter)->updateValueFromNormal(v);
                if (this->range_type==MINIMUM)
                    return parameter->set_minimum_limit(v);
                else
                    return parameter->set_maximum_limit(v);
                } 
        }

        // directly increase the parameter's value
        virtual void increase_value() override {
            //this->debug = true;
            (*parameter)->incrementValue();
            this->internal_value = (*parameter)->getCurrentNormalValue(); //this->maximum_value;
            //if (this->debug) Serial.printf(F("ParameterValueMenuItem#increase_value updated internal_value to %f (from %f * 100.0)\n"), internal_value, parameter->getCurrentNormalValue());
            //this->debug = false;
        }
        // directly decrease the parameter's value
        virtual void decrease_value() override {
            //this->debug = true;

            (*parameter)->decrementValue();
            this->internal_value = (*parameter)->getCurrentNormalValue(); // * 100.0; //this->maximum_value;
            //if (this->debug) Serial.printf(F("ParameterValueMenuItem#decrease_value updated internal_value to %f (from %f * 100.0)\n"), internal_value, parameter->getCurrentNormalValue());
            //this->debug = false;
        }

        /*virtual bool knob_left() override {
            if (readOnly) return false;
            //if (this->debug) Serial.printf(F("------ ParameterValueMenuItem#knob_left, internal_value=%f\n"), internal_value);
            increase_value();
            //if (this->debug) Serial.printf(F("------ ParameterValueMenuItem#knob_left, about to call change_value(%f)\n"), internal_value);
            change_value(this->internal_value);
            //if (this->debug) Serial.printf(F(">------<\n"));
            return true;
        }
        virtual bool knob_right() override {
            if (readOnly) return false;
            //if (this->debug) Serial.printf(F("------ ParameterValueMenuItem#knob_right, internal_value=%f\n"), internal_value);
            decrease_value();
            //if (this->debug) Serial.printf(F("------ ParameterValueMenuItem#knob_right, about to call change_value(%f)\n"), internal_value);
            change_value(this->internal_value);
            //if (this->debug) Serial.printf(F(">------<\n"));
            return true;
        }
        virtual bool button_select() override {
            if (readOnly) return true;

            this->internal_value = this->get_current_value();
            change_value(this->internal_value);
            
            return go_back_on_select;
        }*/

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

// compound menu item that shows a direct value-setter widget, 3x modulation amount widgets, and the last post-modulation output value
class ParameterMenuItem : public SubMenuItemBar {
    public:

    //FloatParameter *parameter = nullptr;
    FloatParameter **proxy_parameter = nullptr; //&parameter;

    ParameterMenuItem(const char *label, FloatParameter **proxy_parameter) 
       : SubMenuItemBar(label)
    {
        this->proxy_parameter = proxy_parameter;

        // add the direct Value changer
        this->add(new ParameterValueMenuItem((char*)"Value", this->proxy_parameter));

        // add the modulation Amount % changers
        for (uint_fast8_t i = 0 ; i < MAX_SLOT_CONNECTIONS ; i++) {
            // todo: make the modulation source part configurable too
            // todo: make the label part dynamically generated on-the-fly by the DirectNumberControl
            char labelnew[8];
            if (proxy_parameter!=nullptr && *proxy_parameter!=nullptr) {
                char *input_name =  (*proxy_parameter)->connections[i].parameter_input!=nullptr ? 
                                    (*proxy_parameter)->connections[i].parameter_input->name : 
                                    (char*)"None";
                Debug_printf(F("\tfor %s, setting to parameter_input@%p '%s'\n"), label, (*proxy_parameter)->connections[i].parameter_input, input_name);
                //Serial_flush();
                snprintf(labelnew, 8, "%s", input_name); //"Amt "*/
            } else {
                snprintf(labelnew, 8, "Amt%i", i);
            }
            ParameterMapPercentageControl *input_amount_control = new ParameterMapPercentageControl(
                labelnew,
                this->proxy_parameter,
                i
            );
            this->add(input_amount_control);
        }

        // add another small widget to display the last output value (after modulation etc)
        ParameterValueMenuItem *output = new ParameterValueMenuItem((char*)"Output", this->proxy_parameter);
        output->setReadOnly();
        output->selectable = false;
        output->set_show_output_mode();

        this->add(output); 
    }

};



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
        if (index==BIPOLAR)
            return "Bi";
        if (index==UNIPOLAR)
            return "Uni";
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
        this->default_fg = (*this->parameter)->connections[slot_number].parameter_input->colour;
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
        //tft->setTextColor((*this->parameter)->connections[slot_number].parameter_input->colour, BLACK);

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