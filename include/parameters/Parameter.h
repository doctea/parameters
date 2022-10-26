#ifndef PARAMETER__INCLUDED
#define PARAMETER__INCLUDED

#include <Arduino.h>
#ifdef CORE_TEENSY
    // use alternative method if Teensy
#else
    #include <ArxTypeTraits.h>
#endif
//#include <stddef.h>

#include <LinkedList.h>

extern char NEXT_PARAMETER_NAME;

//#include "ParameterInput.h"

class BaseParameterInput;
//#include "parameter_inputs/ParameterInput.h"

#define MAX_SLOT_CONNECTIONS 3

struct ParameterToInputConnection {
    //BaseParameter *parameter = nullptr;
    BaseParameterInput *parameter_input = nullptr;
    double amount = 0.0f;
    //bool volt_per_octave = false;
};

#ifdef ENABLE_SCREEN
    class MenuItem;
#endif

class BaseParameter { 
    public:
        bool debug = false;

        char label[20];

        BaseParameter(char *label) {
            strcpy(this->label, label);
        };
        virtual void updateValueFromNormal(double value/*, double range = 1.0*/) {};
        virtual void modulateValue(double value) {};
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
    double maximumNormalValue = 1.0; //100.0;

    double lastModulatedNormalValue = 0.0;
    double lastOutputNormalValue = 0.0;

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

    virtual const char* getFormattedValue(double value) {
        Serial.printf(F("WARNING: dummy DoubleParameter#getFormattedValue(%f) for '%s'\n"), value, this->label);
        return "[NaN]";
    }
    virtual const char* getFormattedValue() {
        return this->getFormattedValue(this->currentNormalValue);
    }
    virtual const char* getFormattedLastOutputValue() {
        return this->getFormattedValue(this->lastOutputNormalValue);
    }

    virtual void updateValueFromNormal(double value/*, double range = 1.0*/) override {
        // TODO: we might actually want this to do something?
        Serial.printf(F("WARNING: dummy DoubleParameter#updateValueFromNormal(%f) for '%s'\n"), value, this->label);
    };

    virtual void on_unbound(BaseParameterInput *input) {
        // TODO: maybe we don't always want to set the value to what it was before we started?
        //      this is only really useful for parameters that modulate or offset or parameters...?
        this->updateValueFromNormal(this->initialNormalValue);
    }

    virtual void incrementValue() {
        Serial.printf(F("WARNING: dummy DoubleParameter#incrementValue() for '%s'\n"), this->label);
    }
    virtual void decrementValue() {
        Serial.printf(F("WARNING: dummy DoubleParameter#decrementValue() for '%s'\n"), this->label);
    }

    // parameter input mixing / modulation stuff
    ParameterToInputConnection connections[MAX_SLOT_CONNECTIONS];

    virtual int find_empty_slot() {
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

    virtual char get_input_name_for_slot(byte slot);

    virtual void set_slot_input(byte slot, char parameter_input_name);
    virtual void set_slot_input(byte slot, BaseParameterInput *parameter_input);
    virtual void set_slot_amount(byte slot, double amount) {
        this->connections[slot].amount = amount;
    }

    virtual void set_slot_0_input(BaseParameterInput *parameter_input) {
        this->set_slot_input(0,parameter_input);
    }
    virtual void set_slot_0_amount(double amount) {
        this->set_slot_amount(0, amount);
    }
    virtual void set_slot_1_input(BaseParameterInput *parameter_input) {
        this->set_slot_input(1,parameter_input);
    }
    virtual void set_slot_1_amount(double amount) {
        this->set_slot_amount(1, amount);
    }
    virtual void set_slot_2_input(BaseParameterInput *parameter_input) {
        this->set_slot_input(2,parameter_input);
    }
    virtual void set_slot_2_amount(double amount) {
        this->set_slot_amount(2, amount);
    }

    virtual void reset_mappings() {
        this->set_slot_0_amount(0.0);
        this->set_slot_1_amount(0.0);
        this->set_slot_2_amount(0.0);
    }

    /*virtual char get_connection_slot_name(int slot) {
        return this->connections[slot].parameter_input!=nullptr ? 
               this->connections[slot].parameter_input->name : // todo: doesnt compile (incomplete type BaseParameterInput) -- make getting the connection name into a method of Parameter?
               'X';   // use X instead of parameter name if no parameter label is set for that parameter
    }*/

    /*void changeValue(BaseParameterInput *parameter_input) {
        // find the parameterinput we've been passed
        // update and send the actual value
    }*/

    // calculate the modulation value based on the inputs * modulation amounts
    virtual double get_modulation_value();

    #ifdef ENABLE_SCREEN
        virtual MenuItem *makeControl();
        virtual LinkedList<MenuItem *> *makeControls();
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
            if (this->debug && value>=0.0) {
                Serial.printf(F("%s#"), this->label);
                Serial.printf(F("normalToData(%f) "), value);
                Serial.printf(F(", range is %i to %i "), this->minimumDataValue, this->maximumDataValue);
            }
            value = this->constrainNormal(value);
            DataType data = this->minimumDataValue + (value * (float)(this->maximumDataValue - this->minimumDataValue));
            if (this->debug && value>=0.0) Serial.printf(" => %i\n", data);
            return data;
        }
        virtual double dataToNormal(DataType value) {
            if (this->debug) Serial.printf(F("dataToNormal(%i) "), value);
            double normal = (double)(value - minimumDataValue) / (double)(maximumDataValue - minimumDataValue);
            if (this->debug) Serial.printf(F(" => %f\n"), normal);
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
                Serial.printf(F("Parameter#updateValueFromData(%i)\n"), value); Serial.flush(); 
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
            //static double lastModulationNormalValue = 0.0;
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
            if (this->debug) Serial.printf(F("\tin %s, got modulated value to set: %f\n"), this->label, value);
            this->setTargetValueFromNormal(value);
        }

