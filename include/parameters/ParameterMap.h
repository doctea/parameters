// map a parameter to an input

#include "parameters/Parameter.h"
#include "parameter_inputs/ParameterInput.h"

struct ParameterToInputConnection {
    BaseParameter *parameter = nullptr;
    BaseParameterInput *parameter_input = nullptr;
    double amount = 0.0f;
    //bool volt_per_octave = false;
};

class ParameterMixer {
    BaseParameter *parameter;
    ParameterToInputConnection connections[3];
    
    ParameterMixer (BaseParameter *parameter) {
        this->parameter = parameter;
    }

    ParameterToInputConnection *make_connection(byte source_number, BaseParameterInput *parameter_input) {
        this->connections[source_number].parameter_input = parameter_input;        
    }

    void set_amount(byte source_number, double amount) {
        this->connections[source_number].amount = amount;
        // update parameter
    }

    void changeValue(BaseParameterInput *parameter_input) {
        // find the parameterinput we've been passed
        // update and send the actual value
    }

    void updateOutput() {
        // get the modulation amount to use
        double modulation = 0.0f;
        for (int i = 0 ; i < 3 ; i++) {
            if (this->connections[i].parameter_input!=nullptr)
                modulation += this->connections[i].parameter_input->get_normal_value();
        }
        this->parameter->modulateValue(modulation);
    }

};