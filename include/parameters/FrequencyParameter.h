#include "ads.h"
#include "Parameter.h"

#include "midi_helpers.h"

template<class TargetClass, class DataType = float>
class FrequencyParameter : public Parameter<TargetClass, DataType> {
    public:

        DataType octave_range = 5.0;

        FrequencyParameter(char *label, TargetClass *target, void(TargetClass::*setter_func)(DataType), DataType octave_range = 5.0) 
            : Parameter<TargetClass,DataType>(label, target, setter_func) 
            {
            this->octave_range = octave_range;
        };
        FrequencyParameter(char *label, TargetClass *target, float initial_value, void(TargetClass::*setter_func)(DataType), DataType octave_range = 5.0) 
            : Parameter<TargetClass,DataType>(label, target, initial_value, setter_func) {
                this->octave_range = octave_range;
            };

        FrequencyParameter(char *label, TargetClass *target, void(TargetClass::*setter_func)(DataType), void(TargetClass::*getter_func)(DataType), DataType octave_range = 5.0) 
            : Parameter<TargetClass,DataType>(label, target, setter_func, getter_func) 
            {
            this->octave_range = octave_range;
        };

        //TODO: move helper functions from ads.cpp ie get_frequencey_for_voltage etc to here
        //TODO: check quantisation / pitch is accurate

        virtual void setParamValue(DataType value, DataType octave_range) override {
            //if (inverted)
                //value = octave_range - (octave_range*value);

            this->octave_range = octave_range;

            float freq = get_frequency_for_voltage(value * this->octave_range); //read_voltage(0));

            this->lastValue = this->currentValue;
            this->currentValue = value;

            if (this->debug) {
                Serial.printf("FrequencyParameter#setParamValue(): calling setter func with frequency (%i) from input value ", (uint32_t) freq);
                //Serial.println((value * this->octave_range));
                Serial.print(value);
                Serial.print(" * octave_range ");
                Serial.println(this->octave_range);
            }
            //Serial.println();

            //((this->target)->(*this->setter_func))(freq);
            (this->target->*this->setter_func)(freq);
        }

        virtual const char* getFormattedValue() {
            static char fmt[20] = "              ";

            float voltage = this->currentValue * this->octave_range;

            float freq = get_frequency_for_voltage(voltage); //read_voltage(0));
            int pitch   = get_midi_pitch_for_voltage(voltage);

            //Serial.printf("%s: getFormattedValue got pitch %i: %s\n", this->label, pitch, get_note_namec(pitch));
            uint16_t ifreq = (uint16_t) freq;
            snprintf(fmt, 20, "%4uhz [%3s]", ifreq, get_note_namec(pitch)); //->getCurrentValue());

            //Serial.printf("getFormattedValue: '%s'\n", fmt);
            return fmt;
        };
};