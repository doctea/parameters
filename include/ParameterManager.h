#ifndef PARAMETERMANAGER__INCLUDED
#define PARAMETERMANAGER__INCLUDED

//#include "Config.h"

#include "debug.h"

#include "voltage_sources/VoltageSource.h"
#include "parameters/Parameter.h"
#include "parameter_inputs/ParameterInput.h"
#include "devices/ADCDevice.h"

//#include "mymenu_items/ParameterInputViewMenuItems.h"

#include <LinkedList.h>

#ifdef ENABLE_SCREEN
  #include "submenuitem.h"
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
        LinkedList<DoubleParameter*>    *available_parameters = new LinkedList<DoubleParameter*>();        // Parameters, ie wrappers around destination object
        DoubleParameter *param_none = nullptr;        // 'blank' parameter used as default mapping

        uint16_t parameter_input_colours[9] = {
            RED,
            YELLOW,
            BLUE,
            PURPLE,
            ORANGE,
            32 + ((YELLOW + BLUE) / 2),
            (ORANGE + BLUE) / 2,
            (GREEN + RED) / 2,
            (GREEN + ORANGE) / 2
        };

        ParameterManager (unsigned long memory_size) {
            this->memory_size = memory_size;
        }
        ~ParameterManager () {}

        FLASHMEM void init() {
            /*this->devices = new LinkedList<ADCDeviceBase*>();
            this->voltage_sources = new LinkedList<VoltageSourceBase*>();
            this->available_inputs = new LinkedList<BaseParameterInput*>();
            this->available_parameters = new LinkedList<DoubleParameter*>();*/
            this->param_none = this->addParameter(new DoubleParameter((char*)"None"));
        }

        LinkedList<BaseParameterInput*> *get_available_pitch_inputs() {
            static LinkedList<BaseParameterInput*> *available_pitch_inputs = new LinkedList<BaseParameterInput*>();
            static bool already_calculated = false;
            if (!already_calculated) {
                for (unsigned int i = 0 ; i < available_inputs->size() ; i++) {
                    if (available_inputs->get(i)->supports_pitch())
                        available_pitch_inputs->add(available_inputs->get(i));
                }
                already_calculated = true;
            }
            return available_pitch_inputs;
        }

        FLASHMEM ADCDeviceBase *addADCDevice(ADCDeviceBase *device) {
            Debug_printf(F("ParameterManager#addADCDevice(%p)\n"), device);
            this->devices->add(device);
            return device;
        }

        FLASHMEM VoltageSourceBase *addVoltageSource(VoltageSourceBase *voltage_source) {
            Debug_printf(F("ParameterManager#addVoltageSource(%p)\n"), voltage_source);
            #if defined(LOAD_CALIBRATION_ON_BOOT) && defined(ENABLE_SD) && defined(ENABLE_CALIBRATION_STORAGE)
                Debug_printf(F("Loading calibration for %i!\n"), voltage_source->slot);
                voltage_source->load_calibration();
            #endif

            this->voltage_sources->add(voltage_source);
            return voltage_source;
        }

        FLASHMEM BaseParameterInput *addInput(BaseParameterInput *input) {
            Debug_printf(F("ParameterManager#addInput(%p)\n"), input);
            input->colour = parameter_input_colours[this->available_inputs->size() % (sizeof(parameter_input_colours)/2)];
            this->available_inputs->add(input);
            return input;
        }

        FLASHMEM DoubleParameter *addParameter(DoubleParameter *parameter) {
            Debug_printf(F("ParameterManager#addParameter(%p), labeled '%s'\n"), parameter, parameter->label);
            this->available_parameters->add(parameter);
            return parameter;
        }
        FLASHMEM void addParameters(LinkedList<DoubleParameter*> *parameters) {
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

        // initialise devices and add all their voltage sources
        FLASHMEM void auto_init_devices() {
            Debug_printf(F("ParameterManager#auto_init_devices)\n"));
            //char parameter_input_name = 'A';
            for (unsigned int i = 0 ; i < devices->size() ; i++) {
                ADCDeviceBase *device = this->devices->get(i);
                Debug_printf(F("ParameterManager#auto_init_devices calling init() on device %i\n"), i);
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
            const unsigned int size = available_inputs->size();
            for(unsigned int i = 0 ; i < size ; i++) {
                if (available_inputs->get(i)->matches_label(input_name))
                    return available_inputs->get(i);
            }
            return nullptr;
            /*int index = this->getInputIndexForName(input_name);
            if (index!=-1) return this->available_inputs->get(index);
            return nullptr;*/
        }
        FASTRUN int getInputIndexForName(const char *input_name) {
            const unsigned int size = available_inputs->size();
            for(unsigned int i = 0 ; i < size ; i++) {
                if (available_inputs->get(i)->matches_label(input_name))
                    return i;
            }
            return -1;
        }
        FASTRUN int getInputIndex(BaseParameterInput *param) {
            if (param==nullptr) return -1;
            const unsigned int size = available_inputs->size();
            for (unsigned int i = 0 ; i < size ; i++) {
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
            const unsigned int size = voltage_sources->size();
            if (size==0) return;
            static unsigned int last_read = 0;

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
            const unsigned int available_inputs_size = available_inputs->size();
            for (unsigned int i = 0 ; i < available_inputs_size ; i++) {
                available_inputs->get(i)->loop();
            }
        }

        unsigned long profile_update_mixers = 0;
        // update every parameter's values based on the mixed ParameterInputs
        FASTRUN void update_mixers() {
            // this is going to be pretty intensive; may need to adjust the way this works...
            unsigned long update_mixers_started = micros();
            const unsigned int available_parameters_size = this->available_parameters->size();
            for (unsigned int i = 0 ; i < available_parameters_size ; i++) {
                this->available_parameters->get(i)->update_mixer();
            }
            unsigned long update_mixers_finished = micros();
            this->profile_update_mixers = update_mixers_finished - update_mixers_started;
        }

        FLASHMEM void setDefaultParameterConnections() {
            Debug_printf(F("ParameterManager#setDefaultParameterConnections() has %i parameters to map to %i inputs..\n"), this->available_parameters->size(), this->available_inputs->size());
            for (unsigned int i = 0 ; i < this->available_parameters->size() ; i++) {
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
            FLASHMEM void addAllParameterInputMenuItems(Menu *menu, const char *label_prefix = nullptr) {
                for (unsigned int i = 0 ; i < available_inputs->size() ; i++) {
                    this->addParameterInputMenuItems(menu, available_inputs->get(i), label_prefix);
                }
            }

            // add all the available parameters to the main menu
            FLASHMEM void addAllParameterMenuItems(Menu *menu) {
                for (unsigned int i = 0 ; i <this->available_parameters->size() ; i++) {
                    LinkedList<MenuItem*> *ctrls = this->makeMenuItemsForParameter(this->available_parameters->get(i));
                    menu->add(ctrls);
                    //continue;
                    //delete ctrls; 
                    //Serial.printf("about to ctrls->clear()\n"); Serial.flush();
                    //ctrls->clear();                    
                }
                //Serial.println("addAllParameterMenuItems finished"); Serial.flush();
            }

            FLASHMEM SubMenuItem *addParameterSubMenuItems(Menu *menu, const char *submenu_label, LinkedList<DoubleParameter*> *parameters, int16_t default_fg_colour = C_WHITE) {
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
            FLASHMEM SubMenuItem *getModulatableParameterSubMenuItems(Menu *menu, const char *submenu_label, LinkedList<DoubleParameter*> *parameters) {
                char label[MAX_LABEL_LENGTH];
                snprintf(label, MAX_LABEL_LENGTH, "Parameters for %s", submenu_label);
                //LinkedList<DataParameter*> *parameters = behaviour_craftsynth->get_parameters();
                SubMenuItem *submenu = new SubMenuItem(label, false);
                for (unsigned int i = 0 ; i < parameters->size() ; i++) {
                    //char tmp[MENU_C_MAX];
                    //sprintf(tmp, "test item %i", i);
                    //submenu->add(new MenuItem(tmp));
                    if (parameters->get(i)->is_modulatable()) {
                        Debug_printf(F("getModulatableParameterSubMenuItems(menu, '%s') processing parameter %i\n"), label, i);
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

                Debug_printf(F("\tdoing menu->add for ParameterInputDisplay with label '%s'\n"), label);
                menu->add(new ParameterInputDisplay(label, this->memory_size, param_input)); //, LOOP_LENGTH_TICKS));

                if (param_input->supports_bipolar()) {
                    SubMenuItem *submenu = new SubMenuItem("Input/Output");
                    submenu->set_default_colours(param_input->colour);
                    submenu->show_header = false;

                    //sprintf(label, "Input type for %s", param_input->name);
                    submenu->add(new InputTypeSelectorControl("Input type"/*label*/, &param_input->input_type));

                    //sprintf(label, "Out type for %s", param_input->name);
                    submenu->add(new InputTypeSelectorControl("Output type"/*label*/, &param_input->output_type));

                    menu->add(submenu);
                    return submenu; // was nullptr
                } 
                return nullptr;
            }

            // create a menuitem for the passed-in parameter; returns nullptr if passed-in parameter is named "None"
            FLASHMEM MenuItem *makeMenuItemForParameter(DoubleParameter *p, const char *label_prefix = "") {
                if (strcmp(p->label,"None")==0) return nullptr;
                MenuItem *ctrl = p->makeControl();
                return ctrl;
            }
            FLASHMEM LinkedList<MenuItem *> *makeMenuItemsForParameter(DoubleParameter *p, const char *label_prefix = "") {
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
                FLASHMEM void addAllVoltageSourceCalibrationMenuItems(Menu *menu) {
                    //Serial.printf(F("------------\nParameterManager#addAllVoltageSourceCalibrationMenuItems() has %i VoltageSources to add items for?\n"), this->voltage_sources->size());
                    const unsigned int size = this->voltage_sources->size();

                    SubMenuItem *submenuitem = new SubMenuItem("Voltage Source Calibration", false);

                    for (unsigned int i = 0 ; i < size ; i++) {
                        //Serial.printf(F("\tParameterManager#addAllVoltageSourceCalibrationMenuItems() for voltage_source %i/%i\n"), i+1, size); Serial_flush();
                        
                        VoltageSourceBase *voltage_source = this->voltage_sources->get(i);
                        submenuitem->add(voltage_source->makeCalibrationControls(i));
                        //Serial.println("addAllVoltageSourceCalibrationMenuItems did makeCalibrationControls!");
                        #if defined(ENABLE_SD) && defined(ENABLE_CALIBRATION_STORAGE)
                            submenuitem->add(voltage_source->makeCalibrationLoadSaveControls(i));
                        #endif

                        //Serial.println(F("\t\taddAllVoltageSourceCalibrationMenuItems done!")); Serial_flush();
                        //Serial.printf(F("\tfinished with voltage_source %i\n"), i);
                    }
                    Debug_printf(F("ParameterManager#addAllVoltageSourceCalibrationMenuItems() done!\n------------\n")); Serial_flush();

                    menu->add(submenuitem);
                }
            //#endif
        #endif

        virtual int find_slot_for_voltage(VoltageSourceBase *source) {
            for (unsigned int i = 0 ; i < voltage_sources->size() ; i++) {
                if (voltage_sources->get(i) == source)
                    return i;
            }
            return -1;
        }

        //virtual bool load_voltage_calibration();
        //virtual bool save_voltage_calibration();
};

//extern ParameterManager *parameter_manager;

#endif
