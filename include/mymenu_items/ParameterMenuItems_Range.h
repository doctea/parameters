
#include "parameters/Parameter.h"
#include "menuitems.h"

enum ParameterRangeType {
    MINIMUM, MAXIMUM
};

// direct control over a Parameter Range from menu
class ParameterRangeMenuItem : public DirectNumberControl<float> {
    public:
        FloatParameter **parameter = nullptr;
        ParameterRangeType range_type;
        bool show_output_mode = false;  // true if this widget should show the last post-modulation output value; false if it should show the pre-modulation value

        ParameterRangeMenuItem(const char *label, FloatParameter **parameter, ParameterRangeType range_type) : DirectNumberControl(label) {
            this->parameter = parameter;
            if (parameter!=nullptr && *parameter!=nullptr) {
                Serial.println("parameter isn't null, getting normal.."); Serial_flush();
                this->internal_value = this->get_current_value(); //(*parameter)->getCurrentNormalValue();// * 100.0;
                Serial.println("set getCurrentNormalValue()!"); Serial_flush();
                this->minimum_value = (*parameter)->minimumNormalValue; 
                this->maximum_value = (*parameter)->maximumNormalValue; 
            }
            this->range_type = range_type;

            go_back_on_select = true;
            //this->minimum_value = parameter->minimum_value;
            //this->maximum_value = parameter->maximum_value;
            //this->step = 0.01;
        }

        // // true if this widget should show the last post-modulation output value; false if it should show the pre-modulation value
        virtual ParameterRangeMenuItem *set_show_output_mode(bool mode = true) {
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
            if (this->range_type==MINIMUM)
                return (*parameter)->getRangeMinimumLimit();
            else
                return (*parameter)->getRangeMaximumLimit();
        }

        virtual const char *getFormattedValue() override {
            static char fmt[20] = "";
            snprintf(fmt, 20, "%3s", (*parameter)->getFormattedLimit(this->get_current_value()));//(*parameter)->getFormattedValue()); 
            return fmt;
        }

        virtual const char *getFormattedInternalValue() override {
            return (*parameter)->getFormattedLimit(this->internal_value);
        }

        virtual const char *getFormattedExtra() override {
            if ((*parameter)->is_modulatable())
                return (*parameter)->getFormattedLimit((*parameter)->getLastModulatedNormalValue());
            return nullptr;
        }

        virtual void set_current_value(float value) override {
            //if (this->debug) { Serial.printf(F("ParameterValueMenuItem#set_current_value(%f) on %s\n"), value, this->label); Serial_flush(); }

            if (parameter==nullptr || *parameter==nullptr)
                return;
            if (this->readOnly)
                return;
           
            if (*parameter!=nullptr) {
                float v = value;
                /*if (this->debug) {
                    Serial.print(F("ParameterValueMenuItem#set_current_value() got v to pass: "));                    
                    Serial.println(v);
                }*/
                //if (this->debug) Serial.printf(F("ParameterValueMenuItem#set_current_value(%f) about to call updateValueFromNormal(%f) (maximum_value is %i)\n"), value, v, this->maximum_value);
                if (this->range_type==MINIMUM)
                    return (*parameter)->setRangeMinimumLimit(v);
                else
                    return (*parameter)->setRangeMaximumLimit(v);
            } 
        }

        // directly increase the parameter's range value
        virtual void increase_value() override {
            if (this->range_type==MINIMUM) {
                (*parameter)->incrementRangeMinimumLimit();
                this->internal_value = (*parameter)->getRangeMinimumLimit();
            } else {
                (*parameter)->incrementRangeMaximumLimit();
                this->internal_value = (*parameter)->getRangeMaximumLimit();
            }
        }
        // directly decrease the parameter's value
        virtual void decrease_value() override {
            if (this->range_type==MINIMUM) {
                (*parameter)->decrementRangeMinimumLimit();
                this->internal_value = (*parameter)->getRangeMinimumLimit();
            } else {
                (*parameter)->decrementRangeMaximumLimit();
                this->internal_value = (*parameter)->getRangeMaximumLimit();
            }
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

