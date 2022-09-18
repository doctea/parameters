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

#define MAX_SLOT_CONNECTIONS 3

struct ParameterToInputConnection {
    //BaseParameter *parameter = nullptr;
    BaseParameterInput *parameter_input = nullptr;
    double amount = 0.0f;
    //bool volt_per_octave = false;
};


//class ParameterMixer;
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
        virtual void modulateValue(double value) {};
        /*virtual DataType getCurrentValue() {};
        virtual DataType getLastValue() {};*/
        virtual const char* getFormattedValue() {
            //static char noval = "[none]";
            return "[none]";
        };
        virtual void on_unbound(BaseParameterInput*) {}

        virtual void update_mixer() {}

        virtual bool connect_input(BaseParameterInput*, double amount) {
            return false;
        }

        // called when a BaseParameterInput that was targetting this item release control of this parameter
};

class DoubleParameter : public BaseParameter {
    public: 
    
    double currentNormalValue = 0.0;
    double lastNormalValue = 0.0;
    double initialNormalValue = 0.0;
    double minimumNormalValue = 0.0;
    double maximumNormalValue = 100.0;

    double lastModulatedNormalValue = 0.0;

    DoubleParameter(char *label) : BaseParameter(label) {}

    virtual double getCurrentNormalValue() {
        return this->currentNormalValue;
    }
    virtual double getLastNormalValue() {
        return this->lastNormalValue;
    }
    virtual double getLastModulatedNormalValue() {
        return this->lastModulatedNormalValue;
    }

    /*virtual int getCurrentDataValue() {
        return round(this->currentNormalValue * 100.0);
    }*/
    virtual const char* getFormattedValue(double value) {
        Serial.printf("WARNING: dummy DoubleParameter#getFormattedValue(%f) for '%s'\n", value, this->label);
        return "[NaN]";
    }
    virtual const char* getFormattedValue() {
        return getFormattedValue(currentNormalValue);
    }

    virtual void updateValueFromNormal(double value/*, double range = 1.0*/) override {
        // TODO: we might actually want this to do something?
        Serial.printf("WARNING: dummy DoubleParameter#updateValueFromNormal(%f) for '%s'\n", value, this->label);
    };

    virtual void on_unbound(BaseParameterInput *input) {
        // TODO: maybe we don't always want to set the value to what it was before we started?
        //      this is only really useful for parameters that modulate or offset or parameters...?
        this->updateValueFromNormal(this->initialNormalValue);
    }

    virtual void incrementValue() {
        Serial.printf("WARNING: dummy DoubleParameter#incrementValue() for '%s'\n", this->label);
    }
    virtual void decrementValue() {
        Serial.printf("WARNING: dummy DoubleParameter#decrementValue() for '%s'\n", this->label);
    }

    // parameter input mixing / modulation stuff
    ParameterToInputConnection connections[MAX_SLOT_CONNECTIONS];
    
    int find_empty_slot() {
        for (int i = 0 ; i < MAX_SLOT_CONNECTIONS ; i++) {
            if (connections[i].parameter_input==nullptr)
                return i;
        }
        return -1;
    }
    virtual bool connect_input(BaseParameterInput *parameter_input, double amount) {
        int slot = find_empty_slot();
        if (slot==-1) 
            return false;

        set_slot_input(slot, parameter_input);
        set_slot_amount(slot, amount);

        return true;
    }
    virtual bool disconnect_input(byte slot) { 
        this->connections[slot].parameter_input = nullptr;
        return true;
    }

    void set_slot_input(byte slot, BaseParameterInput *parameter_input) {
        this->connections[slot].parameter_input = parameter_input;
    }
    void set_slot_amount(byte slot, double amount) {
        this->connections[slot].amount = amount;
    }

    void set_slot_0_input(BaseParameterInput *parameter_input) {
        this->set_slot_input(0,parameter_input);
    }
    void set_slot_0_amount(double amount) {
        this->set_slot_amount(0, amount);
    }
    void set_slot_1_input(BaseParameterInput *parameter_input) {
        this->set_slot_input(0,parameter_input);
    }
    void set_slot_1_amount(double amount) {
        this->set_slot_amount(0, amount);
    }
    void set_slot_2_input(BaseParameterInput *parameter_input) {
        this->set_slot_input(0,parameter_input);
    }
    void set_slot_2_amount(double amount) {
        this->set_slot_amount(0, amount);
    }

