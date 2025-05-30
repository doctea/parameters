#pragma once

#ifndef MODULATION_THRESHOLD
    //#define MODULATION_THRESHOLD 0.02
    #define MODULATION_THRESHOLD 0.01
#endif

#include "ParameterTypes.h"

#include <Arduino.h>
#if defined(CORE_TEENSY)
    // use alternative method if Teensy - no ARXTypeTraits, FLASHMEM
#elif defined(ARDUINO_ARCH_RP2040)
    // use alternative method if Raspberry Pico - ARXTypeTraits, no FLASHMEM define
    #ifndef FLASHMEM
        #define FLASHMEM
    #endif
    /*#ifndef USE_ARX_TYPE_TRAITS
        #define USE_ARX_TYPE_TRAITS
    #endif*/
#else
    // default arduino - ARXTypeTraits, no FLASHMEM 
    #ifndef FLASHMEM
        #define FLASHMEM
    #endif
    #ifndef USE_ARX_TYPE_TRAITS
        #define USE_ARX_TYPE_TRAITS
    #endif
#endif

#ifdef USE_ARX_TYPE_TRAITS
    #include <ArxTypeTraits.h>
#endif

#include "debug.h"

#include <LinkedList.h>

#include "calibration.h"

//extern char NEXT_PARAMETER_NAME;

//#include "ParameterInput.h"
class BaseParameterInput;
class BaseParameter;
#ifdef ENABLE_SCREEN
    class MenuItem;
    template<class DataType> class SelectorControl;
    class ParameterMenuItem;
    //, ParameterInputSelectorControl;
#endif

#define MAX_SLOT_CONNECTIONS 3

struct ParameterToInputConnection {
    //BaseParameter *parameter = nullptr;
    BaseParameterInput *parameter_input = nullptr;
    float amount = 0.0f;
    /*#ifdef ENABLE_SCREEN
        // done more directly? todo: add colour, and update the colour of the widget too
        // link the parameter mapping back to the screen controls, so that we can update the screen when mapping changes
        MenuItem *amount_control = nullptr;
        SelectorControl<int> *input_control = nullptr;
    #endif*/
    byte polar_mode = UNIPOLAR;
    //bool volt_per_octave = false;
};

const size_t MAX_PARAMETER_NAME_LENGTH = 20;

class BaseParameter { 
    public:
        bool debug = false;

        const char *label_on = "On";
        const char *label_off = "Off";

        char label[MAX_PARAMETER_NAME_LENGTH];

        BaseParameter(const char *label) {
            strncpy(this->label, label, MAX_PARAMETER_NAME_LENGTH);
        };
        virtual void updateValueFromNormal(float value/*, float range = 1.0*/) {};
        virtual void modulateValue(float value) {};
        virtual const char* getFormattedValue() {
            //static char noval = "[none]";
            return "[none]";
        };
        // called when a BaseParameterInput that was targetting this item release control of this parameter
        virtual void on_unbound(BaseParameterInput*) {}

        virtual void update_mixer() {}

        virtual bool connect_input(BaseParameterInput*, float amount) {
            return false;
        }

        virtual void process_pending() {}

        virtual void save_pattern_add_lines(LinkedList<String> *lines) {}

        virtual void load_calibration() {}  // todo: the only reason this is here is because of CVOutputParameter at the moment; should probably be moved to ICalibratable and work out a way to do this better
        virtual bool needs_calibration() {
            return false;
        }
        virtual void output_calibration_data() {
            Serial.printf("BaseParameter calibration data for %s: no calibration data\n", this->label);
        }

        #ifdef ENABLE_SCREEN
            virtual LinkedList<MenuItem*> *makeCalibrationControls() { return nullptr; }
            virtual MenuItem *makeCalibrationLoadSaveControls() { return nullptr; }
        #endif
};

// floattype-backed Parameter class from which usable types descend
class FloatParameter : public BaseParameter {
    public: 
    
    float currentNormalValue = 0.0f;
    float lastNormalValue = 0.0f;
    float initialNormalValue = 0.0f;
    float minimumNormalValue = 0.0f;
    float maximumNormalValue = 1.0f; //100.0;

    float lastModulatedNormalValue = 0.0f;
    float lastOutputNormalValue = 0.0f;

    float lastRealOutputNormalValue = 0.0f;

    char float_unit = '%';

    FloatParameter *self = this;

    FloatParameter(const char *label) : BaseParameter(label) {
        self = this;
    }

    ////// slew rate stuff
    uint32_t last_slewed_at = 0;
    float last_slewed_value = 0.0f;

    bool slew_enabled = false;      // enable/disable slewing
    bool slewing = false;           // track whether we are currently slewing or not

    // actual slew values - amount of change per millisecond (then divided by 10)
    const float slowest_slew_rate = 0.001f;
    const float fastest_slew_rate = 0.1f;
    float slew_rate = (fastest_slew_rate) / 10.0f;

    // normalised value for use by menus / modulation
    float slew_rate_normal = 1.0f;

    // weirdly, having this here seems to cause lockups when usb is connected..?
    /*virtual void set_slew_rate(float slew_rate) {
        //this->slew_rate = slew_rate;
        this->slew_rate = map(slew_rate, 0.0f, 1.0f, slowest_slew_rate, fastest_slew_rate) / 10.0f;
    }*/

    virtual float get_slew_rate_normal() {
        return slew_rate_normal;
    }
    virtual void set_slew_rate_normal(float slew_rate_normal) {
        this->slew_rate_normal = slew_rate_normal;
        this->slew_rate = map(slew_rate_normal, 0.0f, 1.0f, slowest_slew_rate, fastest_slew_rate) / 100.0f;
        if (debug && Serial) Serial.printf("%s#set_slew_rate_normal(%3.3f) - slew_rate is %3.3f\n", this->label, slew_rate_normal, this->slew_rate);
    }
    
