#pragma once

//#define ENABLE_CV_OUTPUT 0x4C 

#ifdef ENABLE_CV_OUTPUT

#include "debug.h"

#include "parameters/Parameter.h"

#include "calibration.h"

#include "midi_helpers.h"

// todo: make CVOutputParameter more agnostic about underlying library so it can support other DACs
#include <DAC8574.h>

// todo: add auto-calibration ability
// todo: figure out an interface to allow using CVOutputParameter as a 1v/oct output from MIDI...
// todo: make able to switch between unipolar/bipolar modes, with different calibration for each

#ifdef ENABLE_SCREEN
    #include "menu.h"
    #include "submenuitem_bar.h"
    #include "mymenu_items/ParameterInputTypeSelector.h"
    #include "mymenu_items/ParameterInputMenuItems.h"
    #include "menuitems_lambda.h"
#endif

// todo: figure out a way to include these from a file
class VoltageParameterInput;
uint16_t calibrate_find_dac_value_for(DAC8574 *dac_output, int channel, VoltageParameterInput *src, float intended_voltage, bool inverted = false);
uint16_t calibrate_find_dac_value_for(DAC8574 *dac_output, int channel, char *input_name, float intended_voltage, bool inverted = false);
void parameter_manager_calibrate(ICalibratable* v);


#if defined(ENABLE_CALIBRATION_STORAGE)
    #ifdef ENABLE_SD
        #define STORAGE SD
        #include "SdFat.h"
    #endif
    #ifdef ENABLE_LITTLEFS
        #define STORAGE LittleFS
        #include <LittleFS.h>
    #endif

    //#include "SD.h"
    #ifdef ENABLE_SD
        #include "SD.h"
        #define FILEPATH_CVOUTPUT_CALIBRATION_FORMAT       "calibration_cvoutput_%s.txt"
        #define FILE_READ_MODE FILE_READ
        #define FILE_WRITE_MODE FILE_WRITE_BEGIN
    #elif defined (ENABLE_LITTLEFS)
        // LittleFS has maximum filepath length of 31 characters
        #define FILE_READ_MODE "r"
        #define FILE_WRITE_MODE "w"
        #define FILEPATH_CVOUTPUT_CALIBRATION_FORMAT       "calib_cvout_%s.txt"
    #endif
#endif


// for applying modulation to a value before sending CC values out to the target device
// eg, a synth's cutoff value
template<class TargetClass=DAC8574, class DataType=float>
class CVOutputParameter : virtual public DataParameter<TargetClass,DataType>, virtual public ICalibratable, virtual public IMIDINoteAndCCTarget {
    public:
        byte channel = 0;
        //DataType floor, ceiling;

        bool inverted = false;
        VALUE_TYPE polarity = VALUE_TYPE::UNIPOLAR;

        //DataType calibrated_min_output_voltage = 0.33;
        //DataType calibrated_max_output_voltage = 9.53;

        uint16_t calibrated_lowest_value  = 2044; //1487; //2163; 
        uint16_t calibrated_highest_value = 59386; //62321; //62455;

        bool configurable = false;

        VoltageParameterInput *calibration_input = nullptr;

        CVOutputParameter(const char* label, TargetClass *target, byte channel, VALUE_TYPE polarity_mode = VALUE_TYPE::UNIPOLAR, bool inverted = false, float floor = 0.0f, float ceiling = 10.0f, bool configurable = false)
            : DataParameter<TargetClass,DataType>(label, target) {
                this->channel = channel;
                this->minimumDataLimit = this->minimumDataRange = floor; //MIDI_MIN_CC;
                this->maximumDataLimit = this->maximumDataRange = ceiling; //MIDI_MAX_CC;
                this->configurable = configurable;

                this->inverted = inverted;
                this->polarity = polarity_mode;

                // todo: probably make this off by default, and configurable..
                this->slew_enabled = true;

                //this->debug = true;
                if (this->debug) Serial_printf("=== %s doing initial constructor, loading calibration, and sending initial value...", this->label);

                #ifdef LOAD_CALIBRATION_ON_BOOT
                    // note that this probably won't actually load anything because SD has not yet been initialised!
                    this->load_calibration();
                #endif

                // force setting output to the starting value

                this->setTargetValueFromData(this->getCurrentDataValue(), true);
                this->process_pending();

                if (this->debug) Serial_printf("=== %s done initial constructor, loading calibration, and sending initial value.", this->label);
                //this->debug = false;
        }

        virtual BaseParameterInput *get_calibration_parameter_input() {
            return this->calibration_input;
        }

