#ifdef ENABLE_SCREEN

#include "parameters/Parameter.h"
#include "mymenu_items/ParameterMenuItems_lowmemory.h"

#include "mymenu_items/ParameterMenuItems_Range.h"

lowmemory_controls_t lowmemory_controls;


void create_low_memory_parameter_controls_actual() {
    ////// amount controls, to set percentage amounts 
    //ParameterMenuItem *parameter_amount_controls;
    if (lowmemory_controls.parameter_amount_controls==nullptr)
        lowmemory_controls.parameter_amount_controls = new ParameterMenuItem("Amounts", &lowmemory_controls.parameter, false);
    menu->add(lowmemory_controls.parameter_amount_controls);

    // controls to choose which ParameterInputs to use for each slot
    // then set up a generic submenuitembar to hold the input selectors
    if (lowmemory_controls.input_selectors_bar==nullptr) {
        lowmemory_controls.input_selectors_bar = new SubMenuItemBar("Inputs", false, false);

        // some spacers so that the input controls align with the corresponding amount controls
        MenuItem *spacer1 = new MenuItem("Inputs", false);
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
        MenuItem *spacer2 = new MenuItem("", false);
        lowmemory_controls.input_selectors_bar->add(spacer2);
    }
    menu->add(lowmemory_controls.input_selectors_bar);

    ////// polarity controls
    if (lowmemory_controls.polarity_submenu==nullptr) {
        lowmemory_controls.polarity_submenu = new SubMenuItemBar("Polarities", false);
        lowmemory_controls.polarity_submenu->show_header = false;
        lowmemory_controls.polarity_submenu->add(new MenuItem("Polarity", false));
        lowmemory_controls.polarity_submenu->add(new ParameterConnectionPolarityTypeSelectorControl("Slot 1", &lowmemory_controls.parameter, 0));
        lowmemory_controls.polarity_submenu->add(new ParameterConnectionPolarityTypeSelectorControl("Slot 2", &lowmemory_controls.parameter, 1));
        lowmemory_controls.polarity_submenu->add(new ParameterConnectionPolarityTypeSelectorControl("Slot 3", &lowmemory_controls.parameter, 2));
        lowmemory_controls.polarity_submenu->add(new MenuItem("")); // so that we use 5 columns, in order to align with the other rows of elements // todo: make this indicate something eg graph values?
    }
    menu->add(lowmemory_controls.polarity_submenu);

    // range limit controls
    if (lowmemory_controls.range_submenu==nullptr) {
        lowmemory_controls.range_submenu = new SubMenuItemBar("Range", true, false);

        MenuItem *label = new MenuItem("Range", false);
        lowmemory_controls.range_submenu->add(label);

        ParameterRangeMenuItem *minimum_value_control = new ParameterRangeMenuItem("Minimum", &lowmemory_controls.parameter, MINIMUM);
        lowmemory_controls.range_submenu->add(minimum_value_control);

        ParameterRangeMenuItem *maximum_value_control = new ParameterRangeMenuItem("Maximum", &lowmemory_controls.parameter, MAXIMUM);
        lowmemory_controls.range_submenu->add(maximum_value_control);
    }
    menu->add(lowmemory_controls.range_submenu);

}

// add controls for several parameters in one place, with a UI to switch between them
// todo: add support for the CustomTypeControls of the parameters too..
//        ^ for this, perhaps we create and store the LinkedList<MenuItem*>* created by addCustomTypeControls
//          and then we render them in the LowMemoryEmbedMenuItem.
void create_low_memory_parameter_controls(const char *label, LinkedList<FloatParameter*> *parameters, int_fast16_t default_fg) {
    ////// control to select which parameter the other controls will edit
    ParameterMenuItemSelector *parameter_selector = new ParameterMenuItemSelector(label, parameters);
    //if (lowmemory_controls.parameter_selector==nullptr)
        //lowmemory_controls.parameter_selector = new ParameterMenuItemSelector(label, parameters);
    menu->add(new LowMemorySwitcherMenuItem((char*)"switcher", parameters, parameter_selector, default_fg));
    lowmemory_controls.parameter = parameter_selector->parameter;
    menu->add(parameter_selector);

    create_low_memory_parameter_controls_actual();
}

// add controls for a single parameter
void create_low_memory_parameter_controls(const char *label, FloatParameter *parameter, int_fast16_t default_fg) {
    //Serial.printf("before create_low_memory_parameter_controls(%s), RAM usage is %u\n", label, freeRam()); Serial.flush();

    // add the custom controls for this parameter first
    LinkedList<MenuItem*> *custom_controls = new LinkedList<MenuItem*>();
    parameter->addCustomTypeControls(custom_controls);
    //Serial.printf("after addCustomTypeControls for %s, RAM usage is %u\n", label, freeRam()); Serial.flush();
    menu->add(custom_controls);
    //Serial.printf("after menu->add(custom_controls) for %s, RAM usage is %u\n", label, freeRam()); Serial.flush();
    //if (custom_controls->size()==0)   // crashes if we do this
    //    delete custom_controls;

    // then add the low memory version of the controls
    menu->add(new LowMemoryEmbedMenuItem((char*)"single", parameter, default_fg));
    lowmemory_controls.parameter = parameter;

    create_low_memory_parameter_controls_actual();
}

#endif