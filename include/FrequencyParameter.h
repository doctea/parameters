#include "ads.h"
#include "Parameter.h"

template<class TargetClass, class DataType = double>
class FrequencyParameter : public Parameter<TargetClass, DataType> {
    public:

        FrequencyParameter(TargetClass *target, void(TargetClass::*setter_func)(DataType)) : Parameter<TargetClass,DataType>(target, setter_func) {};

        virtual void setParamValue(DataType value) override {
            this->last_value = value;
            this->current_value = value;
            //this->func(value);
            int freq = get_frequency_for_voltage(value); //read_voltage(0));
            Serial.printf("FrequencyParameter setParamValue(): calling setter func with value (%i)\n", freq);

            //((this->target)->(*this->setter_func))(freq);
            (this->target->*this->setter_func)(freq);
        }
};