    /*void changeValue(BaseParameterInput *parameter_input) {
        // find the parameterinput we've been passed
        // update and send the actual value
    }*/

    double get_modulation_value();/* {
        // get the modulation amount to use
        double modulation = 0.0f;
        for (int i = 0 ; i < MAX_CONNECTIONS ; i++) {
            if (this->connections[i].parameter_input!=nullptr)
                modulation += (
                    this->connections[i].parameter_input->get_normal_value() * this->connections[i].amount
                );
        }
        return modulation;
        //this->parameter->modulateValue(modulation);
    }*/

    #ifdef ENABLE_SCREEN
    virtual MenuItem *makeControl();
    #endif
};



// an object that can be targeted by a ParameterInput, calls setter method on final target object
template<class TargetClass, class DataType = double>
class DataParameter : public DoubleParameter {
    public:

        DataType minimumDataValue = 0.0;
        DataType maximumDataValue = 100.0;
        DataType lastDataValue = 0;
        DataType currentDataValue = 0;
        DataType initialDataValue = 0;

        double modulateNormalValue = 0.0;

        TargetClass *target;
        void(TargetClass::*setter_func)(DataType value) = nullptr;// setter_func;
        DataType(TargetClass::*getter_func)() = nullptr;// setter_func;

        //ParameterMixer *mixer = nullptr;

        DataParameter(char *label, TargetClass *target) : DoubleParameter(label) {
            this->target = target;
            //this->mixer = new ParameterMixer(); //this);
        }
        DataParameter(char *label, TargetClass *target, DataType initial_value_normal) : DoubleParameter(label, target) {
            this->initialNormalValue = initial_value_normal;
        }
        DataParameter(char *label, TargetClass *target, void(TargetClass::*setter_func)(DataType)) : DoubleParameter(label, target) {
            this->setter_func = setter_func;
        }
        DataParameter(char *label, TargetClass *target, void(TargetClass::*setter_func)(DataType), DataType(TargetClass::*getter_func)()) : DoubleParameter(label, target, setter_func) {
            this->getter_func = getter_func;
            //if (getter_func!=nullptr)
            //    this->setInitialValue();
        }
        DataParameter(char *label, TargetClass *target, double initial_value_normal, void(TargetClass::*setter_func)(DataType)) : DoubleParameter(label, target, setter_func) {
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
            if (this->debug) Serial.printf("normalToData(%f) ", value);
            if (this->debug) Serial.printf(", range is %i ", this->maximumDataValue - this->minimumDataValue);
            DataType data = this->minimumDataValue + (value * (float)(this->maximumDataValue - this->minimumDataValue));
            if (this->debug) Serial.printf(" => %i\n", data);
            return data;
        }
        virtual double dataToNormal(DataType value) {
            if (this->debug) Serial.printf("dataToNormal(%i) ", value);
            double normal = (double)(value - minimumDataValue) / (double)(maximumDataValue - minimumDataValue);
            if (this->debug) Serial.printf(" => %f\n", normal);
            return normal;
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
            if (this->debug) { 
                Serial.printf("Parameter#updateValueFromData(%i)\n", value); Serial.flush(); 
            }

            if (this->currentDataValue==value)
                return;

            this->lastDataValue = this->currentDataValue;
            this->lastNormalValue = this->lastNormalValue;

            this->currentDataValue = value;
            this->currentNormalValue = this->dataToNormal((DataType)value);

            this->sendCurrentTargetValue();
        }

        virtual void update_mixer() {
            //this->mixer->updateOutput();
            static double lastModulationNormalValue = 0.0;
            this->modulateNormalValue = this->get_modulation_value();
            if (modulateNormalValue!=lastModulatedNormalValue) {
                this->sendCurrentTargetValue();
                lastModulatedNormalValue = modulateNormalValue;
            }
        }

        /*virtual void connect_input(BaseParameterInput* input, double amount = 1.0) {
            this->connect(input, amount);
        }
        virtual void disconnect_input(BaseParameterInput* input) {
            this->disconnect(input);
        }*/

        virtual void sendCurrentTargetValue() {
            double value = this->currentNormalValue + this->modulateNormalValue;
            Serial.printf("\tgot modulated value to set: %f\n", value);
            this->setTargetValueFromNormal(value);
        }

        // update internal param and send to target
        virtual void updateValueFromNormal(double value) override { //}, double range=1.0) override {
            if (this->debug) Serial.printf("updateValueFromNormal(%f)\n", value);
            this->updateValueFromData((DataType)this->normalToData(value));
        }

