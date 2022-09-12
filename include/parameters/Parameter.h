#ifndef PARAMETER__INCLUDED
#define PARAMETER__INCLUDED

#include <Arduino.h>
#ifdef CORE_TEENSY
    // use alternative method if Teensy
#else
    #include <ArxTypeTraits.h>
#endif
#include <stddef.h>

extern char NEXT_PARAMETER_NAME;

//#include "ParameterInput.h"

class BaseParameterInput;
#ifdef ENABLE_SCREEN
class MenuItem;
class ParameterMenuItem;
#endif

/*class AbstractBaseParameter {
    public:
    char label[20];
    AbstractBaseParameter(char *label) {
        strcpy(this->label, label);
    };
};

template<class DataType>*/
class BaseParameter { //: public AbstractBaseParameter {
    public:
        //DataType minimum_value = 0.0f;
        //DataType maximum_value = 1.0f;

        bool debug = false;

        char label[20];

        BaseParameter(char *label) {
            strcpy(this->label, label);
        };
        virtual void setParamValue(double value, double range = 1.0) {};
        /*virtual DataType getCurrentValue() {};
        virtual DataType getLastValue() {};*/
        virtual const char* getFormattedValue() {
            //static char noval = "[none]";
            return "[none]";
        };
        virtual void on_unbound(BaseParameterInput*) {}

        // called when a BaseParameterInput that was targetting this item release control of this parameter
};

//template<class DataType>
class DataParameter : public BaseParameter {
    public: 
    
    double currentValue;
    double lastValue;
    double minimum_value = 0.0;
    double maximum_value = 100.0;
    double initial_value = 0.0;

    DataParameter(char *label) : BaseParameter(label) {}

    virtual double getCurrentValue() {
        return this->currentValue;
    }
    virtual double getLastValue() {
        return this->lastValue;
    }

    virtual void setParamValue(double value, double range = 1.0) {};
    virtual void on_unbound(BaseParameterInput *input) {
        this->setParamValue(this->initial_value * this->maximum_value);
        //this->setParamValue(0.0f);
    }

    #ifdef ENABLE_SCREEN
    virtual MenuItem *makeControl();
    #endif
};


// an object that can be targeted by a ParameterInput, calls setter method on final target object
template<class TargetClass, class DataType = double>
class Parameter : public DataParameter {
    public:

        //DataType minimum_value = 0.0;
        //DataType maximum_value = 100.0;

        TargetClass *target;
        void(TargetClass::*setter_func)(DataType value) = nullptr;// setter_func;
        double(TargetClass::*getter_func)() = nullptr;// setter_func;

        /*void (test::*func)(DataType);
        void (test::*func)(float);
        void (test::*func)(int);*/

        Parameter(char *label, TargetClass *target) : DataParameter(label) {
            this->target = target;
        }
        Parameter(char *label, TargetClass *target, DataType initial_value_normal) : Parameter(label, target) {
            this->initial_value = initial_value_normal;
        }
        Parameter(char *label, TargetClass *target, void(TargetClass::*setter_func)(DataType)) : Parameter(label, target) {
            this->setter_func = setter_func;
        }
        Parameter(char *label, TargetClass *target, void(TargetClass::*setter_func)(DataType), double(TargetClass::*getter_func)()) : Parameter(label, target, setter_func) {
            this->getter_func = getter_func;
            //if (getter_func!=nullptr)
            //    this->setInitialValue();
        }
        Parameter(char *label, TargetClass *target, double initial_value_normal, void(TargetClass::*setter_func)(DataType)) : Parameter(label, target, setter_func) {
            this->initial_value = initial_value_normal;
            //this->setInitialValueFromNormal(initial_value_normal);
        }

        /*virtual DataParameter* setMinimumValue(double minimum_value) {
            this->minimum_value = minimum_value;
            return this;
        }
        virtual DataParameter* setMaximumValue(double minimum_value) {
            this->minimum_value = minimum_value;
            return this;
        }*/
        virtual DataParameter* initialise_values(double minimum_value = 0.0, double maximum_value = 100.0) {
            double current_value_normal = 0.0f;
            if (this->getter_func!=nullptr) 
                current_value_normal = (this->target->*getter_func)() / maximum_value;
            return this->initialise_values(minimum_value, maximum_value, current_value_normal);
        }
        virtual DataParameter* initialise_values(double minimum_value, double maximum_value, double current_value_normal) {
            // TODO: this probably isnt legal usage of NULL?
            if (/*current_value_normal==NULL && */this->getter_func!=nullptr) 
                current_value_normal = (this->target->*getter_func)() / maximum_value;
            this->minimum_value = minimum_value;
            this->maximum_value = maximum_value;
            this->setParamValue(current_value_normal * maximum_value);
            return this;
        }


