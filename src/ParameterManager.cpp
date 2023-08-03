/*#include "ParameterManager.h"

#ifdef ENABLE_SCREEN

    #include "parameter_inputs/ParameterInput.h"
    #include "mymenu_items/ParameterInputViewMenuItems.h"

    void ParameterManager::addAllParameterInputMenuItems(Menu *menu) {
        Serial.println("ParameterManager#addAllParameterInputMenuItems()...");
        for (unsigned int i = 0 ; i < this->available_inputs.size() ; i++) {
            // TODO: probably merge these things into one new type of MenuItem?
            BaseParameterInput *input = this->available_inputs.get(i);
            //Serial.printf("\t%i: doing menu->add for makeMenuItemForParameterInput()..", i);
            //menu->add(this->makeMenuItemForParameterInput(param));

            //this->addParameterInputMenuItems(menu, input);           
            char label[20];
            sprintf(label, "Graph for %c", this->name);

            Serial.printf("\tdoing menu->add for ParameterInputDisplay with label '%s'\n", label);
            input->addMenuControls(menu);
            menu->add(new ParameterInputDisplay(label, this); //, LOOP_LENGTH_TICKS));
            sprintf(label, "Input type for %c", this->name);
            menu->add(new InputTypeSelectorControl(label, input, set_input_type, get_input_type));
            sprintf(label, "Output type for %c", this->name);
            menu->add(new InputTypeSelectorControl(label, input, set_output_type, get_output_type));
        }
    }

#endif*/
/*
#include "ParameterManager.h"

// todo: different implementations depending on whether we have SD card available or not
bool ParameterManager::load_voltage_calibration(int slot) {
    VoltageSourceBase 
}
bool ParameterManager::save_voltage_calibration(int slot) {

}*/

#include "ParameterManager.h"

LinkedList<BaseParameterInput*> *ParameterManager::get_available_pitch_inputs() {
    static LinkedList<BaseParameterInput*> *available_pitch_inputs = new LinkedList<BaseParameterInput*>();
    static bool already_calculated = false;
    if (!already_calculated) {
        for (uint_fast8_t i = 0 ; i < available_inputs->size() ; i++) {
            if (available_inputs->get(i)->supports_pitch())
                available_pitch_inputs->add(available_inputs->get(i));
        }
        already_calculated = true;
    }
    return available_pitch_inputs;
}

FLASHMEM ADCDeviceBase *ParameterManager::addADCDevice(ADCDeviceBase *device) {
    Debug_printf(F("ParameterManager#addADCDevice(%p)\n"), device);
    this->devices->add(device);
    return device;
}

FLASHMEM VoltageSourceBase *ParameterManager::addVoltageSource(VoltageSourceBase *voltage_source) {
    Debug_printf(F("ParameterManager#addVoltageSource(%p)\n"), voltage_source);
    #if defined(LOAD_CALIBRATION_ON_BOOT) && defined(ENABLE_CALIBRATION_STORAGE)
        //Debug_printf(F("Loading calibration for %i!\n"), voltage_source->slot);
        Debug_printf("Loading calibration for %i!\n", voltage_source->slot);
        voltage_source->load_calibration();
    #endif

    this->voltage_sources->add(voltage_source);
    return voltage_source;
}

BaseParameterInput *ParameterManager::addInput(BaseParameterInput *input) {
    Debug_printf(F("ParameterManager#addInput(%p)\n"), input);
    #ifdef ENABLE_SCREEN
        input->colour = parameter_input_colours[this->available_inputs->size() % (sizeof(parameter_input_colours)/2)];
    #endif
    this->available_inputs->add(input);
    return input;
}

FLASHMEM FloatParameter *ParameterManager::addParameter(FloatParameter *parameter) {
    Debug_printf(F("ParameterManager#addParameter(%p), labeled '%s'\n"), parameter, parameter->label);
    this->available_parameters->add(parameter);
    return parameter;
}

FLASHMEM void ParameterManager::addParameters(LinkedList<FloatParameter*> *parameters) {
    Debug_println(F("ParameterManager#addParameters()..")); Serial_flush();
    Debug_printf(F("\t\tpassed @%p, has size %i\n"), parameters, parameters->size()); Serial_flush();
    for (unsigned int i = 0 ; i < parameters->size() ; i++) {
        Debug_printf(F("\t%i: adding from @%p '%s'\n"), i, parameters->get(i), parameters->get(i)->label); Serial_flush();
        //this->available_parameters->add(parameters->get(i));
        this->addParameter(parameters->get(i));
        Debug_printf(F("..added\n")); Serial_flush();
    }
    Debug_println(F("finished in addParameters"));
}