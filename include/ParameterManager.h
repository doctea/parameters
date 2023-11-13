#ifndef PARAMETERMANAGER__INCLUDED
#define PARAMETERMANAGER__INCLUDED

#if defined(USE_UCLOCK) && defined(CORE_TEENSY)
  #include <util/atomic.h>
  #define USE_ATOMIC
#endif

//#include "Config.h"

#include "debug.h"

#include "voltage_sources/VoltageSource.h"
#include "parameters/Parameter.h"
#include "parameter_inputs/ParameterInput.h"
#include "devices/ADCDevice.h"

//#include "mymenu_items/ParameterInputViewMenuItems.h"

#include <LinkedList.h>

#ifdef ENABLE_SCREEN
  #include "submenuitem_bar.h"
  #include "mymenu_items/ParameterInputViewMenuItems.h"
  #include "colours.h"
#endif

class ParameterManager {
    public:
        bool debug = false;

        unsigned long memory_size;

        LinkedList<ADCDeviceBase*>      *devices = new LinkedList<ADCDeviceBase*>();  // actual i2c ADC devices, potentially with multiple channels
        LinkedList<VoltageSourceBase*>  *voltage_sources = new LinkedList<VoltageSourceBase*>();  // voltage-measuring channels
        LinkedList<BaseParameterInput*> *available_inputs = new LinkedList<BaseParameterInput*>();  // ParameterInputs, ie wrappers around input mechanism, assignable to a Parameter
        LinkedList<FloatParameter*>     *available_parameters = new LinkedList<FloatParameter*>();        // Parameters, ie wrappers around destination object
        FloatParameter *param_none = nullptr;        // 'blank' parameter used as default mapping

        #ifdef ENABLE_SCREEN
        uint16_t parameter_input_colours[9] = {
            RED,
            YELLOW,
            BLUE,
            PURPLE,
            ORANGE,
            32 + ((YELLOW + BLUE) / 3),
            (ORANGE + BLUE) / 2,
            (GREEN + RED) / 2,
            (GREEN + ORANGE) / 2
        };
        #endif

        ParameterManager (unsigned long memory_size) {
            this->memory_size = memory_size;
        }
        ~ParameterManager () {}

        FLASHMEM void init() {
            /*this->devices = new LinkedList<ADCDeviceBase*>();
            this->voltage_sources = new LinkedList<VoltageSourceBase*>();
            this->available_inputs = new LinkedList<BaseParameterInput*>();
            this->available_parameters = new LinkedList<FloatParameter*>();*/
            this->param_none = this->addParameter(new FloatParameter((char*)"None"));
        }

        LinkedList<BaseParameterInput*> *get_available_pitch_inputs();
        FLASHMEM ADCDeviceBase *addADCDevice(ADCDeviceBase *device);
        FLASHMEM VoltageSourceBase *addVoltageSource(VoltageSourceBase *voltage_source);
        //FLASHMEM 
        BaseParameterInput *addInput(BaseParameterInput *input);
        FLASHMEM FloatParameter *addParameter(FloatParameter *parameter);
        FLASHMEM void addParameters(LinkedList<FloatParameter*> *parameters);
        
        // initialise devices and add all their voltage sources
        FLASHMEM void auto_init_devices() {
            Debug_printf(F("ParameterManager#auto_init_devices)\n"));
            //char parameter_input_name = 'A';
            for (unsigned int i = 0 ; i < devices->size() ; i++) {
                ADCDeviceBase *device = this->devices->get(i);
                Debug_printf(F("ParameterManager#auto_init_devices calling init() on device %i of %i\n"), i+1, devices->size());
                device->init();

                VoltageSourceBase *vs = device->make_voltage_source();
                int counter = 0;
                while (vs != nullptr) {
                    Debug_printf(F("\tParameterManager#auto_init_devices adding voltage source count %i\n"), counter);
                    this->addVoltageSource(vs);
                    //this->addInput(device->make_input_for_source(parameter_input_name++, vs));
                    Debug_printf(F("\tParameterManager#auto_init_devices calling make_voltage_source on count %i\n"), counter);
                    vs = device->make_voltage_source();
                    counter++;
                }
            }
        }

