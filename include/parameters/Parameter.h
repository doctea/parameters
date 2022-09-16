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
    //class ParameterMenuItem;
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
        virtual void updateValueFromNormal(double value/*, double range = 1.0*/) {};
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
    
    double currentNormalValue = 0.0;
    double lastNormalValue = 0.0;
    double initialNormalValue = 0.0;
    double minimumNormalValue = 0.0;
    double maximumNormalValue = 100.0;

    DataParameter(char *label) : BaseParameter(label) {}

    virtual double getCurrentNormalValue() {
        return this->currentNormalValue;
    }
    virtual double getLastNormalValue() {
        return this->lastNormalValue;
    }

    virtual void updateValueFromNormal(double value/*, double range = 1.0*/) override {};

    virtual void on_unbound(BaseParameterInput *input) {
        //this->updateValueFromNormal(this->initialNormalValue * this->maximumNormalValue);
        this->updateValueFromNormal(this->initialNormalValue);
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

        DataType minimumDataValue = 0.0;
        DataType maximumDataValue = 100.0;
        DataType lastDataValue = 0;
        DataType currentDataValue = 0;
        DataType initialDataValue = 0;

        TargetClass *target;
        void(TargetClass::*setter_func)(DataType value) = nullptr;// setter_func;
        DataType(TargetClass::*getter_func)() = nullptr;// setter_func;

        /*void (test::*func)(DataType);
        void (test::*func)(float);
        void (test::*func)(int);*/

        Parameter(char *label, TargetClass *target) : DataParameter(label) {
            this->target = target;
        }
        Parameter(char *label, TargetClass *target, DataType initial_value_normal) : Parameter(label, target) {
            this->initialNormalValue = initial_value_normal;
        }
        Parameter(char *label, TargetClass *target, void(TargetClass::*setter_func)(DataType)) : Parameter(label, target) {
            this->setter_func = setter_func;
        }
        Parameter(char *label, TargetClass *target, void(TargetClass::*setter_func)(DataType), DataType(TargetClass::*getter_func)()) : Parameter(label, target, setter_func) {
            this->getter_func = getter_func;
            //if (getter_func!=nullptr)
            //    this->setInitialValue();
        }
        Parameter(char *label, TargetClass *target, double initial_value_normal, void(TargetClass::*setter_func)(DataType)) : Parameter(label, target, setter_func) {
            this->initialNormalValue = initial_value_normal;
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
        virtual DataParameter* initialise_values(DataType minimum_value, DataType maximum_value) { //double minimum_value = 0.0, double maximum_value = 100.0) {
            DataType current_value = 0;
            if (this->getter_func!=nullptr) 
                current_value = (this->target->*getter_func)();
                //current_value_normal = (this->target->*getter_func)() / maximum_value;
            //this->setParamValue(current_value_normal);
            return this->initialise_values(minimum_value, maximum_value, current_value);
            //return this; //this->initialise_values(minimum_value, maximum_value, current_value_normal);
        }
        virtual DataParameter* initialise_values(DataType minimum_value, DataType maximum_value, DataType current_value) {
            // TODO: this probably isnt legal usage of NULL?
            //if (/*current_value_normal==NULL && */this->getter_func!=nullptr) 
                //current_value_normal = (this->target->*getter_func)() / maximum_value;
            this->minimumDataValue = minimum_value;
            this->maximumDataValue = maximum_value;
            this->currentDataValue = current_value;
            this->initialDataValue = current_value;

            this->minimumNormalValue = 0.0; //minimum_value;
            this->maximumNormalValue = 1.0; //maximum_value;
            this->currentNormalValue = this->dataToNormal(current_value); //(current_value - minimum_value) / (maximum_value - minimum_value);
            this->initialNormalValue = this->currentNormalValue;

            //this->updateValueFromNormal(current_value);
            return this;
        }

        virtual DataType normalToData(double value) {
            return this->minimumDataValue + (value * (this->maximumNormalValue - this->minimumDataValue));
        }
        virtual double dataToNormal(DataType value) {
            return (value - minimumDataValue) / (maximumDataValue - minimumDataValue);
            // eg   min = 0, max = 100, actual = 50 ->          ( 50 - 0 ) / (100-0)            = 0.5
            //      min = 0, max = 100, actual = 75 ->          ( 75 - 0 ) / (100-0)            = 0.75
            //      min = -100, max = 100, actual = 0 ->        (0 - -100) / (100--100)         = 0.5
            //      min = -100, max = 100, actual = -100 - >    (-100 - -100) / (100 - -100)    = 0
        }

        virtual DataType getCurrentDataValue() {
            return this->currentDataValue;
        }

        // setInitialValue in target value ie as a double to be multiplied by maximum_value
        virtual void setInitialValueFromNormal(double value) {
            this->currentNormalValue = value; // * this->maximumNormalValue;
            this->currentDataValue = this->normalToData(value); //->minimumDataValue + (value * this->maximumNormalValue);
            this->initialNormalValue = value;
            this->initialDataValue = this->currentDataValue;
        }
        virtual void setInitialValueFromData(DataType value) {
            this->currentNormalValue = this->initialNormalValue = this->dataToNormal(value); 
            this->currentDataValue = this->initialDataValue = value;
        }

        virtual void setInitialValue() {
            if (this->getter_func!=nullptr)
                this->setInitialValueFromData((this->target->*getter_func)());
        }

        // update internal param and call setter on target
        virtual void updateValueFromData(DataType value) {
            if (this->currentDataValue==value)
                return;

            if (this->debug) { 
                Serial.println("Parameter#updateValueFromData()"); Serial.flush(); 
            }

            this->lastDataValue = this->currentDataValue;
            this->lastNormalValue = this->lastNormalValue;
            this->currentDataValue = value;
            this->currentNormalValue = this->dataToNormal(value);

            this->setTargetValueFromData(value);
        }

        // update internal param and send to target
        virtual void updateValueFromNormal(double value) override { //}, double range=1.0) override {
            this->updateValueFromData(this->normalToData(value));
        }

        virtual DataType incrementDataValue(int value) {
            return constrain(++value, this->minimumDataValue, this->maximumDataValue);
        }
        virtual DataType incrementDataValue(float value) {
            return constrain(value + 0.1, this->minimumDataValue, this->maximumDataValue);
        }
        virtual DataType decrementDataValue(int value) {
            return constrain(--value, this->minimumDataValue, this->maximumDataValue);
        }
        virtual DataType decrementDataValue(float value) {
            return constrain(value - 0.1, this->minimumDataValue, this->maximumDataValue);
        }       

        #ifdef CORE_TEENSY
            // use Teensy version of this code that uses overriding functions
            /*virtual const char* getFormattedValue() override {
                const char *fmt = this->parseFormattedDataType((DataType)this->getCurrentDataValue());
                //Serial.printf("getFormattedValue: '%s'\n", fmt);
                return fmt;
            };
            virtual const char* parseFormattedDataType(bool value) {
                static char fmt[20] = "              ";
                sprintf(fmt, "%s", this->getCurrentNormalValue() ? "On" : "Off");
                return fmt;
            }
            virtual const char* parseFormattedDataType(double value) {
                static char fmt[20] = "              ";
                sprintf(fmt, "%3i%% (float)",     (int)(100.0*this->getCurrentNormalValue())); //->getCurrentValue());
                return fmt;
            }
            virtual const char* parseFormattedDataType(unsigned int value) {
                static char fmt[20] = "              ";
                sprintf(fmt, "%5u (unsigned)",    (unsigned int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
                return fmt;
            }
            virtual const char* parseFormattedDataType(int value) {
                static char fmt[20] = "              ";
                sprintf(fmt, "%5i (signed)",      (int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
                return fmt;
            }*/
            virtual const char* getFormattedValue() override {
                const char *fmt = this->parseFormattedDataType((DataType)this->getCurrentDataValue());
                //Serial.printf("getFormattedValue: '%s'\n", fmt);
                return fmt;
            };
            virtual const char* parseFormattedDataType(bool value) {
                static char fmt[20] = "              ";
                sprintf(fmt, "%s", value ? "On" : "Off");
                return fmt;
            }
            virtual const char* parseFormattedDataType(double value) {
                static char fmt[20] = "              ";
                sprintf(fmt, "%3i%% (float)",     (int)(100.0*value)); //->getCurrentValue());
                return fmt;
            }
            virtual const char* parseFormattedDataType(unsigned int value) {
                static char fmt[20] = "              ";
                sprintf(fmt, "%5u (unsigned)",   value); // (unsigned int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
                return fmt;
            }
            virtual const char* parseFormattedDataType(int value) {
                static char fmt[20] = "              ";
                sprintf(fmt, "%5i (signed)",     value); // (int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
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

        // actually set the target from data, do the real setter call on target
        virtual void setTargetValueFromData(DataType value) {
            if (this->target!=nullptr && this->setter_func!=nullptr) {
                #ifdef ENABLE_PRINTF
                    if (this->debug) {
                        Serial.println("Parameter#updateValueFromNormal()"); Serial.flush();
                        Serial.printf("%s: Calling setter func for value (", this->label);
                        Serial.print(value);
                        Serial.println(")");
                    }
                #endif   
                (this->target->*setter_func)((DataType)value);
            } else {
                #ifdef ENABLE_PRINTF
                    Serial.printf("WARNING: no target / no setter_func in %s!\n", this->label);
                #endif
            }
        }
        // actually set the target from normal
        virtual void setTargetValueFromNormal(double value) {
            this->setTargetValueFromData(this->normalToData(value));
        }

        virtual void set_target_object(TargetClass *target) {
            this->target = target;
        }
        virtual void set_target_func(void(TargetClass::*fp)(DataType)) {
            this->setter_func = fp;
        }
};



#endif