        virtual void set_calibration_parameter_input(BaseParameterInput *calibration_input) {
            if (calibration_input!=nullptr) {
                Serial_printf("CVOutputParameter#set_calibration_parameter_input(%s) in %s\n", calibration_input->name, this->label);
            } else {
                Serial_printf("CVOutputParameter#set_calibration_parameter_input(nullptr) in %s\n", this->label);
            }
            this->calibration_input = (VoltageParameterInput*)calibration_input;
        }
        virtual void set_calibration_parameter_input(const char *input_name) {
            this->set_calibration_parameter_input(parameter_manager->getInputForName(input_name));
        }

        /*virtual const char* getFormattedValue() override {
            static char fmt[MENU_C_MAX] = "              ";
            sprintf(fmt, "%i", this->getCurrentDataValue());
            //Serial.printf("getFormattedValue: '%s'\n", fmt);
            return fmt;
        };*/


        void calibrate() override {
            Serial.println("Finding lowest value..\n");
            this->calibrated_lowest_value = calibrate_find_dac_value_for(this->target, this->channel, this->calibration_input, 0.0, this->inverted);
            Serial.println("Finding highest value..\n");
            this->calibrated_highest_value = calibrate_find_dac_value_for(this->target, this->channel, this->calibration_input, 10.0, this->inverted);
        }
        
        virtual DataType map(DataType x, DataType in_min, DataType in_max, DataType out_min, DataType out_max) {
            return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
        }

        /*virtual DataType get_calibrated_voltage(DataType intended_voltage) {
            // if 0.0 gives -0.351
                // then to get real 0.0 we need to add 0.351 to it
            // if 10.0 gives 10.297
                // then to get real 10.0 we need to subtract 0.297 from it
                // so therefore max should be 10.0 - (10.297-10.0) ?

            // zero offset = uni_min_output_voltage * -65535
            float zero_offset = calibrated_min_output_voltage; // * -65535.0;
            float apex_offset = calibrated_max_output_voltage;
            //float total_range = uni_max_output_voltage - uni_min_output_voltage;
            //float total_range = uni_max_output_voltage - zero_offset + apex_offset;

            //intended_voltage += zero_offset;
            //intended_voltage /= 10.0;
            //intended_voltage *= (total_range/10.0);
            //intended_voltage *= 10.0;
            //intended_voltage = map(intended_voltage, 0.0, 10.0, uni_min_output_voltage, total_range);

            float real_zero = zero_offset;
            float real_apex = apex_offset;
            //Serial.printf("real_zero=%3.3f, ",    real_zero);
            //Serial.printf("real_apex=%3.3f, ",    real_apex);
            //Serial.printf("uni_max_output_voltage=%3.3f, ", uni_max_output_voltage);
            //Serial.printf("apex_offset=%3.3f, ",    apex_offset);
            //Serial.printf(" => \n");

            intended_voltage = map(intended_voltage, this->minimumDataLimit, this->maximumDataLimit, real_zero, real_apex);

            return intended_voltage;
        }*/
        
        virtual uint16_t get_calibrated_dac_value(DataType uncalibrated_voltage) {
            //DataType calibrated_voltage = get_calibrated_voltage(uncalibrated_voltage);

            uint16_t dac_value = (float)__UINT16_MAX__ * (uncalibrated_voltage / this->maximumDataLimit);

            dac_value = map(dac_value, 0, __UINT16_MAX__, calibrated_lowest_value, calibrated_highest_value);

            return dac_value;
        }

        virtual uint16_t get_dac_value_for_voltage(DataType uncalibrated_voltage) {
            uint16_t dac_value = get_calibrated_dac_value(uncalibrated_voltage);

            if (inverted)
                dac_value = __UINT16_MAX__ - dac_value;

            return dac_value;
        }

        bool is_pending_value = false;
        uint16_t pending_value = 0; 
        uint16_t last_value = 0;

        virtual void setTargetValueFromData(DataType value, bool force = false) override {
            if (this->target!=nullptr) {
                if (this->debug) Serial_printf("CVParameter#setTargetValueFromData(%3.3f, %i)\n", value, this->channel);

                uint16_t calibrated_dac_value = get_dac_value_for_voltage(value);

                // TODO: without this, output value widget doesn't update; with it, 
                // slew doesn't work since it incorrectly records the mid-slew value as the current target value
                //this->currentDataValue = value;

                if (last_value!=calibrated_dac_value || force) {
                    // only store the pending value, for setting DAC value later during process_pending
                    if (this->debug) Serial_printf("%s: storing pending_value = %u\t from input value %3.3f\n", this->label, pending_value, value);
                    this->pending_value = calibrated_dac_value;
                    this->is_pending_value = true;
                }

                last_value = calibrated_dac_value;
            } else {
                if (this->debug) Serial_printf("WARNING: target object is nullptr in CVParameter#setTargetValueFromData in '%s'!\n", this->label);
            }
        }

