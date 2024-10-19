#pragma once

#include "ParameterInput.h"
#include "AnalogParameterInputBase.h"

#include "../voltage_sources/VoltageSource.h"

#include "bpm.h"
#include "midi_helpers.h"

#include "ads.h"

#include <parameters/Parameter.h>

class VirtualMixerParameterInput : public AnalogParameterInputBase<float> {
    public:
        float source_value = 0.0f;

        DataParameter<VirtualMixerParameterInput,float> *underlying_parameter = nullptr;

        VirtualMixerParameterInput(char *name, const char *group_name) : AnalogParameterInputBase(name, group_name) {
            underlying_parameter = 
                new DataParameter<VirtualMixerParameterInput,float>(
                    name, 
                    this, 
                    &VirtualMixerParameterInput::set_source_value,
                    &VirtualMixerParameterInput::get_source_value,
                    -1.0f, 
                    1.0f
                );
            parameter_manager->addParameter(underlying_parameter);
        }

        virtual void set_source_value(float v) {
            source_value = v;
        }
        virtual float get_source_value() {
            return source_value;
        }

        virtual void read() override {
            //float currentValue = this->voltage_source->get_voltage_normal();
            this->lastValue = this->currentValue;
            this->currentValue = source_value;
        }

        virtual SubMenuItemBar *makeControls(int16_t memory_size, const char *label_prefix = "") override { 
            SubMenuItemBar *a = AnalogParameterInputBase::makeControls(TICKS_PER_PHRASE);
            LinkedList<MenuItem*> *p_controls = underlying_parameter->makeControls();
            menu->add(p_controls);
            return a;
        }

};