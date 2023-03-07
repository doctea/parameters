#ifndef MENU_PARAMETER_MENUITEMS__INCLUDED
#define MENU_PARAMETER_MENUITEMS__INCLUDED

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

#include "../parameters/Parameter.h"

#include <LinkedList.h>

// direct control over a Parameter from menu
class ParameterValueMenuItem : public DirectNumberControl<float> {
    public:
        FloatParameter *parameter = nullptr;
        //Parameter<TargetClass, DataType> *parameter = nullptr;

        bool show_output_mode = false;  // true if this widget should show the last post-modulation output value; false if it should show the pre-modulation value

        ParameterValueMenuItem(char *label, FloatParameter *parameter) : DirectNumberControl(label) {
            strcpy(this->label, label);
            this->parameter = parameter;
            this->internal_value = parameter->getCurrentNormalValue() * 100.0;
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
            if (this->parameter==nullptr)
                return 0;
            /*if (this->debug) {
                Serial.printf("ParameterValueMenuItem for %s (parameter %s) has currentValue ", this->label, this->parameter->label);
                Serial.println(parameter->getCurrentValue());
            }*/
            //return (int) (parameter->getCurrentNormalValue() * 100.0); //(float)this->maximum_value);    // turn into percentage
            return parameter->getCurrentNormalValue();
        }

        virtual const char *getFormattedValue() override {
            static char fmt[20] = "";
            if (this->show_output_mode) {
                return this->getFormattedOutputValue();
            }
            snprintf(fmt, 20, "%-3s", this->parameter->getFormattedValue()); 
            return fmt;
        }
        virtual const char *getFormattedOutputValue() {
            static char fmt[20] = "";
            snprintf(fmt, 20, "%s", this->parameter->getFormattedLastOutputValue());
            return fmt;
        }

        virtual const char *getFormattedInternalValue() override {
            return this->parameter->getFormattedValue(this->internal_value);
        }

        virtual const char *getFormattedExtra() override {
            if (this->parameter->is_modulatable())
                return this->parameter->getFormattedValue(parameter->getLastModulatedNormalValue());
            return nullptr;
        }

        virtual void set_current_value(float value) override { 
            //if (this->debug) { Serial.printf(F("ParameterValueMenuItem#set_current_value(%f) on %s\n"), value, this->label); Serial_flush(); }

            if (this->parameter==nullptr)
                return;
            if (this->readOnly)
                return;
           
            if (this->parameter!=nullptr) {
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
                this->parameter->updateValueFromNormal(v);
            } 
        }

        // directly increase the parameter's value
        virtual void increase_value() override {
            //this->debug = true;
            parameter->decrementValue();
            this->internal_value = parameter->getCurrentNormalValue(); //this->maximum_value;
            //if (this->debug) Serial.printf(F("ParameterValueMenuItem#increase_value updated internal_value to %f (from %f * 100.0)\n"), internal_value, parameter->getCurrentNormalValue());
            //this->debug = false;
        }
        // directly decrease the parameter's value
        virtual void decrease_value() override {
            //this->debug = true;
            parameter->incrementValue();
            this->internal_value = parameter->getCurrentNormalValue(); // * 100.0; //this->maximum_value;
            //if (this->debug) Serial.printf(F("ParameterValueMenuItem#decrease_value updated internal_value to %f (from %f * 100.0)\n"), internal_value, parameter->getCurrentNormalValue());
            //this->debug = false;
        }

        virtual bool knob_left() override {
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
            change_value(this->internal_value);
            
            return go_back_on_select;
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

// compound menu item that shows a direct value-setter widget, 3x modulation amount widgets, and the last post-modulation output value
class ParameterMenuItem : public SubMenuItemBar {
    public:

    FloatParameter *parameter = nullptr;

    ParameterMenuItem(const char *label, FloatParameter *parameter) : SubMenuItemBar(label) {
        this->parameter = parameter;

        // add the direct Value changer
        this->add(new ParameterValueMenuItem((char*)"Value", parameter));

        // add the modulation Amount % changers
        for (unsigned int i = 0 ; i < MAX_SLOT_CONNECTIONS ; i++) {
            // todo: make the modulation source part configurable too
            // todo: make the label part dynamically generated on-the-fly by the DirectNumberControl
            char labelnew[8];
            char *input_name =  parameter->connections[i].parameter_input!=nullptr ? 
                                parameter->connections[i].parameter_input->name : 
                                (char*)"None";
            Debug_printf(F("\tfor %s, setting to parameter_input@%p '%s'\n"), label, parameter->connections[i].parameter_input, input_name);
            Serial_flush();
            snprintf(labelnew, 8, "%s", input_name); //"Amt "
            DirectNumberControl<float> *input_amount_control = new DirectNumberControl<float>(
                labelnew, 
                &parameter->connections[i].amount, 
                parameter->connections[i].amount, 
                -1.0f, 
                1.0f, 
                nullptr
            );
            //input_amount_control->default_fg = parameter->connections[i].parameter_input->colour;
                //input_amount_control->debug = true;
            this->add(input_amount_control);
            if (parameter->connections[i].parameter_input!=nullptr) {
                Debug_printf(F("ParameterMenuItem(%s) connection %i got colour %4x from '%s'!\n"), label, i, parameter->connections[i].parameter_input->colour, parameter->connections[i].parameter_input->name);
                input_amount_control->default_fg = parameter->connections[i].parameter_input->colour; // do this after add to avoid parent colours overwriting it?
            } else {
                Debug_printf(F("\tNo parameter_input assigned"));
            }
        }

        // add another small widget to display the last output value (after modulation etc)
        ParameterValueMenuItem *output = new ParameterValueMenuItem((char*)"Output", parameter);
        output->setReadOnly();
        output->set_show_output_mode();
        this->add(output); 
        
        /*this->add(this->ctrl_amt_1);
        this->add(this->ctrl_amt_2);
        this->add(this->ctrl_amt_3);*/
        //this->add(new SourceSelectorControl("S2", parameter));
        //this->add(new SourceSelectorControl("S3", parameter));
        //this->
    }

    void on_add() override {
        SubMenuItemBar::on_add();
        for (unsigned int i = 0 ; i < 3 ; i++) {
            parameter->connections[i].amount_control->set_default_colours(parameter->connections[i].parameter_input->colour);
        }
    }
};

#endif