        // called after all parameter modulation processing has been done so that reading and writing don't get entangled and cause problems
        virtual void process_pending() override {
            if (is_pending_value) {
                if (this->debug) Serial_printf("%u\t: %s#setTargetValueFromData(%u) on channel %u!\n", micros(), this->label, pending_value, channel);
                this->target->write(channel, pending_value);
                /* // for testing all channels
                this->target->write(1, pending_value);
                this->target->write(2, pending_value);
                this->target->write(3, pending_value);*/
                int error = this->target->lastError();
                if (error>0) {
                    if (this->debug) Serial_printf("\t!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! got error code %02x!\n", error);
                }
                if (this->debug) {
                    // note that this actually reads back from the device to confirm the write was successful, so it's a bit slow
                    Serial_printf("%u\t: %s#completed write!\n", micros(), this->label);
                    Serial_printf("%u\t: %s#reading back %u (wrote %u)\n", micros(), this->label, this->target->read(channel), pending_value);
                }
                is_pending_value = false;
            }
        }

        virtual bool load_parse_key_value(String key, String value) override {
            if (this->configurable) {
                //Serial.printf("CVParameter named '%s' asked to load_parse_key_value with '%s' => '%s'..", this->label, key.c_str(), value.c_str());
                String prefix = String("parameter_value_") + String(this->label) + String("_");
                //Serial.printf("Checking if starts with '%s'..", prefix.c_str());
                if (key.startsWith(prefix)) {
                    //Serial.println("YES! now checking if endswith '_channel' or '_cc'..");
                    if (key.endsWith("_channel")) {
                        Serial.println("YES! Ends with _channel");
                        this->channel = value.toInt();
                        return true;
                    } /*else if (key.endsWith("_cc")) {
                        Serial.println("YES! Ends with _cc");
                        this->cc_number = value.toInt();
                        return true;
                    }*/
                }
                //Serial.println("no match.");
            }
            return FloatParameter::load_parse_key_value(key, value);
        }
        virtual void save_pattern_add_lines(LinkedList<String> *lines) override {
            if (this->configurable) {
                String prefix = String("parameter_value_") + String(this->label); //+ String("_");
                lines->add(prefix + String("_channel=") + String(this->channel));
                //lines->add(prefix + String("_cc=") + String(this->cc_number));
            }
            FloatParameter::save_pattern_add_lines(lines);
        }

        //FLASHMEM virtual LinkedList<MenuItem *> *makeControls() override;
        FLASHMEM
        virtual LinkedList<MenuItem *> *addCustomTypeControls(LinkedList<MenuItem *> *controls) override { 
            if (this->configurable) {
                //SubMenuItem *bar = new SubMenuItemBar("Settings", true, false);

                /*bar->add(new DirectNumberControl<byte>(
                    "Output CC", //label, 
                    &this->cc_number,
                    this->cc_number,
                    MIDI_MIN_VELOCITY,
                    MIDI_MAX_VELOCITY
                ));*/

                /*bar->add(new DirectNumberControl<byte>(
                    "Output MIDI Channel", 
                    &this->channel,
                    this->channel,
                    MIDI_MIN_CHANNEL,
                    MIDI_MAX_CHANNEL
                ));*/

                // TODO: ... calibration settings maybe?
            }

            // calibration settings..
            //controls->add(new VALUE_TYPE<uint16_t>("uni min dac", &calibrated_lowest_value, calibrated_lowest_value,  0, __UINT16_MAX__));
            SubMenuItem *bar1 = new SubMenuItemBar("Settings", true, false);
            // todo: make InputTypeSelectorControl fire a function to switch the ranges+calibration
            InputTypeSelectorControl<> *type_selector = new InputTypeSelectorControl<>("Polarity", &this->polarity);
            bar1->add(type_selector);
            bar1->add(new ToggleControl<bool>("Invert", &this->inverted));

            // insert controls at the top
            controls->add(bar1);

            return controls; 
        };