        FASTRUN BaseParameterInput *getInputForName(const char *input_name) {
            // todo: could perhaps mildly optimise this so that search starts at the last found entry, for very mild speedup when loading from save files?
            const uint_fast8_t size = available_inputs->size();
            for(uint_fast8_t i = 0 ; i < size ; i++) {
                if (available_inputs->get(i)->matches_label(input_name))
                    return available_inputs->get(i);
            }
            return nullptr;
            /*int index = this->getInputIndexForName(input_name);
            if (index!=-1) return this->available_inputs->get(index);
            return nullptr;*/
        }
        FASTRUN int getInputIndexForName(const char *input_name) {
            const uint_fast8_t size = available_inputs->size();
            for(uint_fast8_t i = 0 ; i < size ; i++) {
                if (available_inputs->get(i)->matches_label(input_name))
                    return i;
            }
            return -1;
        }
        FASTRUN int getInputIndex(BaseParameterInput *param) {
            if (param==nullptr) return -1;
            const uint_fast8_t size = available_inputs->size();
            for (uint_fast8_t i = 0 ; i < size ; i++) {
                if (param==this->available_inputs->get(i))
                    return i;
            }
            return -1;
            //return this->getInputIndexForName(param->name);
        }
        /*FASTRUN int getPitchInputIndex(BaseParameterInput *param) {
            if (param==nullptr) return -1;
            LinkedList<BaseParameterInput*> *pitch_inputs = this->get_available_pitch_inputs();
            const unsigned int size = pitch_inputs->size();
            for (unsigned int i = 0 ; i < size ; i++) {
                if (param==pitch_inputs->get(i))
                    return i;
            }
            return -1;
            //return this->getInputIndexForName(param->name);
        }*/

        // read the values, but don't pass them on outside
        FASTRUN void update_voltage_sources() {
            //if (this->debug) Serial.printf(F("ParameterManager#update_voltage_sources()"));
            // round robin reading so they get a chance to settle in between adc reads?
            const uint_fast8_t size = voltage_sources->size();
            if (size==0) return;
            static uint_fast8_t last_read = 0;

            //if (this->debug) Serial.printf(F("ParameterManager#update_voltage_sources() about to read from %i\n"), last_read);
            if (this->debug) voltage_sources->get(last_read)->debug = true;

            voltage_sources->get(last_read)->update();

            //if (this->debug) Serial.printf(F("ParameterManager#update_voltage_sources() just did read from %i\n"), last_read);
            /*#ifdef ENABLE_PRINTF
                if (this->debug) {
                    //Serial.printf("Reading from ADC %i...", last_read);
                    Serial.printf(F("update_voltage_sources read value %f\n"), voltage_sources->get(last_read)->current_value);
                }
            #endif*/

            last_read++;
            if (last_read>=size)
                last_read = 0;

            voltage_sources->get(last_read)->discard_update();   // pre-read the next one so it has a chance to settle?

            /*  // simple reading of all of them    
            for (unsigned int i = 0 ; i < voltage_sources->size() ; i++) {
                voltage_sources->get(i)->update();
            }*/
        }

        // update the ParameterInputs with the latest values from the VoltageSources
        FASTRUN void update_inputs() {
            const uint_fast8_t available_inputs_size = available_inputs->size();
            for (uint_fast8_t i = 0 ; i < available_inputs_size ; i++) {
                available_inputs->get(i)->loop();
            }
        }

        unsigned long profile_update_mixers = 0;
        // update every parameter's values based on the mixed ParameterInputs
        FASTRUN void update_mixers() {
            // this is going to be pretty intensive; may need to adjust the way this works...
            unsigned long update_mixers_started = micros();
            const uint_fast8_t available_parameters_size = this->available_parameters->size();
            for (uint_fast8_t i = 0 ; i < available_parameters_size ; i++) {
                this->available_parameters->get(i)->update_mixer();
            }
            unsigned long update_mixers_finished = micros();
            this->profile_update_mixers = update_mixers_finished - update_mixers_started;
        }