        // update internal param and send to target
        virtual void updateValueFromNormal(double value) override { //}, double range=1.0) override {
            if (this->debug) Serial.printf(F("updateValueFromNormal(%f)\n"), value);
            this->updateValueFromData((DataType)this->normalToData(value));
        }

        virtual void modulateValue(double value) override {
            this->modulateNormalValue = value;
            //this->updateValueFromNormal(this->currentNormalValue);
        }

        // increment the value and update
        virtual void incrementValue() override {
            //this->debug = true;
            if (this->debug) Serial.printf(F("Parameter#incrementValue() for '%s', normal %f, data %i about to call updateValueFromData()....\n"), this->label, this->getCurrentNormalValue(), this->getCurrentDataValue());
            this->updateValueFromData(this->incrementDataValue((DataType)this->getCurrentDataValue()));
            if (this->debug) Serial.printf(F("....Parameter#incrementValue() value became normal %f, data %i\n"),this->getCurrentNormalValue(),this->getCurrentDataValue());
        }
        // decrement the value and update
        virtual void decrementValue() override {
            //this->debug = true;
            if (this->debug) Serial.printf(F("Parameter#decrementValue() for '%s', initial FormattedValue '%s' (normal %f)"), this->label, this->getFormattedValue(this->getCurrentDataValue()), this->getCurrentNormalValue());
            this->updateValueFromData(this->decrementDataValue((DataType)this->getCurrentDataValue()));
            if (this->debug) Serial.printf(F("became '%s' (normal %f)\n"), this->getFormattedValue(this->getCurrentDataValue()), this->getCurrentNormalValue());
        }

        // returns an incremented DataType version of input value (int)
        virtual DataType incrementDataValue(int value) {
            //if (this->debug) Serial.printf("Parameter#incrementDataValue(%i)..\n", value);
            int new_value = constrain(++value, this->minimumDataValue, this->maximumDataValue);
            //if (this->debug) Serial.printf("became %i (after constrain to %i:%i)..\n", new_value, this->minimumDataValue, this->maximumDataValue);
            return new_value;
        }
        // returns a decremented DataType version of input value (int)
        virtual DataType decrementDataValue(int value) {
            return constrain(--value, this->minimumDataValue, this->maximumDataValue);
        }
        // returns an incremented DataType version of input value (float)
        virtual DataType incrementDataValue(float value) {
            return constrain(value + 0.1, this->minimumDataValue, this->maximumDataValue);
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
                return value ? "On" : "Off";
            }
            virtual const char* parseFormattedDataType(double value) {
                static char fmt[10] = "         ";
                //sprintf(fmt, "%3i%% (float)",     (int)(100.0*value)); //->getCurrentValue());
                sprintf(fmt, "%3i%%", (int)(100.0*value)); //->getCurrentValue());
                return fmt;
            }
            virtual const char* parseFormattedDataType(unsigned int value) {
                static char fmt[10] = "         ";
                //sprintf(fmt, "%5u (unsigned)",   value); // (unsigned int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
                sprintf(fmt, "%5u",   value); // (unsigned int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
                return fmt;
            }
            virtual const char* parseFormattedDataType(int value) {
                static char fmt[10] = "         ";
                //sprintf(fmt, "%5i (signed)",     value); // (int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
                sprintf(fmt, "%5i",   value); // (int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
                return fmt;
            }
            virtual const char* parseFormattedDataType(byte value) {
                static char fmt[10] = "         ";
                //sprintf(fmt, "%5i (signed)",     value); // (int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
                sprintf(fmt, "%3i",  value); // (int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
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

        // actually set the target from data, do the REAL setter call on target
        virtual void setTargetValueFromData(DataType value, bool force = false) {
            // early return if this value is the same as last one and we aren't being asked to force it
            static DataType last_sent_value = -1;
            if (!force && last_sent_value==value)
                return;

            if (this->target!=nullptr && this->setter_func!=nullptr) {
                #ifdef ENABLE_PRINTF
                    if (this->debug) {
                        Serial.println(F("Parameter#setTargetValueFromData()")); Serial.flush();
                        Serial.printf(F("%s: Calling setter func for value ("), this->label);
                        Serial.print(value);
                        Serial.println(')');
                    }
                #endif   
                (this->target->*setter_func)((DataType)value);
            } else {
                #ifdef ENABLE_PRINTF
                    Serial.printf(F("WARNING: no target / no setter_func in %s!\n"), this->label);
                #endif
            }
        }
        // set the target from normalised post-modulation value
        virtual void setTargetValueFromNormal(double value, bool force = false) {
            value = this->constrainNormal(value);
            this->lastModulatedNormalValue = value;
            this->lastOutputNormalValue = this->lastModulatedNormalValue; // = value;
            this->setTargetValueFromData(this->normalToData(lastOutputNormalValue), force);
        }

        virtual double constrainNormal(double value) {
            // TODO: check if polar/bipolar?
            /*if (this->debug) Serial.printf("in %s,\tconstraining %f to %f:%f => %f \n", 
                this->label, 
                value, 
                this->minimumNormalValue, 
                this->maximumNormalValue, 
                constrain(value, this->minimumNormalValue, this->maximumNormalValue)
            );*/
            return constrain(value, this->minimumNormalValue, this->maximumNormalValue);
        }

        virtual void set_target_object(TargetClass *target) {
            this->target = target;
        }
        virtual void set_target_func(void(TargetClass::*fp)(DataType)) {
            this->setter_func = fp;
        }
};

#endif