    virtual float get_slewed_value(float normal) {
        if (!slew_enabled || slew_rate_normal>=1.0f) {
            last_slewed_value = normal;
            return normal;
        }
        uint32_t delta = millis() - last_slewed_at;
        last_slewed_at = millis();

        if (debug && Serial) Serial.printf("%s#get_slewed_value() - slewing enabled, slew_rate is %3.3f, slew_rate_normal is %3.3f\n", this->label, this->slew_rate, this->slew_rate_normal);

        float slew_rate = this->slew_rate * (float)delta;
        float diff = normal - this->lastRealOutputNormalValue;
        
        if (debug) {
            Serial.printf("%s#get_slewed_value()\ttarget value=%3.4f\t - lastOutputNormalValue=%3.4f\t => diff=%3.4f\n", this->label, normal, this->lastRealOutputNormalValue, diff);
            //Serial.printf("%s#get_slewed_value()\tnormal=%3.4f,\tlastOutputNormalValue=%3.4f,\tdiff=%3.4f,\tslew_rate=%3.4f\n", this->label, normal, lastOutputNormalValue, diff, slew_rate);
        }

        if (abs(diff)>0.0)
            if (debug) Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! diff is non-zero!");

        if (debug && Serial)
            Serial.printf("%s#get_slewed_value()\tnormal=%3.4f,\tlast_slewed_value=%3.4f,\tdiff=%3.4f,\tslew_rate=%3.4f\n", this->label, normal, last_slewed_value, diff, slew_rate);

        if (diff > slew_rate) {
            slewing = true;
            if (debug && Serial) Serial.printf("slewing up:\t%3.4f + %3.4f -> %3.4f\n", lastRealOutputNormalValue, slew_rate, lastOutputNormalValue + slew_rate);
            normal = lastRealOutputNormalValue + slew_rate;
        } else if (diff < -slew_rate) {
            slewing = true;
            if (debug && Serial) Serial.printf("slewing down:\t%3.4f - %3.4f -> %3.4f\n", lastRealOutputNormalValue, slew_rate, lastOutputNormalValue - slew_rate);
            normal = lastRealOutputNormalValue - slew_rate;
        } else 
            slewing = false;

        if (debug && Serial) {
            Serial.printf("%s#get_slewed_value()\treturning %f\n", this->label, normal);
        }
        last_slewed_value = normal;
        return normal;
    }

    /// value constraining/range limiting stuff
    virtual float getMinimumDataLimit() {
        return minimumNormalValue;
    }
    virtual float getMaximumDataLimit() {
        return maximumNormalValue;
    }

    virtual float getCurrentNormalValue() {
        return this->currentNormalValue;
    }
    virtual float getLastNormalValue() {
        return this->lastNormalValue;
    }
    virtual float getLastModulatedNormalValue() {
        return this->lastModulatedNormalValue;
    }

    virtual const char* getFormattedValue(float value) {
        //Serial.printf(F("WARNING: dummy FloatParameter#getFormattedValue(%f) for '%s'\n"), value, this->label);
        return "[NaN]";
    }
    virtual const char* getFormattedValue() {
        return this->getFormattedValue(this->getCurrentNormalValue());
    }
    virtual const char* getFormattedLastOutputValue() {
        return this->getFormattedValue(this->lastOutputNormalValue);
    }

    virtual const char *getFormattedLimit(float range) {
        //return parseFormattedDataType(range);
        return "[n/a]";
    }

    virtual void updateValueFromNormal(float value/*, float range = 1.0*/) override {
        // TODO: we might actually want this to do something?
        Serial.printf((const char*)F("WARNING: dummy FloatParameter#updateValueFromNormal(%f) for '%s'\n"), value, this->label);
    };

    virtual void on_unbound(BaseParameterInput *input) {
        // TODO: maybe we don't always want to set the value to what it was before we started?
        //      this is only really useful for parameters that modulate or offset or parameters...?
        this->updateValueFromNormal(this->initialNormalValue);
    }

    virtual void incrementValue() {
        Serial.printf((const char*)F("WARNING: dummy FloatParameter#incrementValue() for '%s'\n"), this->label);
    }
    virtual void decrementValue() {
        Serial.printf((const char*)F("WARNING: dummy FloatParameter#decrementValue() for '%s'\n"), this->label);
    }

    // parameter input mixing / modulation stuff
    ParameterToInputConnection connections[MAX_SLOT_CONNECTIONS];

    virtual bool is_valid_slot(int8_t slot) {
        return(slot>=0 && slot < MAX_SLOT_CONNECTIONS);
    }

    virtual int find_empty_slot() {
        for (unsigned int i = 0 ; i < MAX_SLOT_CONNECTIONS ; i++) {
            if (connections[i].parameter_input==nullptr)
                return i;
        }
        return -1;
    }
    // TODO: verify if this is actually working properly?!
    virtual int find_connected_or_next_empty_slot(BaseParameterInput *parameter_input) {
        for (unsigned int i = 0 ; i < MAX_SLOT_CONNECTIONS ; i++) {
            if (connections[i].parameter_input==parameter_input)
                return i;
        }
        return find_empty_slot();
    }
    virtual bool connect_input(BaseParameterInput *parameter_input, float amount) {
        //int slot = find_empty_slot();
        int slot = find_connected_or_next_empty_slot(parameter_input);
        if (!is_valid_slot(slot))
            return false;

        set_slot_input(slot, parameter_input);
        set_slot_amount(slot, amount);

        return true;
    }
    virtual bool connect_input(int slot_number, float amount) {
        if (!is_valid_slot(slot_number))
            return false;
        set_slot_amount(slot_number, amount);
        return true;
    }
    virtual bool disconnect_input(int8_t slot) { 
        if (!is_valid_slot(slot))
            return false;
        this->connections[slot].parameter_input = nullptr;
        return true;
    }

    virtual const char *get_input_name_for_slot(int8_t slot);
    float get_amount_for_slot(int8_t slot);

    BaseParameterInput *get_slot_input(int slot) {
        return this->connections[slot].parameter_input;
    }
    BaseParameterInput *get_slot_0_input() {
        return this->get_slot_input(0);
    }
    BaseParameterInput *get_slot_1_input() {
        return this->get_slot_input(1);
    }
    BaseParameterInput *get_slot_2_input() {
        return this->get_slot_input(2);
    }

    virtual void set_slot_input(int8_t slot, const char *parameter_input_name);
    virtual void set_slot_input(int8_t slot, BaseParameterInput *parameter_input);
    virtual void set_slot_amount(int8_t slot, float amount) {
        if (!is_valid_slot(slot)) {
            Serial.printf("WARNING: in '%s', set_slot_amount with invalid slot number %i\n", this->label, slot);
            return;
        }
        this->connections[slot].amount = amount;
    }
    virtual void set_slot_polarity(int8_t slot, int polar_mode) {
        this->connections[slot].polar_mode = polar_mode;
    }