        // update X mixers at a time
        FASTRUN void update_mixers_sliced(int_fast8_t proportion = 3) {
            static uint_fast8_t pos = 0;
            const uint_fast8_t SLICE_SIZE = this->available_parameters->size() / proportion;
            const uint_fast8_t available_parameters_size = this->available_parameters->size();
            for (uint_fast8_t i = pos ; i < available_parameters_size && i < SLICE_SIZE ; i++) {
                this->available_parameters->get(i)->update_mixer();
            }
            if (pos>=available_parameters_size) pos = 0;
        }

        FLASHMEM // causes a section type conflict with 'void setup_cv_input()'
        void setDefaultParameterConnections() {
            //Debug_printf(F("ParameterManager#setDefaultParameterConnections() has %i parameters to map to %i inputs..\n"), this->available_parameters->size(), this->available_inputs->size());
            for (uint_fast8_t i = 0 ; i < this->available_parameters->size() ; i++) {
                // todo: make this configurable dynamically / load defaults from save file
                available_parameters->get(i)->set_slot_0_input(available_inputs->get(0));
                available_parameters->get(i)->set_slot_1_input(available_inputs->get(1));
                available_parameters->get(i)->set_slot_2_input(available_inputs->get(2));
                /*for (unsigned int i = 0 ; i < MAX_SLOT_CONNECTIONS ; i++) {
                    available_parameters->get(i)->connections[i].parameter_input = available_inputs->get(i);
                }*/
            }
        }

        #ifdef ENABLE_SCREEN
            FLASHMEM void addAllParameterInputMenuItems(Menu *menu, bool page_per_input = false) {
                const char *last_group_name = nullptr;
                for (unsigned int i = 0 ; i < available_inputs->size() ; i++) {
                    BaseParameterInput *parameter_input = available_inputs->get(i);
                    //Serial.printf("!!! Adding parameter menu items for %s\tfrom %s!\n", parameter_input->name, parameter_input->group_name);
                    char label[MENU_C_MAX];
                    if (last_group_name!=parameter_input->group_name || page_per_input) {                        
                        if (page_per_input)
                            snprintf(label, MENU_C_MAX, "%s: %s", parameter_input->group_name, parameter_input->name);
                        else
                            snprintf(label, MENU_C_MAX, "%s inputs", parameter_input->group_name);
                        menu->add_page(label);
                        last_group_name = parameter_input->group_name;
                        snprintf(label, MENU_C_MAX, "%s ", last_group_name);
                    }
                    this->addParameterInputMenuItems(menu, parameter_input, label); //label_prefix);
                }
            }

            // add all the available parameters to the main menu
            FLASHMEM void addAllParameterMenuItems(Menu *menu) {
                for (unsigned int i = 0 ; i < this->available_parameters->size() ; i++) {
                    //LinkedList<MenuItem*> *ctrls = this->makeMenuItemsForParameter(this->available_parameters->get(i));
                    //menu->add(ctrls);
                    menu->add(this->makeMenuItemsForParameter(this->available_parameters->get(i)));
                    //Serial.printf("%2i: After adding parameter menu items for (%s),\tfree RAM is \t%u\n", i, this->available_parameters->get(i)->label, rp2040.getFreeHeap());
                    //ctrls->clear();
                    //delete ctrls;
                    //continue;
                    //delete ctrls; 
                    //Serial.printf("about to ctrls->clear()\n"); Serial.flush();
                    //ctrls->clear();                    
                }
                //Serial.println("addAllParameterMenuItems finished"); Serial.flush();
                //ctrls->clear();
                //delete ctrls;
            }

            FLASHMEM SubMenuItem *addParameterSubMenuItems(Menu *menu, const char *submenu_label, LinkedList<FloatParameter*> *parameters, int16_t default_fg_colour = C_WHITE) {
                // first add all the modulatable items into a sub menu
                SubMenuItem *sub = getModulatableParameterSubMenuItems(menu, submenu_label, parameters);
                if (sub!=nullptr && sub->items->size()>0) {
                    menu->add(sub);
                    sub->set_default_colours(default_fg_colour);
                }

                // then add all the non-modulatable ones as separate rows
                for (unsigned int i = 0 ; i < parameters->size() ; i++) {
                    if (!parameters->get(i)->is_modulatable()) {
                        Debug_printf("addParameterSubMenuItems() adding non-modulatable item %i aka '%s'\n", i, parameters->get(i)->label);
                        MenuItem *item = this->makeMenuItemForParameter(parameters->get(i));
                        item->set_default_colours(default_fg_colour);
                        menu->add(item);
                    }
                }

                return sub; // we actually add them in this function above, so we can do other stuff
            }

