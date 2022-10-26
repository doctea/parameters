char NEXT_PARAMETER_NAME = 'A';

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
    MenuItem *DoubleParameter::makeControl() {
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
        for (int i = 0 ; i < MAX_SLOT_CONNECTIONS ; i++) {
            char slot_label[MAX_LABEL_LENGTH] = "        ";
            //sprintf(slot_label, "%i: Source", i);
            // need new source selector widget!
            
            sprintf(slot_label, "%i: Amount", i);
            DirectNumberControl<double> *amt = new DirectNumberControl<double>(
                (const char*)slot_label, 
                &this->connections[i].amount, 
                this->connections[i].amount, 
                -1.0, 
                1.0,
                (void (*)(double,double))nullptr
            );

            p_submenu->add(amt);
        }

        return p_submenu;
    }
#else
    #include "mymenu_items/ParameterInputMenuItems.h"

    MenuItem *DoubleParameter::makeControl() {
        Serial.printf("DataParameter#makeControl for %s\n", this->label);
        // first set up the submenu to hold the values
        ParameterMenuItem *fullmenuitem = new ParameterMenuItem(this->label, this);

        return fullmenuitem;
    }
    LinkedList<MenuItem *> *DoubleParameter::makeControls() {
        LinkedList<MenuItem *> *controls = new LinkedList<MenuItem *>();
        
        Serial.printf("DataParameter#makeControls for %s\n", this->label);
        // first set up the submenu to hold the values
        ParameterMenuItem *fullmenuitem = new ParameterMenuItem(this->label, this);
        controls->add(fullmenuitem);

        //while(!Serial);
        Serial.printf("in makeControls() in %s, parameter_manager is @%p and available_inputs is @%p\n", this->label, parameter_manager, parameter_manager->available_inputs);

        SubMenuItemBar *input_selectors_bar = new SubMenuItemBar("Inputs");
        //input_selectors_bar->debug = true;  // TODO: remove this !
        input_selectors_bar->add(new ParameterInputSelectorControl<DoubleParameter>(
            "Input 1", 
            this,
            &DoubleParameter::set_slot_0_input,
            parameter_manager->available_inputs,
            parameter_manager->getInputForName(this->get_input_name_for_slot(0)),
            fullmenuitem->items->get(1)     // second item of ParameterMenuItem is first slot
        ));
        input_selectors_bar->add(new ParameterInputSelectorControl<DoubleParameter>(
            "Input 2", 
            this,
            &DoubleParameter::set_slot_1_input,
            parameter_manager->available_inputs,
            parameter_manager->getInputForName(this->get_input_name_for_slot(1)),
            fullmenuitem->items->get(2)     // third item of ParameterMenuItem is second slot
        ));
        input_selectors_bar->add(new ParameterInputSelectorControl<DoubleParameter>(
            "Input 3", 
            this,
            &DoubleParameter::set_slot_2_input,
            parameter_manager->available_inputs,
            parameter_manager->getInputForName(this->get_input_name_for_slot(2)),
            fullmenuitem->items->get(3)     // fourth item of ParameterMenuItem is third slot
        ));
        controls->add(input_selectors_bar);
    }
#endif

#endif

// get the modulation amount to use
double DoubleParameter::get_modulation_value() {
        double modulation = 0.0f;
        int number_of_modulations = 0;
        for (int i = 0 ; i < MAX_SLOT_CONNECTIONS ; i++) {
            if (this->connections[i].parameter_input!=nullptr && this->connections[i].amount!=0.0) {
                modulation += (
                    this->connections[i].parameter_input->get_normal_value() * this->connections[i].amount
                );
                number_of_modulations++;
            }
        }
        if (this->debug && number_of_modulations>0) 
            Serial.printf("%s#get_modulation_value()\treturning\t%f from\t%i modulations\n", this->label, modulation, number_of_modulations);
        /*else {
            Serial.printf("%s#get_modulation_value() got no modulations\n", this->label);
        }*/
        return modulation;
        //this->parameter->modulateValue(modulation);
}
