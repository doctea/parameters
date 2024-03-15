#pragma once

#include "Parameter.h"

template<class DataType>
class ProxyParameter : public DataParameterBase<DataType> {
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

    virtual void setTargetValueFromData(DataType value, bool force = false) override {
        *this->target = value;
    }

    virtual float getCurrentNormalValue() override {
        return this->dataToNormal(this->getCurrentDataValue());
    }
    virtual DataType getCurrentDataValue() override {
        return *this->source;
    }

    virtual void updateValueFromNormal(float v) override {
        *this->target = *this->source = this->normalToData(v);
    }
    virtual void updateValueFromData(DataType v) override {
        /*if (Serial) { 
            Serial.printf("ProxyParameter#updateValueFromData(");
            Serial.print(v);
            Serial.println(")");
        }*/
        *this->target = *this->source = v;        
    }
};