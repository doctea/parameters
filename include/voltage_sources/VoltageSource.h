#ifndef VOLTAGE_SOURCE__INCLUDED
#define VOLTAGE_SOURCE__INCLUDED

// base class for a voltage source, eg a wrapper around an ADC library
class VoltageSourceBase {
    public:
        bool debug = false;

        double current_value = 0.0;
        double last_value = 0.0;

        double maximum_input_voltage;

        // actually fetch the current value from ADC, put it in the current_value
        virtual double fetch_current_voltage();
        // update the current voltage values

        virtual void update() {
            //last_value = ads_source->readADC(channel);
            this->last_value = this->current_value;
            this->current_value = this->fetch_current_voltage();
        }

        // returns the last read raw voltage value
        virtual double get_voltage() {
            return this->current_value;
        }

        // return a normalised version of the last value (ie 0.0-1.0)
        virtual double get_voltage_normal() {
            return this->get_voltage() / this->maximum_input_voltage;
        }
};
#endif