    virtual void set_slot_0_input(BaseParameterInput *parameter_input) {
        this->set_slot_input(0,parameter_input);
    }
    virtual void set_slot_0_amount(float amount) {
        this->set_slot_amount(0, amount);
    }
    virtual void set_slot_0_polarity(int polar_mode) {
        this->set_slot_polarity(0, polar_mode);
    }

    virtual void set_slot_1_input(BaseParameterInput *parameter_input) {
        this->set_slot_input(1,parameter_input);
    }
    virtual void set_slot_1_amount(float amount) {
        this->set_slot_amount(1, amount);
    }
    virtual void set_slot_1_polarity(int polar_mode) {
        this->set_slot_polarity(1, polar_mode);
    }
    
    virtual void set_slot_2_input(BaseParameterInput *parameter_input) {
        this->set_slot_input(2,parameter_input);
    }
    virtual void set_slot_2_amount(float amount) {
        this->set_slot_amount(2, amount);
    }
    virtual void set_slot_2_polarity(int polar_mode) {
        this->set_slot_polarity(2, polar_mode);
    }


    virtual void reset_mappings() {
        this->set_slot_0_amount(0.0f);
        this->set_slot_1_amount(0.0f);
        this->set_slot_2_amount(0.0f);
    }

    // account for rounding errors that are causing spurious modulation
    virtual bool is_modulation_slot_active(int slot);

    // calculate the modulation value based on the inputs * modulation amounts
    virtual float get_modulation_value();

    // whether to allow modulations to be set to this object, ie whether it can be modulated in realtime
    bool modulatable = true;

    virtual bool is_modulatable() {
        return this->modulatable;
    }
    virtual FloatParameter* set_modulatable(bool value) {
        this->modulatable = value;
        return this;
    }

    virtual float getRangeMinimumLimit()  {
        return this->minimumNormalValue;
    };
    virtual float getRangeMaximumLimit() {
        return this->maximumNormalValue;
    };
    virtual void setRangeMinimumLimit(float v) {
        //this->minimumNormalValue = v;
    };
    virtual void setRangeMaximumLimit(float v) {
        //this->maximumNormalValue = v;
    };
    virtual void incrementRangeMinimumLimit() {
        //this->minimumNormalValue += 0.1f;
    };
    virtual void decrementRangeMinimumLimit() {

    };
    virtual void incrementRangeMaximumLimit() {

    };
    virtual void decrementRangeMaximumLimit() {

    };

    virtual void save_pattern_add_lines(LinkedList<String> *lines); 
    virtual bool load_parse_key_value(String key, String value);
    
    //DataType lastGetterValue;
    virtual void update_mixer() {
        float modulation_value = this->get_modulation_value();
        float current_value = this->currentNormalValue;
        if (debug)
            if (strcmp(this->label, "None")!=0) {
                Serial.printf("update_mixer() in %s\tgot current_value=", this->label);
                Serial.println(current_value);
            }
        //if (modulateNormalValue!=lastModulatedNormalValue) {
            //this->sendCurrentTargetValue();
            lastModulatedNormalValue = constrain(current_value + modulation_value, this->minimumNormalValue, this->maximumNormalValue);
            //lastGetterValue = current_value;
        //}
    }

    #ifdef ENABLE_SCREEN
        FLASHMEM virtual MenuItem *makeControl();
        FLASHMEM virtual LinkedList<MenuItem *> *makeControls();
        FLASHMEM virtual MenuItem *makeInputSelectorControls(ParameterMenuItem *parent_control);
        //FLASHMEM 
        virtual LinkedList<MenuItem *> *addCustomTypeControls(LinkedList<MenuItem *> *controls) { return controls; };

        /*void link_parameter_input_controls_to_connections(MenuItem *amt1, MenuItem *amt2, MenuItem *amt3, SelectorControl<int> *source_selector_1, SelectorControl<int> *source_selector_2, SelectorControl<int> *source_selector_3) {
            this->connections[0].amount_control = amt1;
            this->connections[1].amount_control = amt2;
            this->connections[2].amount_control = amt3;
            this->connections[0].input_control = source_selector_1;
            this->connections[1].input_control = source_selector_2;
            this->connections[2].input_control = source_selector_3;
        }*/

        // update the slot's menu control to represent the newly set parameter input source
        //void update_slot_amount_control(int8_t slot, BaseParameterInput *parameter_input);
        //void update_slot_amount_control(byte slot, char name);
    #endif
};

template<class DataType = float>
class DataParameterBase : public FloatParameter {
    public:

        DataType minimumDataLimit = 0.0f;
        DataType maximumDataLimit = 100.0f;
        DataType lastDataValue = 0.0f;
        DataType currentDataValue = 0.f;
        DataType initialDataValue = 0.f;

        float modulateNormalValue = 0.0f;

        DataType minimumDataRange = this->minimumDataLimit, maximumDataRange = this->maximumDataLimit;

        DataParameterBase(const char *label) : FloatParameter(label) {}

        virtual DataType getter() = 0 ; //{}

        virtual DataParameterBase* initialise_values(DataType minimumDataValue, DataType maximumDataValue) { //float minimumDataValue = 0.0, float maximumDataValue = 100.0) {
            DataType current_value = minimumDataLimit;
            current_value = this->getter();

            return this->initialise_values(minimumDataValue, maximumDataValue, current_value);
        }
        virtual DataParameterBase* initialise_values(DataType minimumDataValue, DataType maximumDataValue, DataType current_value) {
            this->minimumDataLimit = minimumDataValue;
            this->maximumDataLimit = maximumDataValue;
            this->currentDataValue = current_value;
            this->initialDataValue = current_value;

            this->minimumNormalValue = 0.0f; //minimumDataValue;
            this->maximumNormalValue = 1.0f; //maximumDataValue;
            this->currentNormalValue = this->dataToNormal(current_value); //(current_value - minimumDataValue) / (maximumDataValue - minimumDataValue);
            this->initialNormalValue = this->currentNormalValue;

            this->minimumDataRange = minimumDataLimit;
            this->maximumDataRange = maximumDataLimit;

            return this;
        }

