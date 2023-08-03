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
        char *label, 
        TargetClass *target, 
        bool initial_value, 
        void(TargetClass::*setter_func_on)(), 
        void(TargetClass::*setter_func_off)()
    ) : DataParameter<TargetClass,DataType>(label, target, nullptr) {
        this->currentValue = initial_value;
        this->target = target;
        this->setter_func_on = setter_func_on;
        this->setter_func_off = setter_func_off;
    }

    ToggleParameter(
        char *label, 
        TargetClass *target, 
        bool initial_value, 
        void(TargetClass::*setter_func)(bool)
    ) : DataParameter<TargetClass,DataType>(label, target, nullptr) {
        this->currentValue = initial_value;
        this->target = target;
        this->setter_func = setter_func;
    }

    virtual void setNormalParamValue(float value, float range = 1.0) override {
        /*if (this->debug) {
            Serial.print(F("ToggleParameter#setParamValue(float of "));
            Serial.print(value);
            Serial.print(F(") about to call setParamValue(bool)")); Serial_flush();
        }*/
        if (value>=0.5) {
            //if (this->debug) Serial.println("true!!");
            this->setParamValue((bool)true);
        } else {
            //if (this->debug) Serial.println("false!!");
            this->setParamValue((bool)false);
        }
    }

    virtual void setParamValue(bool value) {
        if (value==this->currentValue) 
            return;
        /*if (this->debug) {
            Serial.print("ToggleParameter#setParamValue("); Serial_flush();
            Serial.print(value);
            Serial.println(")"); Serial_flush();
        }*/

        this->lastValue = this->currentValue;
        this->currentValue = value;
        //this->func(value);
        /*if (this->debug) {
            Serial.printf("%s: Calling setter func for value (", this->label);
            Serial.print(value);
            Serial.println(")");
        }*/
        
        if (this->setter_func_off!=nullptr) { // if a second setter_func_off is specifed, call the individual functions without passing state as a parameter
            if (this->currentValue) {
                (this->target->*setter_func_on)();
            } else {
                (this->target->*setter_func_off)();
            }
        } else {        // no specified setter_func_off passed, so pass state as parameter
            (this->target->*setter_func)(this->currentValue);
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

    virtual const char* getFormattedValue() {
        return this->currentValue ? BaseParameter::label_on : BaseParameter::label_off;
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