        // setInitialValue in target value ie as a double to be multiplied by maximum_value
        virtual void setInitialValueFromNormal(double value) {
            this->currentValue = value * this->maximum_value;
        }

        virtual void setInitialValue() {
            if (this->getter_func!=nullptr)
                this->setInitialValueFromNormal((this->target->*getter_func)() / this->maximum_value);
            //this->currentValue = value;
            /*if (getter_func!=nullptr) {
                this->currentValue = 
            }*/
        }

        virtual void actual_set_value(double value) {
            if (this->target!=nullptr && this->setter_func!=nullptr) {
                (this->target->*setter_func)((DataType)value);
            } else {
                #ifdef ENABLE_PRINTF
                    Serial.printf("WARNING: no target / no setter_func in %s!\n", this->label);
                #endif
            }
        }

        virtual void setParamValue(double value, double range=1.0) override {
            if (this->currentValue==value) 
                return;
            if (this->debug) { 
                Serial.println("Parameter#setParamValue()"); Serial.flush(); 
            }
            this->lastValue = this->currentValue;
            this->currentValue = value;
            //this->func(value);
            #ifdef ENABLE_PRINTF
                if (this->debug) {
                    Serial.println("Parameter#setParamValue()"); Serial.flush();
                    Serial.printf("%s: Calling setter func for value (", this->label);
                    Serial.print(value);
                    Serial.println(")");
                }
            #endif   

            this->actual_set_value(value);
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
        #ifdef CORE_TEENSY
            // use Teensy version of this code that uses overriding functions
            virtual const char* getFormattedValue() override {
                char *fmt = this->parseFormattedDataType((DataType)this->getCurrentValue());
                //Serial.printf("getFormattedValue: '%s'\n", fmt);
                return fmt;
            };
            virtual const char* parseFormattedDataType(bool value) {
                static char fmt[20] = "              ";
                sprintf(fmt, "%s", this->getCurrentValue()?"On" : "Off");
                return fmt;
            }
            virtual const char* parseFormattedDataType(double value) {
                static char fmt[20] = "              ";
                sprintf(fmt, "%3i%% (float)",     (int)(100.0*this->getCurrentValue())); //->getCurrentValue());
                return fmt;
            }
            virtual const char* parseFormattedDataType(unsigned int value) {
                static char fmt[20] = "              ";
                sprintf(fmt, "%5u (unsigned)",    (unsigned int)(this->maximum_value*this->getCurrentValue())); //getCurrentValue());
                return fmt;
            }
            virtual const char* parseFormattedDataType(int value) {
                static char fmt[20] = "              ";
                sprintf(fmt, "%5i (signed)",      (int)(this->maximum_value*this->getCurrentValue())); //getCurrentValue());
                return fmt;
            }
        #else
            virtual const char* getFormattedValue() override {
                static char fmt[20] = "              ";
                if constexpr (std::is_integral<DataType>::value && std::is_same<DataType, bool>::value) {
                    sprintf(fmt, "%s", this->getCurrentValue()?"On" : "Off");
                } else if constexpr (std::is_floating_point<DataType>::value) {
                    sprintf(fmt, "%3i%% (float)",     (int)(100.0*this->getCurrentValue())); //->getCurrentValue());
                } else if constexpr (std::is_unsigned<DataType>::value) {
                    sprintf(fmt, "%5u (unsigned)",    (unsigned int)(this->maximum_value*this->getCurrentValue())); //getCurrentValue());
                } else {
                    sprintf(fmt, "%5i (signed)",      (int)(this->maximum_value*this->getCurrentValue())); //getCurrentValue());
                }
                //Serial.printf("getFormattedValue: '%s'\n", fmt);
                return fmt;
            };
        #endif

        virtual void set_target_object(TargetClass *target) {
            this->target = target;
        }
        virtual void set_target_func(void(TargetClass::*fp)(DataType)) {
            this->setter_func = fp;
        }
};



#endif