        virtual float getMinimumDataLimit() override {
            return this->minimumDataLimit;
        }
        virtual float getMaximumDataLimit() override {
            return this->maximumDataLimit;
        }

        virtual float getRangeMinimumLimit() override {
            return this->constrainDataToLimit(this->minimumDataRange);
        }
        virtual float getRangeMaximumLimit() override {
            return this->constrainDataToLimit(this->maximumDataRange);
        }
        virtual void setRangeMinimumLimit(float v) override {
            this->minimumDataRange = v;
            if (this->minimumDataRange < this->minimumDataLimit)
                this->minimumDataRange = this->minimumDataLimit;
            if (this->debug) Serial.printf("%s#setRangeMinimumLimit(%3.3f) => %3.3f\t(minimumDataLimit is %3.3f)\n", this->label, v, this->minimumDataRange, this->minimumDataLimit);
            //this->minimumNormalValue = this->dataToNormal(minimumDataRange);
        }
        virtual void setRangeMaximumLimit(float v) override {
            this->maximumDataRange = v;
            if (this->maximumDataRange > this->maximumDataLimit)
                this->maximumDataRange = this->maximumDataLimit;
            //this->maximumNormalValue = this->dataToNormal(maximumDataRange);
        }
        virtual void incrementRangeMinimumLimit() override {
            //Serial.printf("%s#incrementRangeMinimumLimit() with minimumDataRange=%s\n", this->label, this->getFormattedValue(this->minimumDataRange));
            this->setRangeMinimumLimit(this->incrementDataRange((DataType)this->getRangeMinimumLimit()));
            // call updateValueFromData to ensure the current value is within the new range
            // actually seems to happen anyway without needing to do this
            //this->updateValueFromData(this->currentDataValue);
            //this->setRangeMinimumLimit(this->getRangeMinimumLimit() + this->get_current_step(this->minimumDataRange));
        }
        virtual void decrementRangeMinimumLimit() override {
            //Serial.printf("%s#decrementRangeMinimumLimit() with minimumDataRange=%s\n", this->label, this->getFormattedValue(this->minimumDataRange));
            this->setRangeMinimumLimit(this->decrementDataRange((DataType)this->getRangeMinimumLimit()));
            // call updateValueFromData to ensure the current value is within the new range
            // actually seems to happen anyway without needing to do this
            //this->updateValueFromData(this->currentDataValue);
            //this->setRangeMinimumLimit(this->getRangeMinimumLimit() - this->get_current_step(this->minimumDataRange));
        }
        virtual void incrementRangeMaximumLimit() override {
            //Serial.printf("%s#incrementRangeMaximumLimit() with maximum_limit=%s\n", this->label, this->getFormattedValue(this->maximumDataRange));
            this->setRangeMaximumLimit(this->incrementDataRange((DataType)this->getRangeMaximumLimit()));
            // call updateValueFromData to ensure the current value is within the new range
            // actually seems to happen anyway without needing to do this
            //this->updateValueFromData(this->currentDataValue);
            //this->setRangeMaximumLimit(this->getRangeMaximumLimit() + this->get_current_step(this->maximum_limit));
        }
        virtual void decrementRangeMaximumLimit() override {
            //Serial.printf("%s#decrementRangeMaximumLimit() with maximum_limit=%s\n", this->label, this->getFormattedValue(this->maximumDataRange));
            this->setRangeMaximumLimit(this->decrementDataRange((DataType)this->getRangeMaximumLimit()));
            // call updateValueFromData to ensure the current value is within the new range
            // actually seems to happen anyway without needing to do this
            //this->updateValueFromData(this->currentDataValue);
            //this->setRangeMaximumLimit(this->getRangeMaximumLimit() - this->get_current_step(this->maximum_limit));
        }

        virtual DataType get_effective_minimum_data_value() {
            if (this->minimumDataRange > this->minimumDataLimit)
                return this->minimumDataRange;
            return this->minimumDataLimit;
        }
        virtual DataType get_effective_maximum_data_value() {
            if (this->maximumDataRange < this->maximumDataLimit)
                return this->maximumDataRange;// + (DataType)1;    // add 1 to ensure that the maximum value is included in the range
            return this->maximumDataLimit;// + (DataType)1;
        }

        virtual DataType normalToData(float value) {
            if (this->debug) {
                Serial.printf("%s#", this->label);
                Serial.printf("normalToData(%f) ", value);
                //Serial.printf(", range is %i to %i ", this->minimumDataLimit, this->maximumDataLimit);
                //Serial.printf(",\trange is %3.3f to %3.3f ", (float)this->minimumDataLimit, (float)this->maximumDataLimit);
                Serial.printf(",\trange is %3.3f to %3.3f ", (float)this->get_effective_minimum_data_value(), (float)this->get_effective_maximum_data_value());
            }
            value = this->constrainNormal(value);
            DataType data = this->get_effective_minimum_data_value() + (value * (float)(this->get_effective_maximum_data_value() - this->get_effective_minimum_data_value()));
            //if (this->debug/* && value>=0.0f*/) if (Serial) Serial.printf(" => %i\n", data);
            if (this->debug && Serial) Serial.printf(" => %3.3f\n", (float)data);
            return data;
        }
        virtual float dataToNormal(DataType value) {
            //if (this->debug) Serial.printf("dataToNormal(%i) ", value);
            value = constrainDataToRange(value);
            if (this->debug) Serial.printf("%s#dataToNormal(%3.3f) ", this->label, (float)value);
            float normal = (float)(value - get_effective_minimum_data_value()) / (float)(get_effective_maximum_data_value() - get_effective_minimum_data_value());
            if (this->debug) Serial.printf(" => %3.3f\n", normal);
            return normal;
            // eg   min = 0, max = 100, actual = 50 ->          ( 50 - 0 ) / (100-0)            = 0.5
            //      min = 0, max = 100, actual = 75 ->          ( 75 - 0 ) / (100-0)            = 0.75
            //      min = -100, max = 100, actual = 0 ->        (0 - -100) / (100--100)         = 0.5
            //      min = -100, max = 100, actual = -100 - >    (-100 - -100) / (100 - -100)    = 0
        }

