#pragma once

#if defined(USE_UCLOCK) 
    #if defined(CORE_TEENSY)
        #include <util/atomic.h>
        #define USE_ATOMIC
    #elif defined(ARDUINO_ARCH_RP2040)
        #include "uClock.h"
        #include "SimplyAtomic.h"
        #define ATOMIC_BLOCK(X) ATOMIC()
        #define USE_ATOMIC      // Microlidian actually seems to work fine even without enabling this
    #endif    
#endif

#ifndef FASTRUN
    #define FASTRUN 
#endif

//#include "Config.h"

//#define ENABLE_DEBUG_SERIAL 

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
  #include "menuitems_lambda.h"
  #include "menuitems_action.h"
#endif

int freeRam();

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
        // https://rgbcolorpicker.com/565/table
        uint16_t parameter_input_colours[15] = {
            RED,
            YELLOW,
            0x867d, // skyblue // cornflourblue BLUE,
            ORANGE,
            0x7fe0, // chartreuse
            0xfbea, // coral
            0x8dd1, // darkseagreen
            0xfb56, // hotpink
            0xdd1b, // plum
            0xd5fa, // thistle
            0xfea0, // gold
            0xef31, // khaki
            0xdfff, // lightcyan
            0xbaba, // mediumorchid
            0x07ef, // springgreen
            /*PURPLE,
            32 + ((YELLOW + BLUE) / 3),
            (ORANGE + BLUE) / 2,
            (GREEN + RED) / 2,
            (GREEN + ORANGE) / 2*/
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
        //FLASHMEM FloatParameter *addParameter(ICalibratable *parameter);
        FLASHMEM void addParameters(LinkedList<FloatParameter*> *parameters);
        
        // initialise devices and add all their voltage sources
        //FLASHMEM  // making this FLASHMEM seems to cause crashes?!
        void auto_init_devices() {
            //char parameter_input_name = 'A';
            for (uint_least8_t i = 0 ; i < devices->size() ; i++) {
                ADCDeviceBase *device = this->devices->get(i);
                Debug_printf("ParameterManager#auto_init_devices calling init() on device %i of %i\n", i+1, devices->size());
                device->init();

                VoltageSourceBase *vs = device->make_voltage_source();
                uint_least8_t counter = 0;
                while (vs != nullptr) {
                    Debug_printf("\tParameterManager#auto_init_devices adding voltage source count %i\n", counter);
                    this->addVoltageSource(vs);
                    //this->addInput(device->make_input_for_source(parameter_input_name++, vs));
                    Debug_printf("\tParameterManager#auto_init_devices calling make_voltage_source on count %i\n", counter);
                    vs = device->make_voltage_source();
                    counter++;
                }
            }

            Debug_println("Finished auto_init_devices.");
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
        //FASTRUN 
        void update_voltage_sources() {
            //if (this->debug) Serial.printf(F("ParameterManager#update_voltage_sources()"));
            // round robin reading so they get a chance to settle in between adc reads?
            const uint_fast8_t size = voltage_sources->size();
            if (size==0) return;
            static uint_fast8_t last_read = 0;

            //if (this->debug) Serial.printf(F("ParameterManager#update_voltage_sources() about to read from %i\n"), last_read);
            if (this->debug) voltage_sources->get(last_read)->debug = true;

            voltage_sources->get(last_read)->update();

            //if (this->debug) Serial.printf(F("ParameterManager#update_voltage_sources() just did read from %i\n"), last_read);
            //#ifdef ENABLE_PRINTF
                if (this->debug && Serial) {
                    //Serial.printf("Reading from ADC %i...", last_read);
                    Serial.printf("update_voltage_sources read value %f\n", voltage_sources->get(last_read)->current_value);
                }
            //#endif

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
            // process any waiting calibration
            // TODO: see if we can move this back here again
            //this->process_calibration();

            const uint_fast8_t available_inputs_size = available_inputs->size();
            for (uint_fast8_t i = 0 ; i < available_inputs_size ; i++) {
                //Serial.printf("ParameterManager#update_inputs updating input [%i/%i].. ", i+1, available_inputs_size); Serial_flush();
                available_inputs->get(i)->loop();
                //Serial.println("looped()!"); Serial_flush();
            }
        }

        unsigned long profile_update_mixers = 0;
        // update every parameter's values based on the mixed ParameterInputs
        FASTRUN void update_mixers() {
            // this is going to be pretty intensive; may need to adjust the way this works...
            //unsigned long update_mixers_started = micros();
            //ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                const uint_fast16_t available_parameters_size = this->available_parameters->size();
                if (debug && Serial) Serial.printf("update_mixers has %i parameters to process\n", available_parameters_size);
                for (uint_fast16_t i = 0 ; i < available_parameters_size ; i++) {
                    if (this->available_parameters->get(i)!=nullptr)
                        this->available_parameters->get(i)->update_mixer();
                }
            //}
            //unsigned long update_mixers_finished = micros();
            //this->profile_update_mixers = update_mixers_finished - update_mixers_started;
        }

        // update X mixers at a time
        FASTRUN void update_mixers_sliced(int_fast8_t proportion = 3) {
            static uint_fast16_t pos = 0;
            const uint_fast16_t SLICE_SIZE = this->available_parameters->size() / proportion;
            const uint_fast16_t available_parameters_size = this->available_parameters->size();
            for (uint_fast16_t i = pos ; i < available_parameters_size && i < SLICE_SIZE ; i++) {
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
            //#include "menuitems_quickpage.h"
            
            FLASHMEM void addAllParameterInputMenuItems(Menu *menu, bool page_per_input = false);
            FLASHMEM void addAllParameterInputOverviews(Menu *menu);

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
                        menu->add(new SeparatorMenuItem(item->label, default_fg_colour));
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
                SubMenuItem *submenu = new SubMenuItem(label);
                for (unsigned int i = 0 ; i < parameters->size() ; i++) {
                    //char tmp[MENU_C_MAX];
                    //sprintf(tmp, "test item %i", i);
                    //submenu->add(new MenuItem(tmp));
                    if (parameters->get(i)->is_modulatable()) {
                        Debug_printf("getModulatableParameterSubMenuItems(menu, '%s') processing parameter %i\n", label, i);
                        submenu->add(new SeparatorMenuItem(parameters->get(i)->label));
                        submenu->add(this->makeMenuItemsForParameter(parameters->get(i)));
                    }
                }
                return submenu;
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
                void addAllVoltageSourceCalibrationMenuItems(Menu *menu, bool make_own_page = false) {
                    //Serial.printf(F("------------\nParameterManager#addAllVoltageSourceCalibrationMenuItems() has %i VoltageSources to add items for?\n"), this->voltage_sources->size());
                    const unsigned int size = this->voltage_sources->size();

                    if (!make_own_page) {
                        SubMenuItem *submenuitem = new SubMenuItem("Voltage Source Calibration");
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
                    } else {
                        if(size>0){
                            menu->add_page("Input Calibration");
                            for (unsigned int i = 0 ; i < size ; i++) {
                                //Serial.printf(F("\tParameterManager#addAllVoltageSourceCalibrationMenuItems() for voltage_source %i/%i\n"), i+1, size); Serial_flush();
                                
                                VoltageSourceBase *voltage_source = this->voltage_sources->get(i);
                                menu->add(voltage_source->makeCalibrationControls(i));
                                //Serial.println("addAllVoltageSourceCalibrationMenuItems did makeCalibrationControls!");
                                #if defined(ENABLE_CALIBRATION_STORAGE)
                                    menu->add(voltage_source->makeCalibrationLoadSaveControls(i));
                                #endif

                                //submenuitem->add(new MenuItem("Test"));

                                //Serial.println(F("\t\taddAllVoltageSourceCalibrationMenuItems done!")); Serial_flush();
                                //Serial.printf(F("\tfinished with voltage_source %i\n"), i);
                            }

                            // add a control that outputs the input calibration data to the serial port
                            menu->add(new LambdaActionItem("Output Calibration Data", [=]() -> void {
                                Serial.println("Outputting calibration data...");
                                for (unsigned int i = 0 ; i < size ; i++) {
                                    VoltageSourceBase *voltage_source = this->voltage_sources->get(i);
                                    if (voltage_source->needs_calibration()) {
                                        Serial.printf("Voltage Source %i: %i\n", i, voltage_source->global_slot);
                                        voltage_source->output_calibration_data();
                                    }
                                }
                            }));
                        }
                    }
                }

                #ifdef ENABLE_CV_OUTPUT
                    void addAllCVOutputCalibrationMenuItems(Menu *menu) {
                        bool page_created = false;
                        Serial.printf("------------\nParameterManager#addAllCVOutputCalibrationMenuItems() has %i parameters to potentially add  calibrations for\n", this->available_parameters->size());
                        const unsigned int size = this->available_parameters->size();
                        for (unsigned int i = 0 ; i < size ; i++) {
                            Serial.printf("\tParameterManager#addAllCVOutputCalibrationMenuItems() for parameter %i/%i\n", i+1, size); Serial_flush();
                            Serial.printf("Parameter name is '%s'\n", this->available_parameters->get(i)->label);
                            
                            FloatParameter *parameter = this->available_parameters->get(i);
                            LinkedList<MenuItem *> *calibration_controls = parameter->makeCalibrationControls();
                            if (calibration_controls!=nullptr) {
                                Serial.println("Got some controls..."); Serial_flush();
                                if (!page_created) {
                                    menu->add_page("Output Calibration");
                                    page_created = true;                                    
                                }
                                menu->add(new SeparatorMenuItem(parameter->label));
                                Serial.println("Adding calibration controls..."); Serial_flush();
                                menu->add(calibration_controls);
                                Serial.println("Added calibration controls!"); Serial_flush();
                                #if defined(ENABLE_CALIBRATION_STORAGE)
                                    menu->add(parameter->makeCalibrationLoadSaveControls());
                                #endif
                            } else {
                                Serial.printf("Nothing to add for parameter %i\n", i);
                            }

                            Serial.println("\t\taddAllCVOutputCalibrationMenuItems done!"); Serial_flush();
                            Serial.printf("\tfinished with parameter %i\n", i);

                            Serial.printf("free ram is %u\n", freeRam());
                            #ifdef ARDUINO_TEENSY41
                                Serial.printf("free ext ram is %u\n", freeExtRam());
                            #endif
                        }
                        
                        // add a control that outputs the calibration data to the serial port
                        menu->add(new LambdaActionItem("Output Calibration Data", [=]() -> void {
                            Serial.println("Outputting input calibration data...");
                            for (unsigned int i = 0 ; i < size ; i++) {
                                FloatParameter *parameter = this->available_parameters->get(i);
                                if (parameter==nullptr) {
                                    Serial.printf("WARNING: parameter %i is null!\n", i);
                                    continue;
                                }
                                if (parameter->needs_calibration()) {
                                    Serial.printf("Parameter %i: %s\n", i, parameter->label); Serial_flush();
                                    parameter->output_calibration_data();
                                }
                            }
                            Serial.println("Outputting input calibration data done!");
                        }));
                    }
                #endif
        #endif

        FASTRUN int find_slot_for_voltage(VoltageSourceBase *source) {
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
                    if (available_inputs->get(i)->load_parse_key_value(key, value)) {
                        if (debug) Serial.printf("ParameterManager#fast_load_parse_key_value(%s, %s)\tfound a match in parameter_input: %s!\n", key.c_str(), value.c_str(), available_inputs->get(i)->name);
                        return true;
                    }
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
                if (parameter!=nullptr && parameter->load_parse_key_value(key, value)) {
                    if (debug) Serial.printf("ParameterManager#fast_load_parse_key_value(%s, %s)\tfound a match in parameter: %s!\n", key.c_str(), value.c_str(), parameter->label);
                    last_found_at = actual_index;
                    return true;
                }
            }
            //Serial.printf("WARNING: didn't find anything to process '%s' => '%s'\n", key.c_str(), value.c_str());
            return false;
        }

        // add all the lines from all parameters (or passed-in list_to_save) to the passed-in lines array
        LinkedList<String> *add_all_save_lines(LinkedList<String> *lines, LinkedList<FloatParameter*> *list_to_save = nullptr) {
            // use list of all parameters if no other list passed
            if (list_to_save==nullptr) 
                list_to_save = this->available_parameters;

            // do the available_parameter_inputs first // TODO: maybe we don't want to always save the ParameterInput settings here, if for example we're saving a passed-in list of Parameters?
            const uint_fast16_t size = available_inputs->size();
            for (uint_fast16_t i = 0 ; i < size ; i++) {
                //Serial.printf("save_pattern_add_lines() on input %i/%i, \tfreeram is %u\n", i+1, size, freeRam());
                available_inputs->get(i)->save_pattern_add_lines(lines);
            }

            // then to the parameters
            const uint_fast16_t actual_size = list_to_save->size();
            for (uint_fast16_t i = 0 ; i < actual_size ; i++) {
                //Serial.printf("save_pattern_add_lines() on parameter %i/%i, \tfreeram is %u\n", i+1, size, freeRam());
                list_to_save->get(i)->save_pattern_add_lines(lines);
            }

            return lines;
        }

        //virtual bool load_voltage_calibration();
        //virtual bool save_voltage_calibration();

        /* functions for handling updates of cv data */

        volatile unsigned long time_of_last_param_update = 0;

        FASTRUN bool ready_for_next_update(unsigned int time_between_cv_input_updates = 5) {
            return millis() - time_of_last_param_update > time_between_cv_input_updates;
        }

        // update inputs WITHOUT also updating mixers
        FASTRUN void throttled_update_cv_inputs(int time_between_cv_input_updates = 5) {
            #ifdef USE_ATOMIC
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
            #endif
            {
                if (ready_for_next_update(time_between_cv_input_updates)) {
                    this->update_voltage_sources();
                    this->update_inputs();
                    time_of_last_param_update = millis();
                }
            }
        }

        FASTRUN void process_pending() {
            uint_fast16_t size = available_parameters->size();
            for (uint_fast16_t i = 0 ; i < size ; i++) {
                available_parameters->get(i)->process_pending();
            }
        }

        // handle slicing stages of update and throttling updates - also update mixers
        FASTRUN void throttled_update_cv_input__all(int time_between_cv_input_updates = 5, bool slice_stages = false, bool slice_mixers = false) {
            {
                if (ready_for_next_update()) {
                    if (slice_stages) {
                        static int_fast8_t current_mode = 0;
                        if (debug) { Serial.println(F("about to do parameter_manager->update_voltage_sources()..")); Serial_flush(); }
                        switch (current_mode) {
                            case 0:
                                this->update_voltage_sources();
                                current_mode++;
                                break;
                            case 1:
                                //if(debug) Serial.println("just did parameter_manager->update_voltage_sources().."); Serial_flush();
                                //if(debug) Serial.println("about to do parameter_manager->update_inputs().."); Serial_flush();
                                this->update_inputs();
                                if(debug && Serial) { Serial.println(F("just did parameter_manager->update_inputs()..")); Serial_flush(); }
                                //if(debug) Serial.println("about to do parameter_manager->update_mixers().."); Serial_flush();
                                current_mode++;
                                break;
                            case 2:
                                if (slice_mixers)
                                    this->update_mixers_sliced();
                                else
                                    this->update_mixers();
                                if(debug && Serial) { Serial.println(F("just did parameter_manager->update_mixers()..")); Serial_flush(); }
                                current_mode = 0;

                                this->process_pending();
                                break;
                        }
                    } else {
                        if(debug) { Serial.println(F("about to do parameter_manager->update_voltage_sources()..")); Serial_flush(); }
                        this->update_voltage_sources();
                        //if(debug) Serial.println("just did parameter_manager->update_voltage_sources().."); Serial_flush();
                        //if(debug) Serial.println("about to do parameter_manager->update_inputs().."); Serial_flush();
                        this->update_inputs();
                        if(debug) { Serial.println(F("just did parameter_manager->update_inputs()..")); Serial_flush(); }
                        //if(debug) Serial.println("about to do parameter_manager->update_mixers().."); Serial_flush();
                        #ifdef USE_ATOMIC
                        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
                        #endif
                        {
                        if(debug) { Serial.println("about to do parameter_manager->update_mixers().."); Serial_flush(); }

                        if (slice_mixers)
                            this->update_mixers_sliced();
                        else
                            this->update_mixers();
                        }

                        this->process_pending();
                    }
                    time_of_last_param_update = millis();
                }
            }
        }

        enum VISUALISE_PARAM_MODE {
            NONE = 0,
            GRAPH_PARAMETER = 1,
            GRAPH_INPUT = 2,
            VALUE_PARAMETER = 4,    // too many to be useful, avoid
            VALUE_INPUT = 8,
        };
        // todo: allow to enable only parameters or only inputs
        FASTRUN void output_parameter_representation(int columns = 100, int8_t mode = GRAPH_PARAMETER | VALUE_INPUT) {
            // don't bother if serial isn't connected
            if (!Serial)
                return;

            static float *last_input_values = new float[this->available_inputs->size()];

            char line[columns+1];
            memset(line, ' ', columns);
            line[0] = '|';
            line[columns-1] = '|';
            line[columns] = '\0';

            if (mode & GRAPH_PARAMETER) {
                for (uint_fast16_t i = 0 ; i < this->available_parameters->size() ; i++) {
                    FloatParameter *p = this->available_parameters->get(i);
                    if (strcmp(p->label,"None")==0)
                        continue;
                    float v = p->getLastModulatedNormalValue();
                    int pos = constrain((float)columns * v, 0, columns-1);
                    line[pos] = '0' +  (i % '9');
                }
            }
            if (mode & GRAPH_INPUT) {
                // TODO: re-enable this
                for (uint_fast8_t i = 0 ; i < this->available_inputs->size() ; i++) {
                    BaseParameterInput *p = this->available_inputs->get(i);
                    //if (strcmp(p->label,"None")==0)
                    //    continue;
                    float v = p->get_normal_value_unipolar();
                    int pos = constrain((float)(columns * v), 0, columns-1);
                    line[pos] = 'A' + i;
                }
            }

            Serial.printf(line);
            
            // output a key to the parameters and inputs
            if (mode & VALUE_PARAMETER) {
                for (uint_fast16_t i = 0 ; i < this->available_parameters->size() ; i++) {
                    FloatParameter *p = this->available_parameters->get(i);
                    if (strcmp(p->label,"None")==0)
                        continue;
                    Serial.printf("%c=%s=%1.3f\t", '0'+i, p->label, p->getLastModulatedNormalValue());                
                }
            }
            if (mode & VALUE_INPUT) {
                for (uint_fast8_t i = 0 ; i < this->available_inputs->size() ; i++) {
                    BaseParameterInput *p = this->available_inputs->get(i);
                    //if (strcmp(p->label,"None")==0)
                    //    continue;
                    float v = p->get_normal_value_unipolar();
                    
                    Serial.printf("\t%c=%s=%1.3f %c", 'A'+i, p->name, v, 
                        v<last_input_values[i] ? 'v' : v>last_input_values[i] ? '^' : ' ');
                    last_input_values[i] = v;
                }
            }
            Serial.println();
        }


        ICalibratable *parameter_to_calibrate = nullptr;
        // tell ParameterManager to calibrate this output the next chance it gets
        void queue_calibration(ICalibratable *parameter_to_calibrate) {
            this->parameter_to_calibrate = parameter_to_calibrate;
        };
        bool pending_calibration() {
            return this->parameter_to_calibrate!=nullptr;
        }
        void process_calibration();

        // todo: this is currently only used by the CVOutputParameter; so perhaps we should come up with a way to make this more specific to that class?
        // called at end of setup() to load all calibrations
        // todo: this should probably also be where the VoltageSource calibrations are loaded too
        void load_all_calibrations() {
            for (uint_fast8_t i = 0 ; i < available_parameters->size() ; i++) {
                available_parameters->get(i)->load_calibration();
            }
        }


};

extern ParameterManager *parameter_manager;

