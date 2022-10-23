#ifndef PARAMETERMANAGER__INCLUDED
#define PARAMETERMANAGER__INCLUDED

//#include "Config.h"

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

        // actual i2c ADC devices, potentially with multiple channels
        LinkedList<ADCDeviceBase*> *devices = new LinkedList<ADCDeviceBase*>(); //nullptr; //LinkedList<ADCDeviceBase*>();

        // voltage-measuring channels
        LinkedList<VoltageSourceBase*> *voltage_sources = new LinkedList<VoltageSourceBase*>(); //nullptr; //= LinkedList<VoltageSourceBase*> ();

        // ParameterInputs, ie wrappers around input mechanism, assignable to a Parameter
        LinkedList<BaseParameterInput*> *available_inputs = new LinkedList<BaseParameterInput*>(); //nullptr; // LinkedList<BaseParameterInput*>();

        // Parameters, ie wrappers around destination object
        LinkedList<DoubleParameter*>    *available_parameters = new LinkedList<DoubleParameter*>(); //nullptr; //   = LinkedList<DoubleParameter*>();

        // 'blank' parameter used as default mapping
        DoubleParameter *param_none = nullptr;

        uint16_t parameter_input_colours[3] = {
            RED,
            YELLOW,
            BLUE
        };

        ParameterManager () {
            /*this->devices = new LinkedList<ADCDeviceBase*>();
            this->voltage_sources = new LinkedList<VoltageSourceBase*>();
            this->available_inputs = new LinkedList<BaseParameterInput*>();
            this->available_parameters = new LinkedList<DoubleParameter*>();*/
        }
        ~ParameterManager () {}

        FLASHMEM void init() {
            /*this->devices = new LinkedList<ADCDeviceBase*>();
            this->voltage_sources = new LinkedList<VoltageSourceBase*>();
            this->available_inputs = new LinkedList<BaseParameterInput*>();
            this->available_parameters = new LinkedList<DoubleParameter*>();*/
            this->param_none = this->addParameter(new DoubleParameter((char*)"None"));
        }

        FLASHMEM ADCDeviceBase *addADCDevice(ADCDeviceBase *device) {
            Serial.printf("ParameterManager#addADCDevice(%p)\n", device);
            this->devices->add(device);
            return device;
        }

        FLASHMEM VoltageSourceBase *addVoltageSource(VoltageSourceBase *voltage_source) {
            Serial.printf("ParameterManager#addVoltageSource(%p)\n", voltage_source);
            #ifdef LOAD_CALIBRATION_ON_BOOT
                Serial.printf("Loading calibration for %i!\n", voltage_source->slot);
                voltage_source->load_calibration();
            #endif

            this->voltage_sources->add(voltage_source);
            return voltage_source;
        }

        FLASHMEM BaseParameterInput *addInput(BaseParameterInput *input) {
            Serial.printf("ParameterManager#addInput(%p)\n", input);
            input->colour = parameter_input_colours[this->available_inputs->size() % sizeof(parameter_input_colours)];
            this->available_inputs->add(input);
            return input;
        }

        FLASHMEM DoubleParameter *addParameter(DoubleParameter *parameter) {
            Serial.printf("ParameterManager#addParameter(%p), labeled '%s'\n", parameter, parameter->label);
            this->available_parameters->add(parameter);
            return parameter;
        }
        FLASHMEM void addParameters(LinkedList<DoubleParameter*> *parameters) {
            Serial.println("ParameterManager#addParameters()..");
            for (int i = 0 ; i < parameters->size() ; i++) {
                Serial.printf("\t%i: adding from @%p '%s'\n", i, parameters->get(i), parameters->get(i)->label);
                //this->available_parameters->add(parameters->get(i));
                this->addParameter(parameters->get(i));
            }
        }

        // initialise devices and add all their voltage sources
        FLASHMEM void auto_init_devices() {
            Serial.printf("ParameterManager#auto_init_devices)\n");
            //char parameter_input_name = 'A';
            for (int i = 0 ; i < devices->size() ; i++) {
                ADCDeviceBase *device = this->devices->get(i);
                Serial.printf("ParameterManager#auto_init_devices calling init() on device %i\n", i);
                device->init();

                VoltageSourceBase *vs = device->make_voltage_source();
                int counter = 0;
                while (vs != nullptr) {
                    Serial.printf("\tParameterManager#auto_init_devices adding voltage source count %i\n", counter);
                    this->addVoltageSource(vs);
                    //this->addInput(device->make_input_for_source(parameter_input_name++, vs));
                    Serial.printf("\tParameterManager#auto_init_devices calling make_voltage_source on count %i\n", counter);
                    vs = device->make_voltage_source();
                    counter++;
                }
            }
        }

        FASTRUN BaseParameterInput *getInputForName(char input_name) {
            for(int i = 0 ; i < available_inputs->size() ; i++) {
                if (available_inputs->get(i)->name==input_name)
                    return available_inputs->get(i);
            }
            return nullptr;
        }
        FASTRUN int getInputIndexForName(char input_name) {
            for(int i = 0 ; i < available_inputs->size() ; i++) {
                if (available_inputs->get(i)->name==input_name)
                    return i;
            }
            return -1;
        }

        /*VoltageSourceBase *get_voltage_source_for_name(char name) {
            for (int i = 0 ; i < voltage_sources->size() ; i++) {
                if (voltage_sources->get(i)->name==name)
                    return voltage_sources->get(i);
            }
        }*/

        /*void connect_input_to_parameter(int input_index, DataParameter *parameter) {
            //this->available_inputs->get(input_index)->setTarget(parameter);
        }*/

        // read the values, but don't pass them on outside
        FASTRUN void update_voltage_sources() {
            if (this->debug) Serial.printf("ParameterManager#update_voltage_sources()");
            // round robin reading so they get a chance to settle in between adc reads?
            const int size = voltage_sources->size();
            if (size==0) return;
            static int last_read = 0;

            if (this->debug) Serial.printf("ParameterManager#update_voltage_sources() about to read from %i\n", last_read);
            if (this->debug) voltage_sources->get(last_read)->debug = true;

            voltage_sources->get(last_read)->update();

            if (this->debug) Serial.printf("ParameterManager#update_voltage_sources() just did read from %i\n", last_read);
            #ifdef ENABLE_PRINTF
                if (this->debug) {
                    //Serial.printf("Reading from ADC %i...", last_read);
                    Serial.printf("update_voltage_sources read value %f\n", voltage_sources->get(last_read)->current_value);
                }
            #endif

            last_read++;
            if (last_read>=size)
                last_read = 0;

            voltage_sources->get(last_read)->discard_update();   // pre-read the next one so it has a chance to settle?

            /*  // simple reading of all of them    
            for (int i = 0 ; i < voltage_sources->size() ; i++) {
                voltage_sources->get(i)->update();
            }*/
        }

        // update the ParameterInputs with the latest values from the VoltageSources
        FASTRUN void update_inputs() {
            const int available_inputs_size = available_inputs->size();
            for (int i = 0 ; i < available_inputs_size ; i++) {
                available_inputs->get(i)->loop();
            }
        }

        unsigned long profile_update_mixers = 0;
        // update every parameter's values based on the mixed ParameterInputs
        FASTRUN void update_mixers() {
            // this is going to be pretty intensive; may need to adjust the way this works...
            unsigned long update_mixers_started = micros();
            const uint16_t available_parameters_size = this->available_parameters->size();
            for (uint16_t i = 0 ; i < available_parameters_size ; i++) {
                this->available_parameters->get(i)->update_mixer();
            }
            unsigned long update_mixers_finished = micros();
            this->profile_update_mixers = update_mixers_finished - update_mixers_started;
        }

        FLASHMEM void setDefaultParameterConnections() {
            Serial.printf("ParameterManager#setDefaultParameterConnections() has %i parameters to map to %i inputs", this->available_parameters->size(), this->available_inputs->size());
            for (int i = 0 ; i < this->available_parameters->size() ; i++) {
                // todo: make this configurable dynamically / load defaults from save file
                available_parameters->get(i)->set_slot_0_input(available_inputs->get(0));
                available_parameters->get(i)->set_slot_1_input(available_inputs->get(1));
                available_parameters->get(i)->set_slot_2_input(available_inputs->get(2));
                /*for (int i = 0 ; i < MAX_SLOT_CONNECTIONS ; i++) {
                    available_parameters->get(i)->connections[i].parameter_input = available_inputs->get(i);
                }*/
            }
        }

        #ifdef ENABLE_SCREEN
            FLASHMEM void addAllParameterInputMenuItems(Menu *menu) {
                for (int i = 0 ; i < available_inputs->size() ; i++) {
                    this->addParameterInputMenuItems(menu, available_inputs->get(i));
                }
            }

            // add all the available parameters to the main menu
            FLASHMEM void addAllParameterMenuItems(Menu *menu) {
                for (int i = 0 ; i <this->available_parameters->size() ; i++) {
                    menu->add(this->makeMenuItemForParameter(this->available_parameters->get(i)));
                }
            }


            FLASHMEM SubMenuItem *addParameterSubMenuItems(Menu *menu, const char *submenu_label, LinkedList<DoubleParameter*> *parameters, int16_t default_fg_colour = C_WHITE) {
                SubMenuItem *sub = getParameterSubMenuItems(menu, submenu_label, parameters);
                menu->add(sub);
                sub->set_default_colours(default_fg_colour);
                return sub;
            }

            FLASHMEM SubMenuItem *getParameterSubMenuItems(Menu *menu, const char *submenu_label, LinkedList<DoubleParameter*> *parameters) {
                char label[MAX_LABEL_LENGTH];
                sprintf(label, "Parameters for %s", submenu_label);
                //LinkedList<DataParameter*> *parameters = behaviour_craftsynth->get_parameters();
                SubMenuItem *submenu = new SubMenuItem(label, false);
                for (int i = 0 ; i < parameters->size() ; i++) {
                    //char tmp[MENU_C_MAX];
                    //sprintf(tmp, "test item %i", i);
                    //submenu->add(new MenuItem(tmp));
                    Serial.printf("addParameterSubMenuItems(menu, '%s') processing parameter %i\n", label, i);
                    submenu->add(this->makeMenuItemForParameter(parameters->get(i)));
                }
                return submenu;
            }

            FLASHMEM MenuItem *addParameterInputMenuItems(Menu *menu, BaseParameterInput *param_input, const char *label_prefix = nullptr) {
                // TODO: a new ParameterInputControl that allows to set expected input ranges
                char label[MENU_C_MAX];
                sprintf(label, "Input %c", param_input->name);

                Serial.printf("\tdoing menu->add for ParameterInputDisplay with label '%s'\n", label);
                menu->add(new ParameterInputDisplay<LOOP_LENGTH_TICKS>(label, param_input)); //, LOOP_LENGTH_TICKS));

                DualMenuItem *submenu = new DualMenuItem("Input/Output");
                submenu->show_header = false;
                sprintf(label, "Input type for %c", param_input->name);
                //menu->add(new InputTypeSelectorControl<BaseParameterInput,byte>(label, param_input, &BaseParameterInput::set_input_type, &BaseParameterInput::get_input_type));
                submenu->add(new InputTypeSelectorControl(label, &param_input->input_type));
                sprintf(label, "Output type for %c", param_input->name);
                submenu->add(new InputTypeSelectorControl(label, &param_input->output_type)); //, &BaseParameterInput::set_output_type, &BaseParameterInput::get_output_type));
                menu->add(submenu);

                return submenu; // was nullptr
            }

            // create a menuitem for the passed-in parameter; returns nullptr if passed-in parameter is named "None"
            FLASHMEM MenuItem *makeMenuItemForParameter(DoubleParameter *p, char *label_prefix = nullptr) {
                if (strcmp(p->label,"None")==0) return nullptr;
                MenuItem *ctrl = p->makeControl();
                return ctrl;
            }

            /*FLASHMEM void *addAllVoltageSourceMenuItems(Menu *menu) {
                Serial.printf(F("------------\nParameterManager#addAllVoltageSourceMenuItems() has %i VoltageSources to add items for?\n"), this->voltage_sources->size());
                const int size = this->voltage_sources->size();
                for (int i = 0 ; i < size ; i++) {
                    Serial.printf(F("\tParameterManager#addAllVoltageSourceMenuItems() for voltage_source %i/%i\n"), i+1, size); Serial.flush();

                    VoltageSourceBase *voltage_source = this->voltage_sources->get(i);
                    if (voltage_source!=nullptr) {
                        menu->add(voltage_source->makeCalibrationControls(i));
                        Serial.println(F("\t\tmakeCalibrationControls done!")); Serial.flush();
                    }

                    Serial.printf(F("\tfinished with voltage_source %i\n"), i);
                }
                Serial.printf(F("ParameterManager#addAllVoltageSourceMenuItems() done!\n------------\n")); Serial.flush();
            }*/

            FLASHMEM void addAllVoltageSourceCalibrationMenuItems(Menu *menu) {
                Serial.printf(F("------------\nParameterManager#addAllVoltageSourceCalibrationMenuItems() has %i VoltageSources to add items for?\n"), this->voltage_sources->size());
                const int size = this->voltage_sources->size();

                SubMenuItem *submenuitem = new SubMenuItem("Voltage Source Calibration", false);

                for (int i = 0 ; i < size ; i++) {
                    Serial.printf(F("\tParameterManager#addAllVoltageSourceCalibrationMenuItems() for voltage_source %i/%i\n"), i+1, size); Serial.flush();
                    
                    VoltageSourceBase *voltage_source = this->voltage_sources->get(i);
                    submenuitem->add(voltage_source->makeCalibrationControls(i));
                    submenuitem->add(voltage_source->makeCalibrationLoadSaveControls(i));

                    Serial.println(F("\t\taddAllVoltageSourceCalibrationMenuItems done!")); Serial.flush();

                    Serial.printf(F("\tfinished with voltage_source %i\n"), i);
                }
                Serial.printf(F("ParameterManager#addAllVoltageSourceCalibrationMenuItems() done!\n------------\n")); Serial.flush();

                menu->add(submenuitem);
            }
        #endif

        virtual int find_slot_for_voltage(VoltageSourceBase *source) {
            for (int i = 0 ; i < voltage_sources->size() ; i++) {
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