            // add only the modulatable items to a sub-menu
            FLASHMEM SubMenuItem *getModulatableParameterSubMenuItems(Menu *menu, const char *submenu_label, LinkedList<FloatParameter*> *parameters) {
                char label[MAX_LABEL_LENGTH];
                snprintf(label, MAX_LABEL_LENGTH, "Parameters for %s", submenu_label);
                //LinkedList<DataParameter*> *parameters = behaviour_craftsynth->get_parameters();
                SubMenuItem *submenu = new SubMenuItem(label, false);
                for (unsigned int i = 0 ; i < parameters->size() ; i++) {
                    //char tmp[MENU_C_MAX];
                    //sprintf(tmp, "test item %i", i);
                    //submenu->add(new MenuItem(tmp));
                    if (parameters->get(i)->is_modulatable()) {
                        Debug_printf("getModulatableParameterSubMenuItems(menu, '%s') processing parameter %i\n", label, i);
                        submenu->add(this->makeMenuItemsForParameter(parameters->get(i)));
                    }
                }
                return submenu;
            }

            FLASHMEM MenuItem *addParameterInputMenuItems(Menu *menu, BaseParameterInput *param_input, const char *label_prefix = "") {
                // TODO: a new ParameterInputControl that allows to set expected input ranges
                char label[MENU_C_MAX];
                snprintf(label, MENU_C_MAX, "%s%s", label_prefix, param_input->name);
                //char *label = param_input->name;

                menu->add(new SeparatorMenuItem(label, param_input->colour));

                //Debug_printf(F("\tdoing menu->add for ParameterInputDisplay with label '%s'\n"), label);
                ParameterInputDisplay *parameter_input_display = new ParameterInputDisplay(label, this->memory_size, param_input);
                #ifdef PARAMETER_INPUTS_USE_CALLBACKS
                    param_input->add_parameter_input_callback_receiver(parameter_input_display);
                #endif
                menu->add(parameter_input_display);

                if (param_input->supports_bipolar()) {
                    //SubMenuItem *submenu = new SubMenuItem("Input/Output");
                    DualMenuItem *submenu = new DualMenuItem("Input/Output");
                    submenu->set_default_colours(param_input->colour);
                    submenu->show_header = false;

                    //sprintf(label, "Input type for %s", param_input->name);
                    submenu->add(new InputTypeSelectorControl("Input"/*label*/, &param_input->input_type)); // Input type

                    //sprintf(label, "Out type for %s", param_input->name);
                    submenu->add(new InputTypeSelectorControl("Output"/*label*/, &param_input->output_type));   // Output type

                    menu->add(submenu);
                    return submenu; // was nullptr
                } 
                return nullptr;
            }

            // create a menuitem for the passed-in parameter; returns nullptr if passed-in parameter is named "None"
            FLASHMEM MenuItem *makeMenuItemForParameter(FloatParameter *p, const char *label_prefix = "") {
                if (strcmp(p->label,"None")==0) return nullptr;
                MenuItem *ctrl = p->makeControl();
                return ctrl;
            }
            FLASHMEM LinkedList<MenuItem *> *makeMenuItemsForParameter(FloatParameter *p, const char *label_prefix = "") {
                if (strcmp(p->label,"None")==0) 
                    return nullptr;
                //Debug_println(F("makeMenuItemsForParameter calling makeControls!..")); Serial_flush();
                //Serial.printf("makeMenuItemsForParameter has parameter @%p, doing makeControls\n", p); Serial.flush();
                LinkedList<MenuItem *> *ctrls = p->makeControls();
                //Serial.println("returning controls..");
                return ctrls;
            }

