#ifndef VOLTAGE_SOURCE__INCLUDED
#define VOLTAGE_SOURCE__INCLUDED

class VoltageSourceBase {
    public:
        double current_value = 0.0;
        double last_value = 0.0;

        double maximum_input_voltage;

        virtual double fetch_current_voltage();

        virtual void update() {
            //last_value = ads_source->readADC(channel);
            this->last_value = this->current_value;
            this->current_value = this->fetch_current_voltage();
        }

        virtual double get_voltage_normal() {
            return this->get_voltage() / this->maximum_input_voltage;
        }

        virtual double get_voltage() {
            return this->current_value;
        }
};
#endif