        virtual DataType getCurrentDataValue() {
            return this->currentDataValue;
        }
        virtual DataType getLastDataValue() {
            return this->lastDataValue;
        }

        // setInitialValue in target value ie as a float to be multiplied by maximumDataValue
        virtual void setInitialValueFromNormal(float value) {
            this->currentNormalValue = value; // * this->maximumNormalValue;
            this->currentDataValue = this->normalToData(value); //->minimumDataLimit + (value * this->maximumNormalValue);
            this->initialNormalValue = value;
            this->initialDataValue = this->currentDataValue;
        }
        virtual void setInitialValueFromData(DataType value) {
            this->currentNormalValue = this->initialNormalValue = this->dataToNormal(value); 
            this->currentDataValue = this->initialDataValue = value;
        }

        // update internal param and call setter on target
        virtual void updateValueFromData(DataType value) {
            if (this->debug) { Serial.printf("DataParameterBase#updateValueFromData(%i)\n", value); Serial_flush(); }

            if (this->getCurrentDataValue()==value) {
                if (this->debug) {
                    Serial.printf("DataParameterBase#updateValueFromData(%i) - value is the same as current value, returning\n", value);
                    Serial_flush();
                }
                return;
            }

            this->lastDataValue = this->getCurrentDataValue();
            this->lastNormalValue = this->lastNormalValue;

            this->currentDataValue = value;
            if (this->debug) Serial.printf("DataParameterBase#updateValueFromData(%i) - currentDataValue=%i, about to call dataToNormal(%u)\n", value, this->currentDataValue, value);
            this->currentNormalValue = this->dataToNormal((DataType)value);
            if (this->debug) Serial.printf("called dataToNormal(%u) - currentNormalValue=%3.3f\n", value, this->currentNormalValue);

            if (this->debug) {
                Serial.printf("DataParameterBase#updateValueFromData(%i) - currentDataValue=%i, currentNormalValue=%3.3f\n", value, this->currentDataValue, this->currentNormalValue);
                Serial_flush();
            }

            if (this->debug) Serial.println("DataParameterBase#updateValueFromData() - about to call sendCurrentTargetValue() -> ");
            this->sendCurrentTargetValue();
            if (this->debug) Serial.println("<- DataParameterBase#updateValueFromData() - called sendCurrentTargetValue()");
        }

        DataType lastGetterValue;
        virtual void update_mixer() {
            this->modulateNormalValue = this->get_modulation_value();
            DataType current_value = this->getCurrentDataValue();
            if (debug) {
                Serial.printf("---->\nupdate_mixer() in %s\tgot current_value=", this->label);
                Serial.println(current_value);
            }
            //if (modulateNormalValue!=lastModulatedNormalValue) {
            
                this->sendCurrentTargetValue();
                lastModulatedNormalValue = modulateNormalValue;
                lastGetterValue = current_value;
            //}
            if (debug) {
                Serial.printf("update_mixer() in %s\tgot lastOutputNormalValue=", this->label);
                Serial.println(lastOutputNormalValue);
                Serial.println("<----");
            }
        }

        virtual void sendCurrentTargetValue() {
            float value = this->getCurrentNormalValue() + this->modulateNormalValue;
            //if (this->debug) Serial.printf(F("\tin %s, got modulated value to set: %f\n"), this->label, value);
            this->setTargetValueFromNormal(value);
        }

        // update internal param and send to target
        virtual void updateValueFromNormal(float value) override { //}, float range=1.0) override {
            //if (this->debug) Serial.printf(F("updateValueFromNormal(%f)\n"), value);
            this->updateValueFromData((DataType)this->normalToData(value));
        }

        virtual void modulateValue(float value) override {
            this->modulateNormalValue = value;
            //this->updateValueFromNormal(this->currentNormalValue);
        }

        uint32_t last_changed_at = 0;
        virtual DataType get_current_step(int ignored) {
            // do knob acceleration
            if (last_changed_at==0)
                return 1;

            uint32_t time_since_changed = constrain(millis() - this->last_changed_at, (uint32_t)0, (uint32_t)201);
            if      (time_since_changed>=200)   return (DataType)  1;
            else if (time_since_changed>=150)   return (DataType) (2);
            else if (time_since_changed>=100)   return (DataType) (4);
            else if (time_since_changed>=75)    return (DataType) (8);
            else                                return (DataType) (10);
        }
        virtual DataType get_current_step(unsigned int ignored) {
            return get_current_step((int)ignored);
        }

        virtual DataType get_current_step(float ignored) {
            // do knob acceleration
            if (last_changed_at==0)
                return 0.01f;

            uint32_t time_since_changed = constrain(millis() - this->last_changed_at, (uint32_t)0, (uint32_t)201);
            if      (time_since_changed>=200)   return (DataType)  0.01f;
            else if (time_since_changed>=100)   return (DataType) (0.02f);
            else if (time_since_changed>=50)    return (DataType) (0.05);
            else                                return (DataType) (0.10f);
        }

        // increment the value and update
        virtual void incrementValue() override {
            //this->debug = true;
            if (this->debug) {
                Serial.println("--");
                Serial.printf("DataParameterBase#incrementValue() for '%s', normal %f, data %i about to call updateValueFromData()....\n", this->label, this->getCurrentNormalValue(), this->getCurrentDataValue());
            }
            DataType current_value = this->getCurrentDataValue();
            if (this->debug) Serial.printf("DataParameterBase#incrementValue() for '%s', current value is %i\n", this->label, current_value);
            DataType incremented_value = this->incrementDataValue(current_value);
            if (this->debug) Serial.printf("DataParameterBase#incrementValue() for '%s', incremented value is %i\n", this->label, incremented_value);
            if (this->debug) Serial.printf("before updateValueFromData() called...\n");
            this->updateValueFromData(incremented_value);
            if (this->debug) Serial.printf("...after updateValueFromData() called.\n");
            this->last_changed_at = millis();
            if (this->debug) {
                Serial.printf("....DataParameterBase#incrementValue() for '%s', value became normal %f, data %i\n", this->label, this->getCurrentNormalValue(),this->getCurrentDataValue());
                Serial.println("--");
            }
        }
        // decrement the value and update
        virtual void decrementValue() override {
            //this->debug = true;
            if (this->debug) Serial.printf((const char*)F("--\nDataParameterBase#decrementValue() for '%s', initial FormattedValue '%s' (normal %f)"), this->label, this->getFormattedValue(this->getCurrentDataValue()), this->getCurrentNormalValue());
            this->updateValueFromData(this->decrementDataValue((DataType)this->getCurrentDataValue()));
            this->last_changed_at = millis();
            if (this->debug) Serial.printf((const char*)F("DataParameterBase#decrementValue() for '%s' became %s (normal %f)\n--\n"), this->label, this->getFormattedValue(this->getCurrentDataValue()), this->getCurrentNormalValue());
        }