            /*FLASHMEM void *addAllVoltageSourceMenuItems(Menu *menu) {
                Serial.printf(F("------------\nParameterManager#addAllVoltageSourceMenuItems() has %i VoltageSources to add items for?\n"), this->voltage_sources->size());
                const unsigned int size = this->voltage_sources->size();
                for (unsigned int i = 0 ; i < size ; i++) {
                    Serial.printf(F("\tParameterManager#addAllVoltageSourceMenuItems() for voltage_source %i/%i\n"), i+1, size); Serial_flush();

                    VoltageSourceBase *voltage_source = this->voltage_sources->get(i);
                    if (voltage_source!=nullptr) {
                        menu->add(voltage_source->makeCalibrationControls(i));
                        Serial.println(F("\t\tmakeCalibrationControls done!")); Serial_flush();
                    }

                    Serial.printf(F("\tfinished with voltage_source %i\n"), i);
                }
                Serial.printf(F("ParameterManager#addAllVoltageSourceMenuItems() done!\n------------\n")); Serial_flush();
            }*/

            //#if defined(ENABLE_SD) && !defined(DISABLE_CALIBRATION_STORAGE)
                //FLASHMEM causes a section type conflict with 'void setup_cv_input()'
                void addAllVoltageSourceCalibrationMenuItems(Menu *menu) {
                    //Serial.printf(F("------------\nParameterManager#addAllVoltageSourceCalibrationMenuItems() has %i VoltageSources to add items for?\n"), this->voltage_sources->size());
                    const unsigned int size = this->voltage_sources->size();

                    SubMenuItem *submenuitem = new SubMenuItem("Voltage Source Calibration", false);
                    //submenuitem->debug = true;

                    for (unsigned int i = 0 ; i < size ; i++) {
                        //Serial.printf(F("\tParameterManager#addAllVoltageSourceCalibrationMenuItems() for voltage_source %i/%i\n"), i+1, size); Serial_flush();
                        
                        VoltageSourceBase *voltage_source = this->voltage_sources->get(i);
                        submenuitem->add(voltage_source->makeCalibrationControls(i));
                        //Serial.println("addAllVoltageSourceCalibrationMenuItems did makeCalibrationControls!");
                        #if defined(ENABLE_CALIBRATION_STORAGE)
                            submenuitem->add(voltage_source->makeCalibrationLoadSaveControls(i));
                        #endif

                        //submenuitem->add(new MenuItem("Test"));

                        //Serial.println(F("\t\taddAllVoltageSourceCalibrationMenuItems done!")); Serial_flush();
                        //Serial.printf(F("\tfinished with voltage_source %i\n"), i);
                    }
                    //Serial.printf(F("ParameterManager#addAllVoltageSourceCalibrationMenuItems() done!\n------------\n")); Serial_flush();

                    menu->add(submenuitem);
                }
            //#endif
        #endif

        int find_slot_for_voltage(VoltageSourceBase *source) {
            for (unsigned int i = 0 ; i < voltage_sources->size() ; i++) {
                if (voltage_sources->get(i) == source)
                    return i;
            }
            return -1;
        }

        // attempt optimised search + update for passed in key+value, on all parameters or on passed-in list_to_search
        bool fast_load_parse_key_value(String key, String value, LinkedList<FloatParameter*> *list_to_search = nullptr) {
            static LinkedList<FloatParameter*> *last_searched = nullptr;
            static uint_fast8_t last_found_at = 0;

            // first, do all the ParameterInputs (save their input/output type, ie bipolar/unipolar, etc)
            // TODO: should probably move this into its own function that can be used to process parameter_inputs separately from parameters
            //       will probably bring about some gains in wasted searching
            if (key.startsWith(ParameterInput::prefix)) {
                const uint_fast8_t available_inputs_count = available_inputs->size();
                for (uint_fast8_t i = 0 ; i < available_inputs_count ; i++) {
                    if (available_inputs->get(i)->load_parse_key_value(key, value))
                        return true;
                }
                // we probably want to return early here, since 'parameter_input_' hasn't been handled by any existing parameter_input, 
                // and so its probably a waste of time searching through all the parameters asking for it?
                return false;
            }

            // use list of all parameters if no other list passed
            if (list_to_search==nullptr) 
                list_to_search = this->available_parameters;
            
            // if the list we've been given is different to the last list we searched, reset the search start position
            if (last_searched != list_to_search)
                last_found_at = 0;
            last_searched = list_to_search;

            // if we previously found an item at an index, search past the "end" of the list 
            const uint_fast16_t actual_size = list_to_search->size();    // just the size of the list
            const uint_fast16_t search_up_to = last_found_at + actual_size;

            // start seach at the last found index, move through past the end of the list and wrap around again to the start
            for (uint_fast16_t i = last_found_at ; i < search_up_to ; i++) {
                uint_fast16_t actual_index = i;  
                if (actual_index>=actual_size)
                    actual_index -= actual_size;    // wrap around to the start of list to ensure we search all entries

                FloatParameter *parameter = list_to_search->get(actual_index);
                if (parameter->load_parse_key_value(key, value)) {
                    last_found_at = actual_index;
                    return true;
                }
            }
            return false;
        }

