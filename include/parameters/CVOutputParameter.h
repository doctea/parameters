#pragma once

#include "parameters/Parameter.h"

#include "midi_helpers.h"

// todo: make CVOutputParameter more agnostic about underlying library so it can support other DACs
#include "DAC8574.h"

// todo: add auto-calibration ability
// todo: figure out an interface to allow using CVOutputParameter as a 1v/oct output from MIDI...
// todo: make able to switch between unipolar/bipolar modes, with different calibration for each

#ifdef ENABLE_SCREEN
    #include "menu.h"
    #include "submenuitem_bar.h"
    #include "mymenu_items/ParameterInputTypeSelector.h"
    #include "menuitems_lambda.h"
#endif

// todo: figure out a way to include these from a file
class VoltageParameterInput;
uint16_t calibrate_find_dac_value_for(int channel, VoltageParameterInput *src, float intended_voltage, bool inverted = false);
uint16_t calibrate_find_dac_value_for(int channel, char *input_name, float intended_voltage, bool inverted = false);
void parameter_manager_calibrate(ICalibratable* v);

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
                //this->debug = true;
        }

        virtual void set_parameter_input_for_calibration(VoltageParameterInput *calibration_input) {
            this->calibration_input = calibration_input;
        }

        /*virtual const char* getFormattedValue() override {
            static char fmt[MENU_C_MAX] = "              ";
            sprintf(fmt, "%i", this->getCurrentDataValue());
            //Serial.printf("getFormattedValue: '%s'\n", fmt);
            return fmt;
        };*/


        void calibrate() override {
            Serial.printf("Finding lowest value..\n");
            this->calibrated_lowest_value = calibrate_find_dac_value_for(this->channel, this->calibration_input, 0.0, this->inverted);
            Serial.printf("Finding highest value..\n");
            this->calibrated_highest_value = calibrate_find_dac_value_for(this->channel, this->calibration_input, 10.0, this->inverted);
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
                if (this->debug && Serial) 
                    Serial.printf("CVParameter#setTargetValueFromData(%3.3f, %i)\n", value, this->channel);

                uint16_t calibrated_dac_value = get_dac_value_for_voltage(value);

                if (last_value!=calibrated_dac_value || force) {
                    // only store the pending value, for setting DAC value later during process_pending
                    this->pending_value = calibrated_dac_value;
                    this->is_pending_value = true;
                }

                last_value = calibrated_dac_value;
            } else {
                if (this->debug) Serial.printf("WARNING: No target set in CVParameter#setTargetValueFromData in '%s'!\n", this->label);
            }
        }

        // called after all parameter modulation processing has been done so that reading and writing don't get entangled and cause problems
        virtual void process_pending() override {
            if (is_pending_value) {
                if (Serial) Serial.printf("%u\t: %s#setTargetValueFromData(%u)!\n", micros(), this->label, pending_value);
                this->target->write(channel, pending_value);
                int error = this->target->lastError();
                if (error>0) {
                    if (Serial) Serial.printf("\t!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! got error code %02x!\n", error);
                }
                if (Serial) Serial.printf("%u\t: %s#completed write!\n", micros(), this->label);
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
                SubMenuItem *bar = new SubMenuItemBar("Settings", true, false);

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

                //controls->add(new ActionConfirmItem("Start calibrating", &start_calibration));
            
                //controls->add(new DirectNumberControl<DataType>("uni min", &calibrated_min_output_voltage, calibrated_min_output_voltage, -20.0, 20.0));
                //controls->add(new DirectNumberControl<DataType>("uni max", &calibrated_max_output_voltage, calibrated_max_output_voltage, 0.0, 20.0));
            }

            // calibration settings..
            //controls->add(new VALUE_TYPE<uint16_t>("uni min dac", &calibrated_lowest_value, calibrated_lowest_value,  0, __UINT16_MAX__));
            SubMenuItem *bar1 = new SubMenuItemBar("Settings", true, false);
            // todo: make InputTypeSelectorControl fire a function to switch the ranges+calibration
            InputTypeSelectorControl<> *type_selector = new InputTypeSelectorControl<>("Polarity", &this->polarity);
            bar1->add(type_selector);
            bar1->add(new ToggleControl<bool>("Invert", &this->inverted));
            bar1->add(new LambdaActionItem("Calibrate", [=](void) -> void { 
                parameter_manager_calibrate(this);
                //this->start_calibration(); 
            }));

            SubMenuItem *bar2 = new SubMenuItemBar("Settings", true, false);
            bar2->add(new DirectNumberControl<uint16_t>("uni min dac", &calibrated_lowest_value,  calibrated_lowest_value,  0, __UINT16_MAX__));
            bar2->add(new DirectNumberControl<uint16_t>("uni max dac", &calibrated_highest_value, calibrated_highest_value, 0, __UINT16_MAX__));

            // insert controls at the top
            controls->add(bar1);
            controls->add(bar2);

            return controls; 
        };

        virtual float get_voltage_for_pitch(int8_t pitch, int16_t detune_cents = 0) {
            if(!is_valid_note(pitch))
                return 0.0;
            return constrain((float)pitch / 12.0, this->minimumDataRange, this->maximumDataRange);
        }

        virtual void sendNoteOn(uint8_t pitch, uint8_t velocity, uint8_t channel) {
            if(is_valid_note(pitch)) {
                float voltage_for_pitch = get_voltage_for_pitch(pitch);
                Serial.printf("%s#sendNoteOn(pitch=%3i,....)\t", this->label, pitch);
                Serial.printf("-> %3s,\t", get_note_name_c(pitch));
                Serial.printf("got voltage_for_pitch = %3.3f\n", voltage_for_pitch);
                this->updateValueFromData(voltage_for_pitch);
            }
        }
        virtual void sendNoteOff(uint8_t pitch, uint8_t velocity, uint8_t channel) {
        }
};
