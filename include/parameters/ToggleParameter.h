#include "Parameter.h"

class MenuItem;
//class ToggleParameterControl;
//class ToggleParameterControl;
#include "mymenu_items/ToggleMenuItems.h"

template<class TargetClass, class DataType = bool>
class ToggleParameter : public DataParameter<TargetClass,DataType> {
    public:

    //TargetClass *target = nullptr;
    void(TargetClass::*setter_func)(bool) = nullptr;
    void(TargetClass::*setter_func_on)() = nullptr;
    void(TargetClass::*setter_func_off)() = nullptr;

    ToggleParameter(
        const char *label, 
        TargetClass *target, 
        bool initial_value, 
        void(TargetClass::*setter_func_on)(), 
        void(TargetClass::*setter_func_off)()
    ) : DataParameter<TargetClass,DataType>(label, target, nullptr) {
        this->currentValue = initial_value;
        this->target = target;
        this->setter_func_on = setter_func_on;
        this->setter_func_off = setter_func_off;

        this->debug = true;
    }

    ToggleParameter(
        const char *label, 
        TargetClass *target, 
        bool initial_value, 
        void(TargetClass::*setter_func)(bool)
    ) : DataParameter<TargetClass,DataType>(label, target, nullptr) {
        this->currentDataValue = initial_value;
        this->target = target;
        this->setter_func = setter_func;

        //this->debug = true;
    }

    /*virtual void setTargetValueFromData(DataType value, bool force = false) override {
        Serial.printf("ToggleParameter#setTargetValueFromData(%i)\n", value);
        DataParameter<TargetClass,DataType>::setTargetValueFromData(value, force);
    }*/

    virtual void setTargetValueFromData(DataType value, bool force = false) override {
        //if (value==this->currentDataValue) 
        //    return;

        // if (this->debug) {
            Serial.print("ToggleParameter#updateValueFromData("); Serial_flush();
            Serial.print(value);
            Serial.println(")"); Serial_flush();
        // }

        this->lastDataValue = this->currentDataValue;
        //this->currentDataValue = value;
        //this->func(value);
        /*if (this->debug) {
            Serial.printf("%s: Calling setter func for value (", this->label);
            Serial.print(value);
            Serial.println(")");
        }*/
        
        if (this->setter_func_off!=nullptr) { // if a second setter_func_off is specifed, call the individual functions without passing state as a parameter
            //Serial.printf("choosing between setter_func_on and setter_func_off based on this->currentDataValue=%i\n", this->currentDataValue);
            //Serial.flush();
            if (value) {
                (this->target->*setter_func_on)();
            } else {
                (this->target->*setter_func_off)();
            }
        } else {        // no specified setter_func_off passed, so pass state as parameter
            //Serial.printf("using setter_func with this->currentDataValue=%i\n", this->currentDataValue);
            //Serial.flush();
            (this->target->*setter_func)(value);
        }
        /*if (this->currentValue) {
            if (this->setter_func_on!=nullptr) {
                if (this->debug) { Serial.println("ToggleParameter#setParamValue calling setter_func_on!"); Serial_flush(); }
                (this->target->*setter_func_on)(true);
            } else {
                Serial.println("ToggleParameter#setParamValue has no setter_func_on!"); Serial_flush();
            }
        } else {
            if (this->setter_func_off!=nullptr) {
                if (this->debug) { Serial.println("ToggleParameter#setParamValue calling setter_func_off!"); Serial_flush(); } 
                (this->target->*setter_func_off)(false);
            } else {
                Serial.println("ToggleParameter#setParamValue has no setter_func_off!"); Serial_flush();
            }
        }*/
    }

    virtual DataType normalToData(float value) override {
        if (this->debug) {
            //Serial.printf("ToggleParameter's %s#", this->label);
            //Serial.printf("ToggleParameter#normalToData(%f) ", value);
            //Serial.printf(", range is %i to %i ", this->minimumDataLimit, this->maximumDataLimit);
            //Serial.printf(",\trange is %3.3f to %3.3f ", (float)this->minimumDataLimit, (float)this->maximumDataLimit);
            //Serial.printf(",\trange is %3.3f to %3.3f ", (float)this->get_effective_minimum_data_value(), (float)this->get_effective_maximum_data_value());
        }

        if (value < 0.5f) {
            return false;
        } else {
            return true;
        }
    }
    virtual float dataToNormal(DataType value) override {
        if (value) return 1.0f;
        return 0.f;
    }


    virtual const char* getFormattedValue() override {
        return this->getCurrentDataValue() ? BaseParameter::label_on : BaseParameter::label_off;
    }

    #ifdef ENABLE_SCREEN
    virtual MenuItem *makeControl() override {
        //Serial.println("ToggleParameter#makeControl()! ############");
        //Serial.printf("ToggleParameter#makeControl for %s\n", this->label);
        return new ToggleParameterControl(this->label, this);
    }
    #endif

    /*virtual const char* getFormattedValue() {
        static char fmt[20] = "              ";
        if constexpr (std::is_floating_point<DataType>::value) {
            sprintf(fmt, "%3i%% (float)",     (int)(100.0*this->getCurrentValue())); //->getCurrentValue());
        } else if constexpr (std::is_unsigned<DataType>::value) {
            sprintf(fmt, "%5u (unsigned)",    (unsigned int)this->getCurrentValue()); //getCurrentValue());
        } else {
            sprintf(fmt, "%5i (signed)",      (int)this->getCurrentValue()); //getCurrentValue());
        }
        //Serial.printf("getFormattedValue: '%s'\n", fmt);
        return fmt;
    };*/

};