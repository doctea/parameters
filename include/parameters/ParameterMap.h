// map a parameter to an input

//#include "parameters/Parameter.h"
//#include "parameter_inputs/ParameterInput.h"

/*class BaseParameterInput;

struct ParameterToInputConnection {
    //BaseParameter *parameter = nullptr;
    BaseParameterInput *parameter_input = nullptr;
    double amount = 0.0f;
    //bool volt_per_octave = false;
};*/

//#define MAX_CONNECTIONS 3
/*
class ParameterMixer {
    //BaseParameter *parameter;
    ParameterToInputConnection connections[MAX_CONNECTIONS];
    
    //ParameterMixer (BaseParameter *parameter) {
        //this->parameter = parameter;
    //}

    ParameterToInputConnection *make_connection(byte source_number, BaseParameterInput *parameter_input) {
        this->connections[source_number].parameter_input = parameter_input;        
    }

    int find_empty_slot() {
        for (int i = 0 ; i < MAX_CONNECTIONS ; i++) {
            if (connections[i].parameter_input==nullptr)
                return i;
        }
        return -1;
    }
    void connect_input(BaseParameterInput *parameter_input, double amount) {
        int slot = find_empty_slot();
        if (slot==-1) return;

        set_slot_input(slot, parameter_input);
        set_slot_amount(slot, amount);
    }
    bool disconnect_input(byte slot) { 
        this->connections[slot].parameter_input = nullptr;
    }

    void set_slot_input(byte slot, BaseParameterInput *parameter_input) {
        this->connections[slot].parameter_input = parameter_input;
    }
    void set_slot_amount(byte slot, double amount) {
        this->connections[slot].amount = amount;
    }

    void set_slot_0_input(BaseParameterInput *parameter_input) {
        this->set_slot_input(0,parameter_input);
    }
    void set_slot_0_amount(double amount) {
        this->set_slot_amount(0, amount);
    }
    void set_slot_1_input(BaseParameterInput *parameter_input) {
        this->set_slot_input(0,parameter_input);
    }
    void set_slot_1_amount(double amount) {
        this->set_slot_amount(0, amount);
    }
    void set_slot_2_input(BaseParameterInput *parameter_input) {
        this->set_slot_input(0,parameter_input);
    }
    void set_slot_2_amount(double amount) {
        this->set_slot_amount(0, amount);
    }

    //void changeValue(BaseParameterInput *parameter_input) {
    //    // find the parameterinput we've been passed
    //   // update and send the actual value
    //}

    double get_modulation_value() {
        // get the modulation amount to use
        double modulation = 0.0f;
        for (int i = 0 ; i < MAX_CONNECTIONS ; i++) {
            if (this->connections[i].parameter_input!=nullptr)
                modulation += (
                    this->connections[i].parameter_input->get_normal_value() * this->connections[i].amount
                );
        }
        return modulation;
        //this->parameter->modulateValue(modulation);
    }
};
*/