        virtual void modulateValue(double value) override {
            this->modulateNormalValue = value;
            //this->updateValueFromNormal(this->currentNormalValue);
        }

        // increment the value and update
        virtual void incrementValue() override {
            this->debug = true;
            if (this->debug) Serial.printf(
                //"Parameter#incrementValue() for '%s', initial FormattedValue '%s' (normal %f)", 
                "Parameter#incrementValue() for '%s', normal %f, data %i about to call updateValueFromData()....\n", 
                this->label, 
                //this->getFormattedValue(this->getCurrentDataValue()), 
                this->getCurrentNormalValue(),
                this->getCurrentDataValue()
            );
            this->updateValueFromData(
                this->incrementDataValue(
                    (DataType)this->getCurrentDataValue()
                )
            );
            if (this->debug) Serial.printf(
                //"became '%s' (normal %f)\n",
                "....Parameter#incrementValue() value became normal %f, data %i\n",
                //this->getFormattedValue(this->getCurrentDataValue()), 
                this->getCurrentNormalValue(),
                this->getCurrentDataValue()
            );
        }
        // decrement the value and update
        virtual void decrementValue() override {
            this->debug = true;
            //if (this->debug) Serial.printf("Parameter#decrementValue() for '%s', initial FormattedValue '%s' (normal %f)", this->label, this->getFormattedValue(this->getCurrentDataValue()), this->getCurrentNormalValue());
            this->updateValueFromData(this->decrementDataValue((DataType)this->getCurrentDataValue()));
            //if (this->debug) Serial.printf("became '%s' (normal %f)\n", this->getFormattedValue(this->getCurrentDataValue()), this->getCurrentNormalValue());
        }

        // returns an incremented DataType version of input value (int)
        virtual DataType incrementDataValue(int value) {
            Serial.printf("Parameter#incrementDataValue(%i)..\n", value);
            int new_value = constrain(++value, this->minimumDataValue, this->maximumDataValue);
            Serial.printf("became %i (after constrain to %i:%i)..\n", new_value, this->minimumDataValue, this->maximumDataValue);
            return new_value;
        }
        // returns an incremented DataType version of input value (float)
        virtual DataType incrementDataValue(float value) {
            return constrain(value + 0.1, this->minimumDataValue, this->maximumDataValue);
        }
        // returns a decremented DataType version of input value (int)
        virtual DataType decrementDataValue(int value) {
            return constrain(--value, this->minimumDataValue, this->maximumDataValue);
        }
        // returns a decremented DataType version of input value (float)
        virtual DataType decrementDataValue(float value) {
            return constrain(value - 0.1, this->minimumDataValue, this->maximumDataValue);
        }       

        #ifdef CORE_TEENSY
            // use Teensy version of this code that uses overriding functions instead of ArxTypeTraits/constexpr to render values
            virtual const char* getFormattedValue(double normal) override {
                const char *fmt = this->parseFormattedDataType(this->normalToData(normal));
                //Serial.printf("getFormattedValue: '%s'\n", fmt);
                return fmt;
            };
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
                //sprintf(fmt, "%3i%% (float)",     (int)(100.0*value)); //->getCurrentValue());
                sprintf(fmt, "%3i%%",     (int)(100.0*value)); //->getCurrentValue());
                return fmt;
            }
            virtual const char* parseFormattedDataType(unsigned int value) {
                static char fmt[20] = "              ";
                //sprintf(fmt, "%5u (unsigned)",   value); // (unsigned int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
                sprintf(fmt, "%5u",   value); // (unsigned int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
                return fmt;
            }
            virtual const char* parseFormattedDataType(int value) {
                static char fmt[20] = "              ";
                //sprintf(fmt, "%5i (signed)",     value); // (int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
                sprintf(fmt, "%5i",     value); // (int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
                return fmt;
            }
        #else
            // use version for 1284p that utilise ArxTypeTraits and constexpr to render values
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
                        Serial.println("Parameter#setTargetValueFromData()"); Serial.flush();
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
            this->lastModulatedNormalValue = this->constrainNormal(value);
            this->setTargetValueFromData(this->normalToData(value));
        }

        virtual double constrainNormal(double value) {
            // todo: check if polar/bipolar
            return constrain(value, 0.0, 1.0);
        }

        virtual void set_target_object(TargetClass *target) {
            this->target = target;
        }
        virtual void set_target_func(void(TargetClass::*fp)(DataType)) {
            this->setter_func = fp;
        }
};



#endif