        // returns an incremented DataType version of input value (unsigned int) - up to the current Range maximum
        virtual DataType incrementDataValue(unsigned int value) {
            if (this->debug) {
                Serial.printf("Parameter#incrementDataValue(%i)..\n", value); Serial.printf("\ttaking value %i and doing ++ on it..", value);
                Serial.printf("\tcurrent step is %i... ", this->get_current_step((int)value));
            }

            value += this->get_current_step((int)value);
            if (this->debug) Serial.printf("\tgot %i.\n", value);
            int new_value = this->constrainDataToRange(value);
            if (this->debug) Serial.printf("\tbecame %i (after constrain to %i:%i)..\n", new_value, this->minimumDataLimit, this->maximumDataLimit);
            return new_value;
        }
        // returns a decremented DataType version of input value (int) - down to the current Range minimum
        virtual DataType decrementDataValue(unsigned int value) {
            value -= this->get_current_step((int)value);
            // handle unsigned values that might wrap back around to max
            if ((DataType)value < this->get_effective_minimum_data_value() || (DataType)value >= this->get_effective_maximum_data_value())
                return this->get_effective_minimum_data_value();
            return this->constrainDataToRange(value);
        }
        // returns an incremented DataType version of input value (int) - up to the current Range maximum
        virtual DataType incrementDataValue(int value) {
            //if (this->debug) Serial.printf("Parameter#incrementDataValue(%i)..\n", value); Serial.printf("\ttaking value %i and doing ++ on it..", value);
            value += this->get_current_step(value);
            //if (this->debug) Serial.printf("\tgot %i.\n");
            int new_value = this->constrainDataToRange(value);
            //if (this->debug) Serial.printf("\tbecame %i (after constrain to %i:%i)..\n", new_value, this->minimumDataLimit, this->maximumDataLimit);
            return new_value;
        }
        // returns a decremented DataType version of input value (int) - down to the current Range minimum
        virtual DataType decrementDataValue(int value) {
            value -= this->get_current_step(value);
            // handle unsigned values that might wrap back around to max
            if ((DataType)value < this->get_effective_minimum_data_value() || (DataType)value >= this->get_effective_maximum_data_value())
                return this->get_effective_minimum_data_value();
            return this->constrainDataToRange(value);
        }
        // returns an incremented DataType version of input value (float) - up to the current Range maximum
        virtual DataType incrementDataValue(float value) {
            //Serial.printf("%s#incrementDataValue(%f) float version\n", this->label, value);
            value += this->get_current_step(value);
            return this->constrainDataToRange(value);
        }
        // returns a decremented DataType version of input value (float) - down to the current Range minimum
        virtual DataType decrementDataValue(float value) {
            value -= this->get_current_step(value);
            if ((DataType)value < this->get_effective_minimum_data_value() || (DataType)value >= this->get_effective_maximum_data_value())
                return this->get_effective_minimum_data_value();
            return this->constrainDataToRange(value);
        }

        // increment a <DataType> Range value up to the maximum of maximumDataLimit -- ie, up to the actual hard maximum of the underlying Parameter
        virtual DataType incrementDataRange(DataType value) {
            value += this->get_current_step(value);
            return this->constrainDataToLimit(value);
        }
        // decrement a <DataType> Range value up to the minimum of minimumDataLimit -- ie, up to the actual hard minimum of the underlying Parameter
        virtual DataType decrementDataRange(DataType value) {
            value -= this->get_current_step(value);
            if (value < this->minimumDataLimit || value >= this->maximumDataLimit)
                return this->minimumDataLimit;
            return this->constrainDataToLimit(value);
        }
        // constrain a <DataType> value to be within the minimum-maximum of the underlying Parameter
        virtual DataType constrainDataToLimit(DataType value) {
            if (debug) Serial.printf("%s#constrainDataToLimit(%3.3f): constraining to lie between %3.3f and %3.3f\n", this->label, (float)value, (float)this->minimumDataLimit, (float)this->maximumDataLimit);
            return constrain(value, this->minimumDataLimit, this->maximumDataLimit);
        }

        // constrain a <DataType> value to be within the minimum-maximum of the current custom range
        virtual DataType constrainDataToRange(DataType value) {
            if (debug && Serial) Serial.printf("%s#constrainDataToRange(%3.3f): constraining to lie between %3.3f and %3.3f\n", this->label, (float)value, (float)this->minimumDataLimit, (float)this->maximumDataLimit);
            return constrain(value, this->get_effective_minimum_data_value(), this->get_effective_maximum_data_value());
        }

        virtual const char *getFormattedLimit(float data_value) {
            //float normal = this->dataToNormal(data_value);
            return parseFormattedDataType((DataType)data_value);
        }

