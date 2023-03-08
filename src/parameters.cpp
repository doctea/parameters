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

#ifdef MENU_SIMPLE_PARAMETERS
    //old, untested
    FLASHMEM MenuItem *FloatParameter::makeControl() {
        Serial.printf("DataParameter#makeControl for %s\n", this->label);

        //char menu_label[MAX_LABEL_LENGTH];
        //sprintf(menu_label, "%s", this->label);

        // first set up the submenu to hold the values
        SubMenuItem *p_submenu = new SubMenuItem(this->label, false);

        // first set up the actual parameter value changer    
        ParameterValueMenuItem *mi = new ParameterValueMenuItem(this->label, this);
        mi->minimum_value = this->minimumNormalValue;
        mi->maximum_value = this->maximumNormalValue;
        mi->internal_value = this->getCurrentNormalValue() * 100.0; // * this->maximum_value;
        Serial.printf("makeControl() in %s:- getCurrentValue()=", this->label);
        Serial.print(this->getCurrentNormalValue());
        Serial.printf(", maximum_value=");
        Serial.print(this->maximumNormalValue);
        Serial.println("<<<<<<<<<<<<<");
        Serial.print("\tInitialised internal_value to ");
        Serial.println(mi->internal_value);

        p_submenu->add(mi);

        // then we set up the slot menus
        for (unsigned int i = 0 ; i < MAX_SLOT_CONNECTIONS ; i++) {
            char slot_label[MAX_LABEL_LENGTH] = "        ";
            //sprintf(slot_label, "%i: Source", i);
            // need new source selector widget!
            
            snprintf(slot_label, MAX_LABEL_LENGTH, "%i: Amount", i);
            DirectNumberControl<float> *amt = new DirectNumberControl<float>(
                (const char*)slot_label, 
                &this->connections[i].amount, 
                this->connections[i].amount, 
                -1.0, 
                1.0,
                (void (*)(float,float))nullptr
            );

            p_submenu->add(amt);
        }

        return p_submenu;
    }
#else
    #include "mymenu_items/ParameterInputMenuItems.h"

    // just main control, with amounts 
    FLASHMEM MenuItem *FloatParameter::makeControl() {
        Debug_printf(F("FloatParameter#makeControl for %s\n"), this->label);
        // first set up the submenu to hold the values
        ParameterValueMenuItem *menuitem = new ParameterValueMenuItem(this->label, this);

        return menuitem;
    }
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
        ParameterMenuItem *fullmenuitem = new ParameterMenuItem(this->label, this);
        controls->add(fullmenuitem);

        // then set up a generic submenuitembar to hold the input selectors
        SubMenuItemBar *input_selectors_bar = new SubMenuItemBar("Inputs");
        input_selectors_bar->show_header = false;
        input_selectors_bar->show_sub_headers = false;

        // some spacers so that the input controls align with the corresponding amount controls
        MenuItem *spacer1 = new MenuItem("Inputs");
        MenuItem *spacer2 = new MenuItem("");
        spacer1->selectable = false;
        spacer2->selectable = false;
        input_selectors_bar->add(spacer1);

        // make the three source selector controls
        ParameterInputSelectorControl<FloatParameter> *source_selector_1 = new ParameterInputSelectorControl<FloatParameter>(
            "Input 1", 
            this,
            &FloatParameter::set_slot_0_input,
            parameter_manager->available_inputs,
            parameter_manager->getInputForName(this->get_input_name_for_slot(0)),
            fullmenuitem->items->get(1)     // second item of ParameterMenuItem is first slot
        );
        source_selector_1->go_back_on_select = true;

        ParameterInputSelectorControl<FloatParameter> *source_selector_2 = new ParameterInputSelectorControl<FloatParameter>(
            "Input 2", 
            this,
            &FloatParameter::set_slot_1_input,
            parameter_manager->available_inputs,
            parameter_manager->getInputForName(this->get_input_name_for_slot(1)),
            fullmenuitem->items->get(2)     // third item of ParameterMenuItem is second slot
        );
        source_selector_2->go_back_on_select = true;

        ParameterInputSelectorControl<FloatParameter> *source_selector_3 = new ParameterInputSelectorControl<FloatParameter>(
            "Input 3", 
            this,
            &FloatParameter::set_slot_2_input,
            parameter_manager->available_inputs,
            parameter_manager->getInputForName(this->get_input_name_for_slot(2)),
            fullmenuitem->items->get(3)     // fourth item of ParameterMenuItem is third slot
        );
        source_selector_3->go_back_on_select = true;

        // tell the parameter's connection mappings which screen controls they need to update
        this->link_parameter_input_controls_to_connections(
            fullmenuitem->items->get(1),
            fullmenuitem->items->get(2),
            fullmenuitem->items->get(3),
            source_selector_1,
            source_selector_2,
            source_selector_3
        );

        input_selectors_bar->add(source_selector_1);
        input_selectors_bar->add(source_selector_2);
        input_selectors_bar->add(source_selector_3);
        input_selectors_bar->add(spacer2);

        controls->add(input_selectors_bar);

        return controls;
    }
#endif

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
    // get the modulation amount to use
    float FloatParameter::get_modulation_value() {
            float modulation = 0.0f;
            int_fast8_t number_of_modulations = 0;
            if (this->connections[0].amount==0.0f && this->connections[1].amount==0.0f && this->connections[2].amount==0.0f)
                return 0.0f;

            for (unsigned int i = 0 ; i < MAX_SLOT_CONNECTIONS ; i++) {
                if (this->connections[i].parameter_input!=nullptr && this->connections[i].amount!=0.0f) {
                    modulation += (
                        this->connections[i].parameter_input->get_normal_value() * this->connections[i].amount
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