#ifndef PARAMETER__INCLUDED
#define PARAMETER__INCLUDED

#include <ArxTypeTraits.h>

extern char NEXT_PARAMETER_NAME;

#include <Arduino.h>

//#include "ParameterInput.h"

class BaseParameterInput;

class BaseParameter {
    public:
        double minimum_value = 0.0f;
        double maximum_value = 1.0f;

        char label[20];

        BaseParameter(char *label) {
            strcpy(this->label, label);
        };
        virtual void setParamValue(double value) {};
        virtual double getCurrentValue() {};
        virtual double getLastValue() {};
        virtual const char* getFormattedValue() {
            //static char noval = "[none]";
            return "[none]";
        };

        // called when a BaseParameterInput that was targetting this item release control of this parameter
        virtual void on_unbound(BaseParameterInput *input);
};

// an object that can be targeted by a ParameterInput, calls setter method on final target object
template<class TargetClass, class DataType>
class Parameter : public BaseParameter {
    public:
        DataType currentValue;
        DataType lastValue;

        TargetClass *target;
        void(TargetClass::*setter_func)(DataType value);// setter_func;

        /*void (test::*func)(DataType);
        void (test::*func)(float);
        void (test::*func)(int);*/

        bool debug = false;

        Parameter(char *label, TargetClass *target, void(TargetClass::*setter_func)(DataType)) : BaseParameter(label) {
            this->target = target;
            this->setter_func = setter_func;
        }

        virtual void setParamValue(DataType value) {
            lastValue = currentValue;
            currentValue = value;
            //this->func(value);
            if (this->debug) {
                Serial.print("Calling setter func for value (");
                Serial.print(value);
                Serial.println(")");
            }
            (target->*setter_func)(value);
        }
        /*void setParamValue(float value) {
            lastValue = currentValue;
            currentValue = value;
            this->func(value);
        }
        void setParamValue(int value) {
            lastValue = currentValue;
            currentValue = value;
            this->func(value);
        }*/
        /*Targetable(TargetClass *target, TargetClass::*setter_func) {
            this->target = target;
            this->setter_func = setter_func;
        }*/
        virtual const char* getFormattedValue() {
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
        };


        virtual DataType getCurrentValue() {
            return this->currentValue;
        }
        virtual DataType getLastValue() {
            return this->lastValue;
        }

        virtual void set_target_object(TargetClass *target) {
            this->target = target;
        }
        virtual void set_target_func(void(TargetClass::*fp)(DataType)) {
            this->setter_func = fp;
        }
};

#endif