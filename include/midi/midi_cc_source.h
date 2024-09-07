#ifndef MIDI_CC_SOURCE__INCLUDED
#define MIDI_CC_SOURCE__INCLUDED

#include "LinkedList.h"
#include "parameter_inputs/MIDIParameterInput.h"

#include "ParameterManager.h"
extern ParameterManager *parameter_manager;

// update parameter inputs from MIDI CC events
// make an instance of this object, or inherit from it in your object that receives MIDI events, and feed it via update_parameter_inputs_cc
class MIDI_CC_Source {
    public:
    
    LinkedList<MIDIParameterInput*> *parameter_inputs = new LinkedList<MIDIParameterInput*> ();

    // call this when you receive a CC change from your device to update the appropriate attached ParameterInput(s)
    void update_parameter_inputs_cc(uint8_t number, uint8_t value, uint8_t channel) {
        //Serial.printf("update_parameter_inputs_cc received %i, %i, %i\n", number, value, channel);
        const uint_fast16_t size = this->parameter_inputs->size();
        for (uint_fast16_t i = 0 ; i < size ; i++) {
            MIDIParameterInput *parameter_input = this->parameter_inputs->get(i);
            if (parameter_input->responds_to(number, channel)) {
                parameter_input->receive_control_change(number, value, channel);
            }
        }
    }

    // register a parameter input to this object
    FLASHMEM void addParameterInput(MIDIParameterInput *parameter_input) {
        this->parameter_inputs->add(parameter_input);
        parameter_manager->addInput(parameter_input);
    }
    //FLASHMEM 
    void addParameterInput(const char *name, const char *group_name, byte number, byte channel = 0) {
        this->addParameterInput(new MIDIParameterInput((char*)name, group_name, number, channel));
    }
};

#endif