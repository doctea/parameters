#pragma once

#include "LinkedList.h"
#include "parameter_inputs/MIDIParameterInput.h"

#include "ParameterManager.h"
extern ParameterManager *parameter_manager;

// update parameter inputs from MIDI CC events
// make an instance of this object, or inherit from it in your object that receives MIDI events, and feed it via update_parameter_inputs_cc

// TODO: turn this into a Behaviour type of thing?
class MIDI_CC_Source {
    public:
    
    LinkedList<MIDIParameterInputBase*> *parameter_inputs = new LinkedList<MIDIParameterInputBase*> ();

    // call this when you receive a CC change from your device to update the appropriate attached ParameterInput(s)
    void update_parameter_inputs_cc(uint8_t number, uint8_t value, uint8_t channel) {
        //Serial.printf("update_parameter_inputs_cc received %i, %i, %i\n", number, value, channel);
        for (auto* parameter_input : *this->parameter_inputs) {
            if (parameter_input->responds_to_cc(number, channel)) {
                parameter_input->receive_control_change(number, value, channel);
            }
        }
    }

    void update_parameter_inputs_pitch_bend(int16_t value, uint8_t channel) {
        for (auto* parameter_input : *this->parameter_inputs) {
            if (parameter_input->responds_to_pitch_bend(channel)) {
                parameter_input->receive_pitch_bend(value, channel);
            }
        }
    }

    // register a MIDI CC parameter input to this object
    FLASHMEM void addParameterInput(MIDIParameterInputBase *parameter_input) {
        this->parameter_inputs->add(parameter_input);
        parameter_manager->addInput(parameter_input);
    }

    // add a new MIDIParameterInput to this object and the parameter manager
    void addCCParameterInput(const char *name, const char *group_name, byte number, byte channel = 0) {
        this->addParameterInput(new MIDICCParameterInput((char*)name, group_name, number, channel));
    }

    // add a new MIDIPitchBendParameterInput to this object and the parameter manager
    void addPitchBendParameterInput(const char *name, const char *group_name, byte channel = 0) {
        this->addParameterInput(new MIDIPitchBendParameterInput((char*)name, group_name, channel));
    }
};

// todo: add MIDI_PitchBend_Source 