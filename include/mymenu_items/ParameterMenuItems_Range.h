
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
                //Serial.println("parameter isn't null, getting normal.."); Serial_flush();
                this->internal_value = this->get_current_value(); //(*parameter)->getCurrentNormalValue();// * 100.0;
                //Serial.println("set getCurrentNormalValue()!"); Serial_flush();
                this->minimumDataValue = (*parameter)->getMinimumDataLimit(); 
                this->maximumDataValue = (*parameter)->getMaximumDataLimit(); 
            }
            this->range_type = range_type;

            go_back_on_select = true;
            //this->minimumDataValue = parameter->minimumDataValue;
            //this->maximumDataValue = parameter->maximumDataValue;
            //this->step = 0.01;
        }

        float getMinimumDataValue() override {
            return (*parameter)->getMinimumDataLimit();
        }
        float getMaximumDataValue() override {
            return (*parameter)->getMaximumDataLimit();
        }

        float get_internal_value() override {
            switch (range_type) {
                case ParameterRangeType::MINIMUM:
                    return (*parameter)->getMinimumDataLimit();
                case ParameterRangeType::MAXIMUM:
                    return (*parameter)->getMaximumDataLimit();
                default: 
                    return 0.0f;
            }
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
            //this->internal_value = this->get_current_value() / 100.0; //->parameter->getCurrentValue() * this->maximumDataValue; //->getCurrentValue() * this->maximumDataValue;
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
               // hmm, ideally we would fetch the underlying parameter's  here
            return (*parameter)->getFormattedLimit(this->get_current_value());
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
                //if (this->debug) Serial.printf(F("ParameterValueMenuItem#set_current_value(%f) about to call updateValueFromNormal(%f) (maximumDataValue is %i)\n"), value, v, this->maximumDataValue);
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
            //if (this->debug) Serial.printf("ParameterRangeMenuItem#change_value(int=%i) about to call change_value(float=%f)\n", new_value, new_value);
            this->change_value(f);
        }

        virtual bool button_select() override {
            //Serial.printf("ParameterRangeMenuItem#button_select - internal value is %3.3f!\n", this->get_internal_value());
            //this->change_value(this->get_internal_value());

            return go_back_on_select;
        }

        virtual void change_value(float new_value) override {    // doesn't override, implements for normalled float?
            if (readOnly) return;
            float last_value = this->get_current_value();
            //if (this->debug) Serial.printf("ParameterRangeMenuItem#change_value(%f)\t in %s\tabout to call set_current_value(%f)\n", new_value, this->label);
            this->set_current_value(new_value);
            //if (this->debug) Serial.printf("ParameterRangeMenuItem#change_value(%f)\t after set_current_value(%f) get_current_value is \n", new_value, this->get_current_value());
            if (on_change_handler!=nullptr) {
                //if (this->debug)  { Serial.println(F("NumberControl calling on_change_handler")); Serial_flush(); }
                on_change_handler(last_value, this->internal_value); //this->get_internal_value());
                //if (this->debug)  { Serial.println(F("NumberControl after on_change_handler")); Serial_flush(); }
            }
        }
};

