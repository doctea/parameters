#include "ads.h"
#include "Parameter.h"

template<class TargetClass, class DataType = double>
class FrequencyParameter : public Parameter<TargetClass, DataType> {
    public:

        DataType octave_range = 5.0;

        FrequencyParameter(TargetClass *target, void(TargetClass::*setter_func)(DataType), DataType octave_range = 5.0) : Parameter<TargetClass,DataType>(target, setter_func) {
            this->octave_range = octave_range;
        };

        //TODO: move helper functions from ads.cpp ie get_frequencey_for_voltage etc to here
        //TODO: check quantisation / pitch is accurate

        virtual void setParamValue(DataType value) override {
            double freq = get_frequency_for_voltage(value * octave_range); //read_voltage(0));

            this->last_value = this->current_value;
            this->current_value = freq;

            Serial.printf("FrequencyParameter setParamValue(): calling setter func with frequency (%i) from input value ", freq);
            Serial.println(value * 5.0);
            //Serial.println();

            //((this->target)->(*this->setter_func))(freq);
            (this->target->*this->setter_func)(freq);
        }
};