        virtual LinkedList<MenuItem*> *makeCalibrationControls() { 
            LinkedList<MenuItem*> *controls = new LinkedList<MenuItem*>();

            SubMenuItem *bar2 = new SubMenuItemBar("Calibration", true, false);
            bar2->add(new ParameterInputSelectorControl<CVOutputParameter>(
                "Cal Input", 
                this,
                &CVOutputParameter::set_calibration_parameter_input,
                &CVOutputParameter::get_calibration_parameter_input,
                parameter_manager->get_available_pitch_inputs(),
                this->calibration_input
            ));
            bar2->add(new LambdaActionConfirmItem("Calibrate", [=](void) -> void { 
                parameter_manager_calibrate(this);
            }));
            LambdaNumberControl<float> *input = new LambdaNumberControl<float>(
                    "Input", 
                    [=] (float value) -> void {}, 
                    [=] () -> float { 
                        if (this->calibration_input!=nullptr) 
                            return this->calibration_input->get_voltage(); 
                        return 0.0;
                    },
                    nullptr,
                    -10.0,
                    10.0,
                    false, 
                    false
                );
            input->setReadOnly(true);
            input->selectable = false;
            bar2->add(input );               

            SubMenuItem *bar3 = new SubMenuItemBar("Settings", true, false);
            //bar3->add(new DirectNumberControl<uint16_t>("uni min dac", &calibrated_lowest_value,  calibrated_lowest_value,  0, __UINT16_MAX__));
            //bar3->add(new DirectNumberControl<uint16_t>("uni max dac", &calibrated_highest_value, calibrated_highest_value, 0, __UINT16_MAX__));
            bar3->add(new LambdaNumberControl<DataType>("Output", 
                [=] (DataType value) -> void { 
                    this->setTargetValueFromData(value, true);
                }, 
                [=] () -> DataType { 
                    return this->getCurrentDataValue();
                },
                nullptr,
                this->minimumDataRange,
                this->maximumDataRange,
                true, 
                true
            ));
            //if (strcmp(this->label, "CVPO1-A")==0) bar3->items->get(0)->debug = true;

            bar3->add(new LambdaNumberControl<uint16_t>("uni min dac", 
                [=] (uint16_t value) -> void { 
                    this->calibrated_lowest_value = value; 
                    this->setTargetValueFromData(this->getCurrentDataValue(), true);
                }, 
                [=] () -> uint16_t { 
                    return this->calibrated_lowest_value; 
                },
                nullptr,
                0,
                __UINT16_MAX__,
                true, 
                true
            ));
            bar3->add(new LambdaNumberControl<uint16_t>("uni max dac", 
                [=] (uint16_t value) -> void { 
                    this->calibrated_highest_value = value; 
                    this->setTargetValueFromData(this->getCurrentDataValue(), true);
                }, 
                [=] () -> uint16_t { 
                    return this->calibrated_highest_value; 
                },
                nullptr,
                0,
                __UINT16_MAX__,
                true, 
                true
            ));

            controls->add(bar2);
            controls->add(bar3);

            return controls;
        }

        virtual MenuItem *makeCalibrationLoadSaveControls() { 
            //LinkedList<MenuItem*> *controls = new LinkedList<MenuItem*>();

            SubMenuItem *bar4 = new SubMenuItemBar("Calibration", false, false);
            bar4->add(new LambdaActionConfirmItem("Save", [=](void) -> void { 
                this->save_calibration(); 
            }));
            bar4->add(new LambdaActionConfirmItem("Load", [=](void) -> void {
                this->load_calibration(); 
            }));

            return bar4;
        }

        virtual float get_voltage_for_pitch(int8_t pitch, int16_t detune_cents = 0) {
            if(!is_valid_note(pitch))
                return 0.0;
            return constrain((float)pitch / 12.0, this->minimumDataRange, this->maximumDataRange);
        }

        virtual void sendNoteOn(uint8_t pitch, uint8_t velocity, uint8_t channel) {
            if (!is_valid_note(pitch))
                return;

            float voltage_for_pitch = get_voltage_for_pitch(pitch);
            if (this->debug && Serial) {
                Serial.printf("%s#sendNoteOn(pitch=%3i,....)\t", this->label, pitch);
                Serial.printf("-> %3s,\t", get_note_name_c(pitch));
                Serial.printf("got voltage_for_pitch = %3.3f\n", voltage_for_pitch);
            }
            this->updateValueFromData(voltage_for_pitch);

            if (gate_bank>=0 && gate_manager!=nullptr) {
                gate_manager->send_gate_on(gate_bank, gate_number);
            }
        }
        virtual void sendNoteOff(uint8_t pitch, uint8_t velocity, uint8_t channel) {
            // todo: should probably have an option to drop the output voltage to 0 when no notes are left?
            if (gate_bank>=0 && gate_manager!=nullptr) {
                gate_manager->send_gate_off(gate_bank, gate_number);
            }
        }

