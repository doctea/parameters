#ifndef VOLTAGEPARAMETERINPUT__INCLUDED
#define VOLTAGEPARAMETERINPUT__INCLUDED

#include "ParameterInput.h"
#include "AnalogParameterInputBase.h"

#include "../voltage_sources/VoltageSource.h"

#include "ads.h"

class VoltageParameterInput : public AnalogParameterInputBase<double> {
    VoltageSourceBase *voltage_source = nullptr;

    public:
        VoltageParameterInput(char *name, VoltageSourceBase *voltage_source) : AnalogParameterInputBase(name) {
            this->voltage_source = voltage_source;
        }

        virtual bool hasExtra() override {
            return this->supports_pitch();
        }
        virtual const char *getExtra() override {
            if (this->voltage_source==nullptr) {
                return "[null voltage_source]";
            }
            if (this->supports_pitch()) {
                static char extra_output[40];
                sprintf(
                    extra_output, 
                    "MIDI pitch for %3.3f is %s\n", 
                    this->voltage_source->get_voltage(), 
                    get_note_name(get_voltage_pitch()).c_str()
                );
                return extra_output;
            }
            return "";
        }

        virtual bool supports_pitch() override {
            //return this->voltage_source->supports_pitch();
            return this->voltage_source->supports_pitch();
        }
        virtual uint8_t get_voltage_pitch() {
            //return get_midi_pitch_for_voltage(this->voltage_source->get_voltage_pitch());
            if (this->voltage_source==nullptr) 
                Serial.printf(F("%c#get_voltage_pitch() has no voltage_source?!"), this->name); Serial_flush();
            return this->voltage_source->get_voltage_pitch();
        }

        virtual void read() override {
            double currentValue = this->voltage_source->get_voltage_normal();
            
            if (this->is_significant_change(currentValue, this->lastValue)) {
                this->lastValue = this->currentValue;
                this->currentValue = currentValue;
                //Serial.printf("%c: Setting this->currentValue to ", this->name);
                //Serial.println(currentValue);
                //this->currentValue = currentValue;
                //this->currentValue = currentValue = this->get_normal_value(currentValue);
                #ifdef ENABLE_PRINTF
                if (this->debug) {
                    /*Serial.printf("%s: VoltageParameterInput->read() got intermediate %i, voltage ", this->name, intermediate);
                    Serial.print((uint32_t) this->ads->toVoltage(intermediate));
                    Serial.printf(", final %i", (uint32_t) currentValue*1000.0);
                    Serial.println();*/
                    Serial.printf(F("%c: VoltageParameterInput->read() got voltage "), this->name); //
                    Serial.println(currentValue);
                }
                #endif

                double normal = this->get_normal_value(currentValue);

                #ifdef ENABLE_PRINTF
                if (this->debug) {
                    Serial.printf(F("VoltageParameterInput#read() for '%c': got currentValue "), this->name); Serial_flush();
                    Serial.print(currentValue); Serial_flush();
                    Serial.print(F(" converted to normal ")); Serial_flush();
                    Serial.println(normal); Serial_flush();
                }
                #endif

                if (this->callback != nullptr) {
                    if (this->debug) {
                        Serial.print(this->name);
                        Serial.print(F(": calling callback("));
                        Serial.print(normal);
                        Serial.println(F(")"));
                        Serial_flush();
                    }      
                    (*this->callback)(normal);
                }
                /*if (this->target_parameter!=nullptr) {
                    if (this->debug) {
                        Serial.println("Calling on target_parameter.."); Serial_flush();
                        Serial.print(this->name); Serial_flush();
                        Serial.print(F(": calling target from normal setParamValue(")); Serial_flush();
                        Serial.print(normal); Serial_flush();
                        Serial.print(F(")")); Serial_flush();
                        Serial.print(" from currentValue "); Serial_flush();
                        Serial.print(currentValue); Serial_flush();
                        if (this->inverted) { Serial.print(F(" - inverted")); Serial_flush(); }
                        Serial.println(); Serial_flush();

                        #ifdef ENABLE_PRINTF
                            Serial.printf("VoltageParameterInput %c calling setParamValue with maximum_input_voltage ", this->name);
                            Serial.println(this->voltage_source->maximum_input_voltage);
                        #endif
                    }
                    //this->target_parameter->setParamValue(normal, this->voltage_source->maximum_input_voltage);
                    this->target_parameter->updateValueFromNormal(normal); //
                }*/
                //Serial.println("Finishing read()"); Serial_flush();
            }
        }
};

#endif