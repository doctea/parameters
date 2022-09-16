#ifndef VOLTAGEPARAMETERINPUT__INCLUDED
#define VOLTAGEPARAMETERINPUT__INCLUDED

#include "ParameterInput.h"
#include "AnalogParameterInputBase.h"

#include "../voltage_sources/VoltageSource.h"

template<class TargetClass, class DataType = double>
class VoltageParameterInput : public AnalogParameterInputBase<TargetClass, DataType> {
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
                this->lastValue = this->currentValue;
                //Serial.printf("%c: Setting this->currentValue to ", this->name);
                //Serial.println(currentValue);
                this->currentValue = currentValue;
                #ifdef ENABLE_PRINTF
                if (this->debug) {
                    /*Serial.printf("%s: VoltageParameterInput->read() got intermediate %i, voltage ", this->name, intermediate);
                    Serial.print((uint32_t) this->ads->toVoltage(intermediate));
                    Serial.printf(", final %i", (uint32_t) currentValue*1000.0);
                    Serial.println();*/
                    Serial.printf("%c: VoltageParameterInput->read() got voltage ", this->name); //
                    Serial.println(currentValue);
                }
                #endif

                //DataType normal = this->get_normal_value(currentValue);
                DataType normal = currentValue;
                #ifdef ENABLE_PRINTF
                if (this->debug) {
                    Serial.printf("VoltageParameterInput#read() for '%c': got currentValue ", this->name); Serial.flush();
                    Serial.print(currentValue); Serial.flush();
                    Serial.print(" converted to normal "); Serial.flush();
                    Serial.println(normal); Serial.flush();
                }
                #endif

                if (this->callback != nullptr) {
                    if (this->debug) {
                        Serial.print(this->name);
                        Serial.print(F(": calling callback("));
                        Serial.print(normal);
                        Serial.println(F(")"));
                        Serial.flush();
                    }      
                    (*this->callback)(normal);
                }
                if (this->target_parameter!=nullptr) {
                    if (this->debug) {
                        Serial.println("Calling on target_parameter.."); Serial.flush();
                        Serial.print(this->name); Serial.flush();
                        Serial.print(F(": calling target from normal setParamValue(")); Serial.flush();
                        Serial.print(normal); Serial.flush();
                        Serial.print(F(")")); Serial.flush();
                        Serial.print(" from currentValue "); Serial.flush();
                        Serial.print(currentValue); Serial.flush();
                        if (this->inverted) { Serial.print(F(" - inverted")); Serial.flush(); }
                        Serial.println(); Serial.flush();

                        #ifdef ENABLE_PRINTF
                            Serial.printf("VoltageParameterInput %c calling setParamValue with maximum_input_voltage ", this->name);
                            Serial.println(this->voltage_source->maximum_input_voltage);
                        #endif
                    }
                    //this->target_parameter->setParamValue(normal, this->voltage_source->maximum_input_voltage);
                    this->target_parameter->updateValueFromNormal(normal); //
                }
                //Serial.println("Finishing read()"); Serial.flush();
            }
        }
};

#endif