        // add all the lines from all parameters (or passed-in list_to_save) to the passed-in lines array
        LinkedList<String> *add_all_save_lines(LinkedList<String> *lines, LinkedList<FloatParameter*> *list_to_save = nullptr) {
            // use list of all parameters if no other list passed
            if (list_to_save==nullptr) 
                list_to_save = this->available_parameters;

            // do the available_parameter_inputs first // TODO: maybe we don't want to always save the ParameterInput settings here, if for example we're saving a passed-in list of Parameters?
            for (unsigned int i = 0 ; i < available_inputs->size() ; i++) {
                available_inputs->get(i)->save_sequence_add_lines(lines);
            }

            // then to the parameters
            const unsigned int actual_size = list_to_save->size();
            for (unsigned int i = 0 ; i < actual_size ; i++) {
                list_to_save->get(i)->save_sequence_add_lines(lines);
            }

            return lines;
        }

        //virtual bool load_voltage_calibration();
        //virtual bool save_voltage_calibration();

        /* functions for handling updates of cv data */

        unsigned long time_of_last_param_update = 0;

        bool ready_for_next_update(unsigned int time_between_cv_input_updates = 5) {
            return millis() - time_of_last_param_update > time_between_cv_input_updates;
        }

        // handle slicing stages of update and throttling updates
        void throttled_update_cv_input(bool slice_stages = false, int time_between_cv_input_updates = 5, bool slice_mixers = false) {
            #ifdef USE_ATOMIC
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
            #endif
            {
                if (ready_for_next_update()) {
                    if (slice_stages) {
                        static int_fast8_t current_mode = 0;
                        if(debug_flag) {
                            this->debug = true;
                            Serial.println(F("about to do parameter_manager->update_voltage_sources()..")); Serial_flush();
                        }
                        switch (current_mode) {
                            case 0:
                                this->update_voltage_sources();
                                current_mode++;
                                break;
                            case 1:
                                //if(debug) Serial.println("just did parameter_manager->update_voltage_sources().."); Serial_flush();
                                //if(debug) Serial.println("about to do parameter_manager->update_inputs().."); Serial_flush();
                                this->update_inputs();
                                //if(debug) Serial.println("about to do parameter_manager->update_mixers().."); Serial_flush();
                                current_mode++;
                                break;
                            case 2:
                                if (slice_mixers)
                                    this->update_mixers_sliced();
                                else
                                    this->update_mixers();
                                if(debug_flag) { Serial.println(F("just did parameter_manager->update_inputs()..")); Serial_flush(); }
                                current_mode = 0;
                                break;
                        }
                    } else {
                        if(debug_flag) this->debug = true;
                        if(debug_flag) { Serial.println(F("about to do parameter_manager->update_voltage_sources()..")); Serial_flush(); }
                        this->update_voltage_sources();
                        //if(debug) Serial.println("just did parameter_manager->update_voltage_sources().."); Serial_flush();
                        //if(debug) Serial.println("about to do parameter_manager->update_inputs().."); Serial_flush();
                        this->update_inputs();
                        //if(debug) Serial.println("about to do parameter_manager->update_mixers().."); Serial_flush();
                        if (slice_mixers)
                            this->update_mixers_sliced();
                        else
                            this->update_mixers();
                        if(debug_flag) { Serial.println(F("just did parameter_manager->update_inputs()..")); Serial_flush(); }
                    }
                    time_of_last_param_update = millis();
                }
            }
        }
};

extern ParameterManager *parameter_manager;

#endif
