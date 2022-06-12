#include "Parameter.h"

class MenuItem;
class ToggleControl;

template<class TargetClass, class DataType = bool>
class ToggleParameter : public Parameter<TargetClass,DataType> {
    public:

    //TargetClass *target = nullptr;
    void(TargetClass::*setter_func_on)(bool) = nullptr;
    void(TargetClass::*setter_func_off)(bool) = nullptr;

    ToggleParameter(
        char *label, 
        TargetClass *target, 
        bool initial_value, 
        void(TargetClass::*setter_func_on)(bool), 
        void(TargetClass::*setter_func_off)(bool)
    ) : Parameter<TargetClass,DataType>(label, target, nullptr) {
        this->currentValue = initial_value;
        this->target = target;
        this->setter_func_on = setter_func_on;
        this->setter_func_off = setter_func_off;
    }

    virtual void setParamValue(double value) override {
        Serial.print("ToggleParameter#setParamValue(double) about to call setParamValue(bool)"); Serial.flush();
        if (value>=0.5) {
            Serial.println("true!!");
            this->setParamValue((bool)true);
        } else {
            Serial.println("false!!");
            this->setParamValue((bool)false);
        }
    }

    virtual void setParamValue(bool value) {
        Serial.print("ToggleParameter#setParamValue("); Serial.flush();
        Serial.print(value);
        Serial.println(")"); Serial.flush();

        this->lastValue = this->currentValue;
        this->currentValue = value;
        //this->func(value);
        if (this->debug) {
            Serial.printf("%s: Calling setter func for value (", this->label);
            Serial.print(value);
            Serial.println(")");
        }
        if (this->currentValue) {
            if (this->setter_func_on!=nullptr) {
                Serial.println("ToggleParameter#setParamValue calling setter_func_on!"); Serial.flush();
                (this->target->*setter_func_on)(true);
            } else {
                Serial.println("ToggleParameter#setParamValue has no setter_func_on!"); Serial.flush();
            }
        } else {
            if (this->setter_func_off!=nullptr) {
                Serial.println("ToggleParameter#setParamValue calling setter_func_off!"); Serial.flush();
                (this->target->*setter_func_off)(true);
            } else {
                Serial.println("ToggleParameter#setParamValue has no setter_func_off!"); Serial.flush();
            }
        }
    }

    virtual const char* getFormattedValue() {
        static char fmt[20] = "              ";
        sprintf(fmt, "%s", this->currentValue?'On':'Off');
        return fmt;
    }

    virtual MenuItem *makeControl() {
        return new ToggleControl(this->label, this);
    }

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