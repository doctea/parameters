#include "ParameterInput.h"
#include "AnalogParameterInput.h"

template<class TargetClass, class DataType = double>
class VoltageParameterInput : public AnalogParameterInput<TargetClass, DataType> {
    VoltageSourceBase *voltage_source;

    public:
        VoltageParameterInput(char name, VoltageSourceBase *voltage_source) {
            this->name = name;
            this->voltage_source = voltage_source;
        }

        VoltageParameterInput(char name, VoltageSourceBase *voltage_source, TargetClass *target_parameter) : VoltageParameterInput(name, voltage_source) {
            this->target_parameter = target_parameter;
        }

        virtual void read() override {
            DataType currentValue = this->voltage_source->get_voltage_normal();
            if (this->is_significant_change(currentValue, this->lastValue)) {
                this->lastValue = currentValue;
                if (this->debug) {
                    /*Serial.printf("%s: VoltageParameterInput->read() got intermediate %i, voltage ", this->name, intermediate);
                    Serial.print((uint32_t) this->ads->toVoltage(intermediate));
                    Serial.printf(", final %i", (uint32_t) currentValue*1000.0);
                    Serial.println();*/
                    Serial.printf("%c: VoltageParameterInput->read() got voltage ", this->name); //
                    Serial.println(currentValue);
                }

                //DataType normal = this->get_normal_value(currentValue);
                DataType normal = currentValue;
                Serial.printf("VoltageParameterInput#read() for '%c': got currentValue ", this->name);
                Serial.print(currentValue);
                Serial.print(" converted to normal ");
                Serial.println(normal);

                if (this->callback != nullptr) {
                    if (this->debug) {
                        Serial.print(this->name);
                        Serial.print(F(": calling callback("));
                        Serial.print(normal);
                        Serial.println(F(")"));
                    }      
                    callback(normal);
                }
                if (this->target_parameter!=nullptr) {
                    if (this->debug) {
                        Serial.print(this->name);
                        Serial.print(F(": calling target from normal setParamValue("));
                        Serial.print(normal);
                        Serial.print(F(")"));
                        Serial.print(" from currentValue ");
                        Serial.print(currentValue);
                        if (this->inverted) Serial.print(F(" - inverted"));
                        Serial.println();
                    }
                    //target->setParamValueA(normal);
                    this->target_parameter->setParamValue(normal);
                }
            }
        }
};