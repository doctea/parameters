#pragma once

#include "VoltageParameterInput.h"

#include "functional-vlpp.h"

class ThresholdToggleParameterInput : public VoltageParameterInput {
    float threshold = 0.0f;
    vl::Func<void(bool)> callback;

    bool currentState = false;
    bool firstRun = true;

    public:
        ThresholdToggleParameterInput(
            char *name, 
            const char *group_name, 
            VoltageSourceBase *voltage_source, 
            float in_sensitivity = 0.005, 
            VALUE_TYPE input_type = BIPOLAR, 
            bool inverted = false, 
            float threshold = 0.5f,
            vl::Func<void(bool)> callback = [](bool state) -> void {}
        ) : VoltageParameterInput(name, group_name, voltage_source, in_sensitivity, input_type, inverted) {
            this->threshold = threshold;
            this->callback = callback;
        }

        virtual void read() override {
            //Serial.printf("ThresholdToggleParameterInput::read() %s\n", this->name);
            float readValue = this->voltage_source->get_voltage_normal();
            if (inverted) {
                readValue = 1.0f - readValue;
            }

            if (this->debug) Serial.printf("ThresholdToggleParameterInput::read() %s readValue %f\n", this->name, readValue);

            if (firstRun || this->is_significant_change(readValue, this->lastValue)) {
                this->lastValue = readValue;

                if (threshold < 0.0f && readValue >= -1.0f*this->threshold) {
                    if (firstRun || !this->currentState) {
                        this->callback(false);  // todo: not sure why these callback states are inverted, but that's a problem for another time
                    }
                    this->currentState = true;
                    this->currentValue = 1.0f;
                } else if (threshold > 0.0f && readValue < this->threshold) {
                    if (firstRun || !this->currentState) {
                        this->callback(false);
                    }
                    this->currentState = true;
                    this->currentValue = 1.0f;
                } else {
                    if (firstRun || this->currentState) {
                        this->callback(true);
                    }
                    this->currentState = false;
                    this->currentValue = 0.0f;
                }
                firstRun = false;
            }
        }
};