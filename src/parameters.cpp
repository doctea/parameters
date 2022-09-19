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

/*MenuItem *DoubleParameter::makeControl() {
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
}*/


MenuItem *DoubleParameter::makeControl() {
    Serial.printf("DataParameter#makeControl for %s\n", this->label);
    // first set up the submenu to hold the values
    ParameterMenuItem *fullmenuitem = new ParameterMenuItem(this->label, this);

    return fullmenuitem;
}

#endif

double DoubleParameter::get_modulation_value() {
        // get the modulation amount to use
        double modulation = 0.0f;
        int number_of_modulations = 0;
        for (int i = 0 ; i < MAX_SLOT_CONNECTIONS ; i++) {
            if (this->connections[i].parameter_input!=nullptr) {
                modulation += (
                    this->connections[i].parameter_input->get_normal_value() * this->connections[i].amount
                );
                number_of_modulations++;
            }
        }
        if (this->debug && number_of_modulations>0) 
            Serial.printf("%s#get_modulation_value() returning %f from %i modulations\n", this->label, modulation, number_of_modulations);
        return modulation;
        //this->parameter->modulateValue(modulation);
}