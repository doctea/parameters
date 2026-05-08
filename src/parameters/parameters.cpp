#include "parameters/Parameter.h"
#include "parameter_inputs/ParameterInput.h"

#ifdef ENABLE_SCREEN 

//template<class TargetClass, class DataType>
/*void DataParameter::on_unbound(BaseParameterInput *input) {
    this->setParamValue(0.0f);
}*/

#include "menu.h"

//#include "Parameter.h"
#include "parameters/ToggleParameter.h"
#include "mymenu_items/ToggleMenuItems.h"
#include "menuitems_numbers.h"
#include "submenuitem.h"

#include "mymenu_items/ParameterInputMenuItems.h"
#include "mymenu_items/ParameterMenuItems_Range.h"

#include "menuitems_graphical_display.h"

// just main control, with (no?) amounts 
FLASHMEM MenuItem *FloatParameter::makeControl() {
    Debug_printf(F("FloatParameter#makeControl for %s\n"), this->label);
    // first set up the submenu to hold the values
    ParameterValueMenuItem *menuitem = new ParameterValueMenuItem(this->label, &self);

    return menuitem;
}

/*FLASHMEM LinkedList<MenuItem *> *FloatParameter::makeControls() {
    // dummy version for debugging/testing
    LinkedList<MenuItem *> *controls = new LinkedList<MenuItem *>();
    return controls;
}*/

FLASHMEM LinkedList<MenuItem *> *FloatParameter::makeControls() {
    Debug_printf(F("FloatParameter#makeControls for %s - is_modulatable is %s\n"), this->label, this->is_modulatable() ? "true" : "false");

    // list for storing all the controls we're about to add
    LinkedList<MenuItem *> *controls = new LinkedList<MenuItem *>();

    // add any custom controls that the particular parameter type wants to add (eg CC+channel selectors for MIDICCParameters)
    this->addCustomTypeControls(controls);

    // if can't be modulated then just add simple control
    // todo: is this actually used by anything?  what does it do?
    if (!this->is_modulatable()) {
        Debug_printf(F("WARNING: %s is NOT modulatable but asked to create modulatable controls!"), this->label);
        controls->add(this->makeControl());
        return controls;
    }

    // Value|Min|Max|Output top row
    ParameterMenuItem *fullmenuitem = new ParameterMenuItem(this->label, &this->self, false);
    controls->add(fullmenuitem);

    // One row per modulation slot — added as separate top-level items for direct navigation
    for (uint_fast8_t i = 0; i < MAX_SLOT_CONNECTIONS; i++) {
        char slot_label[8];
        snprintf(slot_label, sizeof(slot_label), "Slt%i", i + 1);
        controls->add(new ParameterModSlotRow(slot_label, &this->self, i));
    }

    //ToggleControl<bool> *debug = new ToggleControl<bool>("Debug", &this->debug, nullptr);
    //controls->add(debug);
    //controls->add(new GraphicalValueDisplay<float>("Value", &this->lastOutputNormalValue, this->minimumNormalValue, this->maximumNormalValue));

    return controls;
}
#endif

#define USE_REAL_FLOATS
#ifndef USE_REAL_FLOATS
    // NOT WORKING PROPERLY !!!! get the modulation amount to use
    float FloatParameter::get_modulation_value() {
            int_fast16_t modulation = 0;
            int_fast8_t number_of_modulations = 0;
            if (this->connections[0].amount==0.0f && this->connections[1].amount==0.0f && this->connections[2].amount==0.0f)
                return 0.0f;

            for (int_fast8_t i = 0 ; i < MAX_SLOT_CONNECTIONS ; i++) {
                if (this->connections[i].parameter_input!=nullptr && this->connections[i].amount!=0.0f) {
                    int amt_int = this->connections[i].amount * 100;
                    int nml_int = this->connections[i].parameter_input->get_normal_value() * 100;
                    modulation += (
                        amt_int * nml_int
                        //this->connections[i].parameter_input->get_normal_value() * this->connections[i].amount
                    );
                    number_of_modulations++;
                }
            }
            if (this->debug && number_of_modulations>0) 
                Debug_printf(F("%s#get_modulation_value()\treturning\t%f from\t%i modulations\n"), this->label, modulation, number_of_modulations);
            /*else {
                Serial.printf("%s#get_modulation_value() got no modulations\n", this->label);
            }*/
            return ((float)modulation)/100.0f;
            //this->parameter->modulateValue(modulation);
    }
#else
    // account for rounding errors that are causing spurious modulation
    bool FloatParameter::is_modulation_slot_active(int slot) {
        if (!is_valid_slot(slot)) return false;

        return (this->connections[slot].parameter_input!=nullptr && abs(this->connections[slot].amount) >= MODULATION_THRESHOLD);
    }
    // get the modulation amount to use
    float FloatParameter::get_modulation_value() {
        float modulation = 0.0f;
        int_fast8_t number_of_modulations = 0;
        if (!is_modulation_slot_active(0) && !is_modulation_slot_active(1) && !is_modulation_slot_active(2))
            return 0.0f;
            
        for (uint_fast8_t i = 0 ; i < MAX_SLOT_CONNECTIONS ; i++) {
            if (is_modulation_slot_active(i)) {
                float nml = 0.0f;
                switch (this->connections[i].polar_mode) {
                    case MOD_SLOT_BI_NATIVE:
                        nml = this->connections[i].parameter_input->get_normal_value_bipolar();
                        break;
                    case MOD_SLOT_UNI_CENTERED:
                        nml = this->connections[i].parameter_input->get_normal_value_unipolar();
                        nml = -1.0f + (nml * 2.0f);
                        break;
                    case MOD_SLOT_UNI_RAW:
                    default:
                        nml = this->connections[i].parameter_input->get_normal_value_unipolar();
                        break;
                }
                modulation += (
                    nml * this->connections[i].amount
                );
                number_of_modulations++;
            }
        }
        if (this->debug && number_of_modulations>0) 
            Debug_printf(F("%s#get_modulation_value()\treturning\t%f from\t%i modulations\n"), this->label, modulation, number_of_modulations);
        /*else {
            Serial.printf("%s#get_modulation_value() got no modulations\n", this->label);
        }*/
        return modulation;
        //this->parameter->modulateValue(modulation);
    }
#endif


// todo: merge this with Parameter.cpp