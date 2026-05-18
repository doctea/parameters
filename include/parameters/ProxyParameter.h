#pragma once

#include "Parameter.h"

template<class DataType>
class ProxyParameter : public DataParameterBase<DataType> 
    #ifdef ENABLE_STORAGE
        , virtual public SHDynamic<0, 4> // no children; ~2-4 own settings (range, modslots if enabled)
    #endif
{
    public:

    DataType *source, *target;

    ProxyParameter(const char *label, DataType *source, DataType *target, DataType minimumDataValue, DataType maximumDataValue) 
        : DataParameterBase<DataType>(label)
        {
            this->source = source;
            this->target = target;
            this->initialise_values(minimumDataValue, maximumDataValue);
        }

    virtual DataType getter() override {
        return *source;
    }

    /*virtual DataType getCurrentDataValue() override {
        return this->getter();
    }*/

    virtual void setTargetValueFromData(DataType value, bool force = false) override {
        *this->target = value;
        this->lastOutputNormalValue = this->dataToNormal(value);
    }

    virtual float getCurrentNormalValue() override {
        return this->dataToNormal(this->getCurrentDataValue());
    }
    virtual DataType getCurrentDataValue() override {
        return *this->source;
    }

    virtual void updateValueFromNormal(float v) override {
        this->last_sent_value = *this->target = *this->source = this->normalToData(v);
        this->lastOutputNormalValue = v;
    }
    virtual void updateValueFromData(DataType v) override {
       this->last_sent_value = *this->target = *this->source = v;
       this->lastOutputNormalValue = this->dataToNormal(v);
       //this->lastOutputValue = v;
       //*this->target = *this->source = v;        
    }

    #ifdef ENABLE_STORAGE
        virtual void setup_saveable_settings() override {
            DataParameterBase<DataType>::setup_saveable_settings();

            // we need to prevent storage of the current value, since it's just a proxy for something else that
            // might also be storing it, so it would be confusing and potentially cause issues when recalling
            this->remove_setting_by_label("current_value");
        }
    #endif       
};

#ifdef ENABLE_SCALES

#include "midi_helpers.h"

template<class DataType=int8_t>
class ProxyNoteParameter : public ProxyParameter<DataType> {
    public:
    ProxyNoteParameter(const char *label, int8_t *source, int8_t *target, int8_t minimumDataValue = MIDI_MIN_NOTE, int8_t maximumDataValue = MIDI_MAX_NOTE) 
        : ProxyParameter<DataType>(label, source, target, minimumDataValue, maximumDataValue) {}

    virtual const char* parseFormattedDataType(int value) override {
        return get_note_name_c(value);
    }
};

#endif