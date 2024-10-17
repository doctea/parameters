#pragma once

#include "ParameterInput.h"
#include "AnalogParameterInputBase.h"

#include "../voltage_sources/VoltageSource.h"

#include "midi_helpers.h"

#include "ads.h"

class VoltageParameterInput : public AnalogParameterInputBase<float> {
    VoltageSourceBase *voltage_source = nullptr;

    public:
        VoltageParameterInput(char *name, const char *group_name, VoltageSourceBase *voltage_source) : AnalogParameterInputBase(name, group_name) {
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
                snprintf(
                    extra_output, 
                    40,
                    "MIDI pitch for %3.3f is %s\n", 
                    this->voltage_source->get_voltage(), 
                    get_note_name(get_voltage_pitch()).c_str()
                );
                return extra_output;
            }
            return "";
        }

        virtual bool supports_pitch() override {
            return this->voltage_source->supports_pitch();
        }
        virtual uint8_t get_voltage_pitch() {
            //return get_midi_pitch_for_voltage(this->voltage_source->get_voltage_pitch());
            if (this->voltage_source==nullptr) 
                Debug_printf("%c#get_voltage_pitch() has no voltage_source?!", this->name); Serial_flush();
            return this->voltage_source->get_voltage_pitch();
        }

        virtual void read() override {
            float currentValue = this->voltage_source->get_voltage_normal();
            
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
                    Debug_printf("%c: VoltageParameterInput->read() got voltage ", this->name); //
                    Debug_println(currentValue);
                }
                #endif

                /*#ifdef ENABLE_PRINTF
                if (this->debug) {
                    Debug_printf("VoltageParameterInput#read() for '%c': got currentValue ", this->name); Serial_flush();
                    Debug_print(currentValue); Serial_flush();
                    Debug_print(" converted to normal "); Serial_flush();
                    Debug_println(normal); Serial_flush();
                }
                #endif*/

                #ifdef PARAMETER_INPUTS_USE_CALLBACKS
                    float normal = this->get_normal_value_unipolar(currentValue);
                    this->on_value_read(normal);
                    if (this->callback != nullptr) {
                        if (this->debug) {
                            Debug_print(this->name);
                            Debug_print(": calling callback(");
                            Debug_print(normal);
                            Debug_println(")");
                            Serial_flush();
                        }      
                        (*this->callback)(normal);
                    }
                #endif

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
