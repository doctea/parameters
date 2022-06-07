#include "ads.h"
#include "Parameter.h"

template<class TargetClass, class DataType = double>
class FrequencyParameter : public Parameter<TargetClass, DataType> {
    public:

        DataType octave_range = 5.0;

        FrequencyParameter(char *label, TargetClass *target, void(TargetClass::*setter_func)(DataType), DataType octave_range = 5.0) 
            : Parameter<TargetClass,DataType>(label, target, setter_func) 
            {
            this->octave_range = octave_range;
        };

        //TODO: move helper functions from ads.cpp ie get_frequencey_for_voltage etc to here
        //TODO: check quantisation / pitch is accurate

        virtual void setParamValue(DataType value) override {
            //if (inverted)
                //value = octave_range - (octave_range*value);

            double freq = get_frequency_for_voltage(value * octave_range); //read_voltage(0));

            this->lastValue = this->currentValue;
            this->currentValue = value;

            if (this->debug) {
                Serial.printf("FrequencyParameter#setParamValue(): calling setter func with frequency (%i) from input value ", freq);
                Serial.println(value * 5.0);
            }
            //Serial.println();

            //((this->target)->(*this->setter_func))(freq);
            (this->target->*this->setter_func)(freq);
        }

        virtual const char* getFormattedValue() {
            static char fmt[20] = "              ";

            double freq = get_frequency_for_voltage(this->currentValue * octave_range); //read_voltage(0));

            //if constexpr (std::is_floating_point<DataType>::value) {
                sprintf(fmt, "%5i hz",      (uint32_t)freq); //->getCurrentValue());
            /*} else if constexpr (std::is_unsigned<DataType>::value) {
                sprintf(fmt, "%5u (unsigned)",    (int32_t)this->getCurrentValue()); //getCurrentValue());
            } else {
                sprintf(fmt, "%5i (signed)",      (uint32_t)this->getCurrentValue()); //getCurrentValue());
            }*/
            //Serial.printf("getFormattedValue: '%s'\n", fmt);
            return fmt;
        };
};