        #if !defined(USE_ARX_TYPE_TRAITS)
            // use Teensy version of this code that uses overriding functions instead of ArxTypeTraits/constexpr to render values
            virtual const char* getFormattedValue(float normal) override {
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
                return value ? label_on : label_off;
            }
            virtual const char* parseFormattedDataType(float value) {
                static char fmt[10] = "         ";
                //sprintf(fmt, "%3i%% (float)",     (int)(100.0*value)); //->getCurrentValue());
                snprintf(fmt, 10, "%3i%c", (int)(100.0f*value), this->float_unit);
                //Serial.printf("parseFormattedDataType(float)\treturning '%s'\n", fmt);
                return fmt;
            }
            virtual const char* parseFormattedDataType(unsigned int value) {
                static char fmt[10] = "         ";
                //sprintf(fmt, "%5u (unsigned)",   value); // (unsigned int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
                snprintf(fmt, 10, "%u",   value); // (unsigned int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
                // was "%5u"
                //Serial.printf("parseFormattedDataType(uint)\treturning '%s'\n", fmt);
                return fmt;
            }
            virtual const char* parseFormattedDataType(int value) {
                static char fmt[10] = "         ";
                //sprintf(fmt, "%5i (signed)",     value); // (int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
                snprintf(fmt, 10, "%i",   value); // (int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
                // was "%5i"
                //Serial.printf("parseFormattedDataType(int) returning '%s'\n", fmt);
                return fmt;
            }
            virtual const char* parseFormattedDataType(byte value) {
                static char fmt[10] = "         ";
                //sprintf(fmt, "%5i (signed)",     value); // (int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
                snprintf(fmt, 10, "%3i",  value); // (int)(this->maximumNormalValue*this->getCurrentNormalValue())); //getCurrentValue());
                //Serial.printf("parseFormattedDataType(byte) returning '%s'\n", fmt);
                return fmt;
            }
        #else
            // use version for 1284p that utilise ArxTypeTraits and constexpr to render values
            virtual const char* getFormattedValue() override {
                static char fmt[MAX_PARAMETER_NAME_LENGTH] = "              ";
                if constexpr (std::is_integral<DataType>::value && std::is_same<DataType, bool>::value) {
                    snprintf(fmt, MAX_PARAMETER_NAME_LENGTH, "%s", this->getCurrentValue()?"On" : "Off");
                } else if constexpr (std::is_floating_point<DataType>::value) {
                    snprintf(fmt, MAX_PARAMETER_NAME_LENGTH, "%3i%c (float)",     (int)(100.0f*this->getCurrentDataValue()), this->float_unit);
                } else if constexpr (std::is_unsigned<DataType>::value) {
                    snprintf(fmt, MAX_PARAMETER_NAME_LENGTH, "%5u (unsigned)",    (unsigned int)(this->getCurrentDataValue()));
                                            //(unsigned int)(this->maximumDataLimit*this->getCurrentValue())); //getCurrentValue());
                } else {
                    snprintf(fmt, MAX_PARAMETER_NAME_LENGTH, "%5i (signed)",      (int)(this->getCurrentDataValue())); 
                                            //(int)(this->maximumDataLimit*this->getCurrentValue())); //getCurrentValue());
                }
                //Serial.printf("getFormattedValue: '%s'\n", fmt);
                return fmt;
            };
        #endif

        DataType last_sent_value = 0;
        // set the target from normalised post-modulation value
        virtual void setTargetValueFromNormal(float value, bool force = false) {
            //if (strcmp(this->label, "CVPO1-A")==0) this->debug = true; else this->debug = false;

            float constrained_value = this->constrainNormal(value);
            this->lastModulatedNormalValue = constrained_value;
            //this->lastOutputNormalValue = this->lastModulatedNormalValue; // = value;
            if (debug && Serial) { 
                Serial.printf("%s#setTargetValueFromNormal(%3.3f) got modulated value %3.3f, ", this->label, value, constrained_value);
                Serial.flush();
            }

            float slewed_value = constrained_value;
            if (slewing || lastModulatedNormalValue!=lastOutputNormalValue) {
                slewed_value = this->get_slewed_value(constrained_value);
                if (this->debug && Serial) {
                    Serial.printf("%s:\tslewed from\t%3.3f\tto %3.3f\n", this->label, constrained_value, slewed_value);
                    Serial.flush();
                }
            }

            //float slewed_value = this->get_slewed_value(constrained_value);          

            DataType value_to_send = this->normalToData(slewed_value);
            if (debug && Serial) Serial.printf("\tgot value_to_send=%3.3f\n", (float)value_to_send);

            if (force || value_to_send!=last_sent_value) {
                this->lastRealOutputNormalValue = slewed_value;
                last_sent_value = value_to_send;
                this->setTargetValueFromData(value_to_send, force);
            }
            //this->setTargetValueFromData(value_to_send, force);
        }

        virtual void setTargetValueFromData(DataType value, bool force = false) = 0;


        virtual float constrainNormal(float value) {
            // TODO: check if polar/bipolar?
            /*if (this->debug) Serial.printf("in %s,\tconstraining %f to %f:%f => %f \n", 
                this->label, 
                value, 
                this->minimumNormalValue, 
                this->maximumNormalValue, 
                constrain(value, this->minimumNormalValue, this->maximumNormalValue)
            );*/
            // hmm, if we want to do bounds, should we do it here?
            return constrain(value, this->minimumNormalValue, this->maximumNormalValue);
        }

        const String prefix__parameter = String("parameter_");
        const String prefix__range_minimum = String("_range_minimum");
        const String prefix__range_maximum = String("_range_maximum");

        virtual void save_pattern_add_lines(LinkedList<String> *lines) override {
            FloatParameter::save_pattern_add_lines(lines);

            const String label_string = String(this->label);

            lines->add(prefix__parameter + label_string + prefix__range_minimum + String('=') + String(this->getRangeMinimumLimit()));
            lines->add(prefix__parameter + label_string + prefix__range_maximum + String('=') + String(this->getRangeMaximumLimit()));
        }

        virtual bool load_parse_key_value(const String incoming_key, String value) override {
            const String label_string = String(this->label);

            if (incoming_key.startsWith(prefix__parameter + label_string + prefix__range_minimum)) {
                this->setRangeMinimumLimit(value.toFloat());
                return true;
            } else if (incoming_key.startsWith(prefix__parameter + label_string + prefix__range_maximum)) {
                //Serial.printf("load_parse_key_value(%s, %s) found new value to set in %s: %3.3f\n", incoming_key.c_str(), value.c_str(), this->label, value.toFloat());
                this->setRangeMaximumLimit(value.toFloat());
                return true;
            } else {
                return FloatParameter::load_parse_key_value(incoming_key, value);
            }
        }

};


template<class TargetClass, class DataType = float>
class DataParameter : public DataParameterBase<DataType> {
    public:

        TargetClass *target = nullptr;
        void(TargetClass::*setter_func)(DataType value) = nullptr;// setter_func;
        DataType(TargetClass::*getter_func)() = nullptr;    // hmm this may not actually be used now..? we keep internal track of the value here, and reference the variable directly via pointer in ProxyParameter...

