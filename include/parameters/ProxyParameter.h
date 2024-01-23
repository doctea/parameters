#pragma once

#include "Parameter.h"

template<class DataType>
class ProxyParameter : public DataParameterBase<DataType> {
    public:

    DataType *source, *target;

    ProxyParameter(const char *label, DataType *source, DataType *target, DataType minimum_value, DataType maximum_value) 
        : DataParameterBase<DataType>(label)
        {
            this->source = source;
            this->target = target;
            this->initialise_values(minimum_value, maximum_value);
        }

    virtual DataType getter() override {
        return *source;
    }

    virtual void setTargetValueFromData(DataType value, bool force = false) override {
        *this->target = value;
    }

};