        IGateTarget *gate_manager = nullptr;
        int8_t gate_bank = -1, gate_number = 0;

        virtual void set_gate_outputter(IGateTarget *gate_manager, int8_t gate_bank, int8_t gate_number) {
            this->gate_manager = gate_manager;
            this->gate_bank = gate_bank;
            this->gate_number = gate_number;
        }

        virtual void save_calibration() override {
            // todo: make VoltageSource know its name so that it knows where to save to
            Debug_printf("CVOutputParameter: save_calibration for slot %i!\n", global_slot);
            //int slot = parameter_manager.find_slot_for_voltage(this);

            //parameter_manager->save_voltage_calibration(slot);

            Debug_printf("\tfor slot %s, saving calibration values %u : %u\n", this->label, this->calibrated_lowest_value, this->calibrated_highest_value);
        
            char filename[255] = "";
            snprintf(filename, 255, FILEPATH_CVOUTPUT_CALIBRATION_FORMAT, this->label); //, preset_number);
            Debug_printf("\tsave_calibration() opening %s\n", filename);

            if (STORAGE.exists(filename)) {
                Debug_println("\tfile exists - removing!");
                STORAGE.remove(filename);
            }

            File myFile = STORAGE.open(filename, FILE_WRITE_MODE /*FILE_WRITE_BEGIN*/);
            if (myFile) {
                //myFile.printf("correction_value_1=%6.6f\n", this->correction_value_1);
                //myFile.printf("correction_value_2=%6.6f\n", this->correction_value_2);
                myFile.printf("calibrated_lowest_value=%u\n", this->calibrated_lowest_value);
                myFile.printf("calibrated_highest_value=%u\n", this->calibrated_highest_value);
                myFile.close();
                Debug_printf("\tsaved!\n");
                //message_log("Saved calibration!");
            } else {
                Debug_printf("\tError saving calibration!\n");
                //message_log("Error saving calibration!");
            }
            //myFile.close();
            Debug_printf("\tEnd of save_calibration.\n");
        }
        virtual void load_calibration() override {
            // todo: make VoltageSource know its name so that it knows where to load from
            //Debug_printf("CVOutputParameter: load_calibration for slot!\n", slot);
            Serial_printf("CVOutputParameter: load_calibration for slot %s!\n", this->label);
            //int slot = parameter_manager.find_slot_for_voltage(this);

            //parameter_manager->load_voltage_calibration(slot); //, this);
            File myFile;

            #ifdef ENABLE_SD
                if (!STORAGE.mediaPresent()) {
                    Serial_println("No SD card found!");
                    return;
                }
            #endif

            char filename[255] = "";
            snprintf(filename, 255, FILEPATH_CVOUTPUT_CALIBRATION_FORMAT, this->label); //, preset_number);
            Debug_printf("\tload_calibration() opening '%s' for global slot %s\n", filename, this->label);
            myFile.setTimeout(0);
            myFile = STORAGE.open(filename, FILE_READ_MODE);

            if (!myFile) {
                //Debug_printf("\tError: Couldn't open '%s' for reading for slot %i!\n", filename, slot);
                Serial_printf("\tError: Couldn't open '%s' for reading for global slot %s!\n", filename, this->label);
                return; // false;
            }
            String line;
            while (myFile.available()) {
                line = myFile.readStringUntil('\n');
                String key = line.substring(0, line.indexOf("="));
                String value = line.substring(line.indexOf("=")+1);
                Debug_printf("\tfor %s, found value '%s' => %6.6f\n", key.c_str(), value.c_str(), value.toFloat());
                /*if (key.equals("correction_value_1")) {
                    this->correction_value_1 = value.toFloat();
                } else if (key.equals("correction_value_2")) {
                    this->correction_value_2 = value.toFloat();
                }*/
                if (key.equals("calibrated_lowest_value")) {
                    this->calibrated_lowest_value = value.toInt();
                } else if (key.equals("calibrated_highest_value")) {
                    this->calibrated_highest_value = value.toInt();            
                }
            }
            myFile.close();

            // now re-send value, which should now be based on the newly loaded calibration data
            this->setTargetValueFromData(this->getCurrentDataValue(), true);

            Debug_printf("for slot %i, got calibration values %u : %u\n", this->label, this->calibrated_lowest_value, this->calibrated_highest_value);
        }
};

#endif