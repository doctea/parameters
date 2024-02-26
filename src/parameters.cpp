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

// just main control, with amounts 
FLASHMEM MenuItem *FloatParameter::makeControl() {
    Debug_printf(F("FloatParameter#makeControl for %s\n"), this->label);
    // first set up the submenu to hold the values
    ParameterValueMenuItem *menuitem = new ParameterValueMenuItem(this->label, &self);

    return menuitem;
}

FLASHMEM MenuItem *FloatParameter::makeInputSelectorControls(ParameterMenuItem *fullmenuitem) {
    // controls to choose which ParameterInputs to use for each slot
    // then set up a generic submenuitembar to hold the input selectors
    SubMenuItemBar *input_selectors_bar = new SubMenuItemBar("Inputs");
    input_selectors_bar->show_header = false;
    input_selectors_bar->show_sub_headers = false;

    // some spacers so that the input controls align with the corresponding amount controls
    MenuItem *spacer1 = new MenuItem("Inputs");
    spacer1->selectable = false;           
    input_selectors_bar->add(spacer1);

    // make the three source selector controls
    ParameterInputSelectorControl<FloatParameter> *source_selector_1 = new ParameterInputSelectorControl<FloatParameter>(
        "Input 1", 
        this,
        &FloatParameter::set_slot_0_input,
        &FloatParameter::get_slot_0_input,
        parameter_manager->available_inputs,
        parameter_manager->getInputForName(this->get_input_name_for_slot(0)),
        fullmenuitem->items->get(1)     // second item of ParameterMenuItem is first slot
    );
    source_selector_1->go_back_on_select = true;

    ParameterInputSelectorControl<FloatParameter> *source_selector_2 = new ParameterInputSelectorControl<FloatParameter>(
        "Input 2", 
        this,
        &FloatParameter::set_slot_1_input,
        &FloatParameter::get_slot_1_input,
        parameter_manager->available_inputs,
        parameter_manager->getInputForName(this->get_input_name_for_slot(1)),
        fullmenuitem->items->get(2)     // third item of ParameterMenuItem is second slot
    );
    source_selector_2->go_back_on_select = true;

    ParameterInputSelectorControl<FloatParameter> *source_selector_3 = new ParameterInputSelectorControl<FloatParameter>(
        "Input 3", 
        this,
        &FloatParameter::set_slot_2_input,
        &FloatParameter::get_slot_2_input,
        parameter_manager->available_inputs,
        parameter_manager->getInputForName(this->get_input_name_for_slot(2)),
        fullmenuitem->items->get(3)     // fourth item of ParameterMenuItem is third slot
    );
    source_selector_3->go_back_on_select = true;

    input_selectors_bar->add(source_selector_1);
    input_selectors_bar->add(source_selector_2);
    input_selectors_bar->add(source_selector_3);

    // empty column at end of bar
    MenuItem *spacer2 = new MenuItem("");
    spacer2->selectable = false;
    input_selectors_bar->add(spacer2);

    return input_selectors_bar;
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

    if (!this->is_modulatable()) {
        Debug_printf(F("WARNING: %s is NOT modulatable but asked to create modulatable controls!"), this->label);
        controls->add(this->makeControl());
        return controls;
    }
    // first, set up the submenu to hold the controls for the amounts
    // this is handled by its own compound control type, 'ParameterMenuItem'
    ParameterMenuItem *fullmenuitem = new ParameterMenuItem(this->label, &this->self);
    controls->add(fullmenuitem);

    #ifndef DISABLE_PARAMETER_INPUT_SELECTORS
        controls->add(this->makeInputSelectorControls(fullmenuitem));
    #endif
    
    #ifndef DISABLE_PARAMETER_POLARITY_SELECTORS
        // add controls to select whether to use bipolar or unipolar values from the ParameterInput
        SubMenuItemBar *polarity_selectors_bar = new SubMenuItemBar("Polarity");
        polarity_selectors_bar->show_header = false;
        polarity_selectors_bar->show_sub_headers = false;

        MenuItem *polarity_spacer_1 = new MenuItem("Polarity");
        polarity_spacer_1->selectable = false;           
        polarity_selectors_bar->add(polarity_spacer_1);

        // make the three polarity selector controls
        ParameterConnectionPolarityTypeSelectorControl *polarity_selector_1 = new ParameterConnectionPolarityTypeSelectorControl(
            "Input 1 Polarity",
            &this->self,
            0
        );
        ParameterConnectionPolarityTypeSelectorControl *polarity_selector_2 = new ParameterConnectionPolarityTypeSelectorControl(
            "Input 2 Polarity",
            &this->self,
            1
        );
        ParameterConnectionPolarityTypeSelectorControl *polarity_selector_3 = new ParameterConnectionPolarityTypeSelectorControl(
            "Input 3 Polarity",
            &this->self,
            2
        );
        polarity_selectors_bar->add(polarity_selector_1);
        polarity_selectors_bar->add(polarity_selector_2);
        polarity_selectors_bar->add(polarity_selector_3);

        MenuItem *polarity_spacer_2 = new MenuItem("");
        polarity_spacer_2->selectable = false;           
        polarity_selectors_bar->add(polarity_spacer_2);

        controls->add(polarity_selectors_bar);
    #endif

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

        return (this->connections[slot].parameter_input!=nullptr && abs(this->connections[slot].amount) > 0.02);
    }
    // get the modulation amount to use
    float FloatParameter::get_modulation_value() {
        float modulation = 0.0f;
        int_fast8_t number_of_modulations = 0;
        if (!is_modulation_slot_active(0) && !is_modulation_slot_active(1) && !is_modulation_slot_active(2))
            return 0.0f;
            
        for (uint_fast8_t i = 0 ; i < MAX_SLOT_CONNECTIONS ; i++) {
            if (is_modulation_slot_active(i)) {
                float nml = this->connections[i].polar_mode==UNIPOLAR ? 
                                this->connections[i].parameter_input->get_normal_value_unipolar() : 
                                this->connections[i].parameter_input->get_normal_value_bipolar();
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