        //ParameterMixer *mixer = nullptr;

        DataParameter(const char *label, TargetClass *target) 
            : DataParameterBase<DataType>(label) {
            this->target = target;
            //this->mixer = new ParameterMixer(); //this);
        }
        DataParameter(const char *label, TargetClass *target, DataType initial_value_normal) 
            : DataParameter<TargetClass,DataType>(label, target) {
            this->initialNormalValue = initial_value_normal;
        }
        DataParameter(const char *label, TargetClass *target, void(TargetClass::*setter_func)(DataType)) 
            : DataParameter<TargetClass,DataType>(label, target) {
            this->setter_func = setter_func;
        }
        DataParameter(const char *label, TargetClass *target, void(TargetClass::*setter_func)(DataType), DataType(TargetClass::*getter_func)()) 
            : DataParameter<TargetClass,DataType>(label, target, setter_func)
        {
            this->getter_func = getter_func;
            //if (getter_func!=nullptr)
            //    this->setInitialValue();
        }
        DataParameter(const char *label, TargetClass *target, void(TargetClass::*setter_func)(DataType), DataType(TargetClass::*getter_func)(), DataType minimumDataValue, DataType maximumDataValue) 
            : DataParameter<TargetClass,DataType>(label, target, setter_func, getter_func) /* minimumDataLimit(minimumDataValue), maximumDataLimit(maximumDataValue), */
        {
            this->minimumDataLimit = minimumDataValue;
            this->maximumDataLimit = maximumDataValue;
        }
        DataParameter(const char *label, TargetClass *target, float initial_value_normal, void(TargetClass::*setter_func)(DataType)) 
            : DataParameter<TargetClass,DataType>(label, target, setter_func) {
            this->initialNormalValue = initial_value_normal;
            //this->setInitialValueFromNormal(initial_value_normal);
        }

        virtual DataType getCurrentDataValue() override {
            return this->getter();
        }
        virtual float getCurrentNormalValue() override {
            return this->dataToNormal(this->getCurrentDataValue());
        }

        virtual DataType getter() override {
            /*
            DataType current_value = this->minimumDataLimit;
            if (this->getter_func!=nullptr) 
                current_value = (this->target->*getter_func)();
            return current_value;
            */
           return this->currentDataValue;
        }

        virtual void setInitialValue() {
            if (this->getter_func!=nullptr)
                this->setInitialValueFromData((this->target->*getter_func)());
        }

        // actually set the target from data, do the REAL setter call on target
        DataType last_sent_value = -1;
        virtual void setTargetValueFromData(DataType value, bool force = false) {
            // early return if this value is the same as last one and we aren't being asked to force it
            if (!force && last_sent_value==value)
                return;

            if (this->target!=nullptr && this->setter_func!=nullptr) {
                #ifdef ENABLE_PRINTF
                    /*if (this->debug) {
                        Serial.println(F("Parameter#setTargetValueFromData()")); Serial_flush();
                        Serial.printf(F("%s: Calling setter func for value ("), this->label);
                        Serial.print(value);
                        Serial.println(')');
                    }*/
                #endif
                if (target==nullptr) {
                    Serial.printf("WARNING: setTargetValueFromData() avoided calling setter_func on nullptr in %s!\n", this->label); Serial_flush();
                    return;
                }
                if (setter_func==nullptr) {
                    Serial.printf("WARNING: setTargetValueFromData() avoided calling null setter_func in %s!\n", this->label); Serial_flush();
                    return;
                }
                (this->target->*setter_func)((DataType)value);
            } else {
                /*#ifdef ENABLE_PRINTF
                    Serial.printf(F("WARNING: no target / no setter_func in %s!\n"), this->label);
                #endif*/
            }
        }

        virtual void set_target_object(TargetClass *target) {
            this->target = target;
        }
        virtual void set_target_func(void(TargetClass::*fp)(DataType)) {
            this->setter_func = fp;
        }

};


#include "functional-vlpp.h"
template<class DataType = float>
class LDataParameter : public DataParameterBase<DataType> {
    public:

        using setter_func_def = vl::Func<void(DataType)>;
        using getter_func_def = vl::Func<DataType(void)>;

        setter_func_def setter_func = [=](DataType v) -> void {};
        getter_func_def getter_func = [=]() -> DataType { return this->minimumDataRange; };

        LDataParameter(const char *label) 
            : DataParameterBase<DataType>(label) {
        }
        LDataParameter(const char *label, DataType initial_value_normal) 
            : LDataParameter<DataType>(label) {
            this->initialNormalValue = initial_value_normal;
        }
        LDataParameter(const char *label, setter_func_def setter_func) 
            : LDataParameter<DataType>(label) {
            this->setter_func = setter_func;
        }
        LDataParameter(const char *label, setter_func_def setter_func, getter_func_def getter_func) 
            : LDataParameter<DataType>(label, setter_func)
        {
            this->getter_func = getter_func;
        }
        LDataParameter(const char *label, setter_func_def setter_func, getter_func_def getter_func, DataType minimumDataValue, DataType maximumDataValue) 
            : LDataParameter<DataType>(label, setter_func, getter_func)
        {
            this->minimumDataLimit = this->minimumDataRange = minimumDataValue;
            this->maximumDataLimit = this->maximumDataRange = maximumDataValue;
        }
        LDataParameter(const char *label, float initial_value_normal, setter_func_def setter_func) 
            : LDataParameter<DataType>(label, setter_func) {
            this->initialNormalValue = initial_value_normal;
            //this->setInitialValueFromNormal(initial_value_normal);
        }

        virtual DataType getCurrentDataValue() override {
            return this->getter();
        }
        virtual float getCurrentNormalValue() override {
            return this->dataToNormal(this->getCurrentDataValue());
        }

        virtual DataType getter() override {
           return this->currentDataValue;
        }

        virtual void setInitialValue() {
            this->setInitialValueFromData(this->getter_func());
        }

        // actually set the target from data, do the REAL setter call on target
        DataType last_sent_value = -1;
        virtual void setTargetValueFromData(DataType value, bool force = false) {
            // early return if this value is the same as last one and we aren't being asked to force it
            if (!force && last_sent_value==value)
                return;

            this->setter_func((DataType)value);
        }
};
