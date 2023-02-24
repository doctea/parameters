/*#include "parameter_inputs/ParameterInput.h"

class ParameterToInputConnection {
    public:

    BaseParameterInput *parameter_input = nullptr;
    float amount = 0.0f;

    char get_input_name() {
        return this->parameter_input!=nullptr ? 
                this->parameter_input->name : // todo: doesnt compile (incomplete type BaseParameterInput) -- make getting the connection name into a method of Parameter?
                'X';   // use X instead of parameter name if no parameter label is set for that parameter
    }

    void set_input_from_name(char parameter_input_name) {

    }

};
*/