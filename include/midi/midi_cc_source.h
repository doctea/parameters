#include "LinkedList.h"
#include "parameter_inputs/MIDIParameterInput.h"

#include "ParameterManager.h"
extern ParameterManager *parameter_manager;

// make an instance of this object, or inherit from it in your object that receives MIDI events
class MIDI_CC_Source {
    public:
    
    LinkedList<MIDIParameterInput*> *parameter_inputs = new LinkedList<MIDIParameterInput*> ();

    // call this when you receive a CC change from your device to update the appropriate attached ParameterInput(s)
    void update_parameter_inputs_cc(uint8_t number, uint8_t value, uint8_t channel) {
        //Serial.printf("update_parameter_inputs_cc received %i, %i, %i\n", number, value, channel);
        for (int i = 0 ; i < this->parameter_inputs->size() ; i++) {
            if (this->parameter_inputs->get(i)->responds_to(number, channel)) {
                this->parameter_inputs->get(i)->receive_control_change(number, value, channel);
            }
        }
    }

    // register a parameter input to this object
    FLASHMEM void addParameterInput(MIDIParameterInput *parameter_input) {
        this->parameter_inputs->add(parameter_input);
        parameter_manager->addInput(parameter_input);
    }
    FLASHMEM void addParameterInput(const char *name, byte number, byte channel = 0) {
        this->addParameterInput(new MIDIParameterInput((char*)name, number, channel));
    }
};