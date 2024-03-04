#include "parameters/Parameter.h"
#include "mymenu_items/ParameterMenuItems_lowmemory.h"

lowmemory_controls_t lowmemory_controls;

void create_low_memory_parameter_controls(const char *label, LinkedList<FloatParameter*> *parameters, int_fast16_t default_fg) {
    ////// control to select which parameter the other controls will edit
    ParameterMenuItemSelector *parameter_selector = new ParameterMenuItemSelector(label, parameters);
    //if (lowmemory_controls.parameter_selector==nullptr)
        //lowmemory_controls.parameter_selector = new ParameterMenuItemSelector(label, parameters);
    menu->add(new LowMemorySwitcherMenuItem((char*)"switcher", parameters, parameter_selector, default_fg));
    lowmemory_controls.parameter = parameter_selector->parameter;

    menu->add(parameter_selector);

    ////// amount controls, to set percentage amounts 
    //ParameterMenuItem *parameter_amount_controls;
    if (lowmemory_controls.parameter_amount_controls==nullptr)
        lowmemory_controls.parameter_amount_controls = new ParameterMenuItem("Amounts", &lowmemory_controls.parameter);
    menu->add(lowmemory_controls.parameter_amount_controls);

    // controls to choose which ParameterInputs to use for each slot
    // then set up a generic submenuitembar to hold the input selectors
    if (lowmemory_controls.input_selectors_bar==nullptr) {
        lowmemory_controls.input_selectors_bar = new SubMenuItemBar("Inputs");
        lowmemory_controls.input_selectors_bar->show_header = false;
        lowmemory_controls.input_selectors_bar->show_sub_headers = false;

        // some spacers so that the input controls align with the corresponding amount controls
        MenuItem *spacer1 = new MenuItem("Inputs");
        spacer1->selectable = false;           
        lowmemory_controls.input_selectors_bar->add(spacer1);

        // make the three source selector controls
        ParameterInputSelectorControl<FloatParameter> *source_selector_1 = new ParameterInputSelectorControl<FloatParameter>(
            "Input 1", 
            &lowmemory_controls.parameter,
            &FloatParameter::set_slot_0_input,
            &FloatParameter::get_slot_0_input,
            parameter_manager->available_inputs
        );
        source_selector_1->go_back_on_select = true;

        ParameterInputSelectorControl<FloatParameter> *source_selector_2 = new ParameterInputSelectorControl<FloatParameter>(
            "Input 2", 
            &lowmemory_controls.parameter,
            &FloatParameter::set_slot_1_input,
            &FloatParameter::get_slot_1_input,
            parameter_manager->available_inputs
        );
        source_selector_2->go_back_on_select = true;

        ParameterInputSelectorControl<FloatParameter> *source_selector_3 = new ParameterInputSelectorControl<FloatParameter>(
            "Input 3", 
            &lowmemory_controls.parameter,
            &FloatParameter::set_slot_2_input,
            &FloatParameter::get_slot_2_input,
            parameter_manager->available_inputs
        );
        source_selector_3->go_back_on_select = true;

        lowmemory_controls.input_selectors_bar->add(source_selector_1);
        lowmemory_controls.input_selectors_bar->add(source_selector_2);
        lowmemory_controls.input_selectors_bar->add(source_selector_3);

        // empty column at end of bar
        MenuItem *spacer2 = new MenuItem("");
        spacer2->selectable = false;
        lowmemory_controls.input_selectors_bar->add(spacer2);
    }
    menu->add(lowmemory_controls.input_selectors_bar);

    ////// polarity controls
    if (lowmemory_controls.polarity_submenu==nullptr) {
        lowmemory_controls.polarity_submenu = new SubMenuItemBar("Polarities", false);
        lowmemory_controls.polarity_submenu->show_header = false;
        lowmemory_controls.polarity_submenu->add(new MenuItem("Polarity"));
        lowmemory_controls.polarity_submenu->add(new ParameterConnectionPolarityTypeSelectorControl("Slot 1", &lowmemory_controls.parameter, 0));
        lowmemory_controls.polarity_submenu->add(new ParameterConnectionPolarityTypeSelectorControl("Slot 2", &lowmemory_controls.parameter, 1));
        lowmemory_controls.polarity_submenu->add(new ParameterConnectionPolarityTypeSelectorControl("Slot 3", &lowmemory_controls.parameter, 2));
        lowmemory_controls.polarity_submenu->add(new MenuItem(""));
    }
    menu->add(lowmemory_controls.polarity_submenu);

    //// todo: add range controls...    
    // so, kinda need to adapt/subclass ParameterValueMenuItem to be able to use a pointer-to-pointer-to-data
}