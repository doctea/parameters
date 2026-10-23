#pragma once

#if defined(ENABLE_CV_OUTPUT) || defined(ENABLE_CV_OUTPUT_2) || defined(ENABLE_CV_OUTPUT_3)

#include "debug.h"

#include "parameters/Parameter.h"

#include "icalibration.h"

#include "midi_helpers.h"

// todo: make CVOutputParameter more agnostic about underlying library so it can support other DACs
#include <DAC8574.h>

#ifdef ENABLE_SCREEN
    #include "menu.h"
    #include "submenuitem_bar.h"
    #include "mymenu_items/ParameterInputTypeSelector.h"
    #include "mymenu_items/ParameterInputMenuItems.h"
    #include "menuitems_lambda.h"
    // Forward declarations for CVOutputFeedbackCalibControls (full definition included after class body)
    template<class DACClass, class DataType> class CVOutputFeedbackCalibState;
    template<class DACClass, class DataType>
    SubMenuItem *makeFeedbackCalibrationControls(CVOutputFeedbackCalibState<DACClass, DataType>*);
#endif

// todo: figure out a way to include these from a file
class VoltageParameterInput;
uint16_t calibrate_find_dac_value_for(DAC8574 *dac_output, int channel, VoltageParameterInput *src, float intended_voltage, bool inverted = false);
uint16_t calibrate_find_dac_value_for(DAC8574 *dac_output, int channel, char *input_name, float intended_voltage, bool inverted = false);
void parameter_manager_calibrate(ICalibratable* v);

// Abstract base class for CV output parameters (lightweight, no heavy deps).
// Defined in its own header to avoid circular includes via ParameterInputMenuItems.h.
#include "parameters/CVOutputParameterBase.h"


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


// for applying modulation to a value, before sending CC values out to the target device and triggering a gate if needed
// eg, a synth's cutoff value
template<class TargetClass=DAC8574, class DataType=float>
class CVOutputParameter : virtual public DataParameter<TargetClass,DataType>, virtual public ICalibratable, virtual public IMIDINoteAndCCTarget, public CVOutputParameterBase {
    public:
        byte dac_channel = 0;

        bool inverted = false;
        bool calibration_mode = false;  // when true, process_pending() is a no-op (blocks modulation during feedback calibration)
        bool midi_notes_enabled = true;
        DataType output_voltage = 0;
        VALUE_TYPE polarity = VALUE_TYPE::UNIPOLAR;

        uint16_t calibrated_lowest_value  = 2044; //1487; //2163; 
        uint16_t calibrated_highest_value = 59386; //62321; //62455;

        bool configurable = false;

        VoltageParameterInput *calibration_input = nullptr;

        int8_t current_pitch_note = NOTE_OFF;
        float omni_pitch_bend_semitones = 0.0f;
        float channel_pitch_bend_semitones = 0.0f;

        // for sending gate signals when notes are triggered
        IGateTarget *gate_manager = nullptr;
        int8_t gate_bank = -1, gate_number = 0;
        bool gate_is_on = false;    // track whether gate is currently on or off
        bool note_is_active = false; // track whether a NoteOn has been received without matching NoteOff (independent of gate)
        bool gate_output_enabled = true;

        // Post-modulation slew rate (written to by ProxyParameter<float> modulation).
        // get_slewed_value() uses this via get_effective_slew_rate_normal() override.
        // Base (user-set) value is stored in slew_base_normal[] in DeviceBehaviour_CVOutput.
        float effective_slew_rate_normal = 1.0f;
        virtual float get_effective_slew_rate_normal() override { return effective_slew_rate_normal; }

        virtual DataType getCurrentDataValue() override {
            return midi_notes_enabled ? this->currentDataValue : (DataType)0;
        }

        CVOutputParameter(const char* label, TargetClass *target, byte dac_channel, VALUE_TYPE polarity_mode = VALUE_TYPE::UNIPOLAR, bool inverted = false, float floor = 0.0f, float ceiling = 10.0f, bool configurable = false)
            : DataParameter<TargetClass,DataType>(label, target) {
                this->dac_channel = dac_channel;
                this->minimumDataLimit = this->minimumDataRange = floor; //MIDI_MIN_CC;
                this->maximumDataLimit = this->maximumDataRange = ceiling; //MIDI_MAX_CC;
                this->configurable = configurable;

                this->inverted = inverted;
                this->polarity = polarity_mode;

                this->float_unit = 'v';

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

        void calibrate() override {
            if (this->calibration_input == nullptr) {
                Serial_printf("CVOutputParameter#calibrate(%s): no calibration input configured\n", this->label);
                return;
            }
            Serial_println("Finding lowest value..\n");
            this->calibrated_lowest_value = calibrate_find_dac_value_for(this->target, this->dac_channel, this->calibration_input, this->minimumDataLimit, this->inverted);
            Serial_println("Finding highest value..\n");
            this->calibrated_highest_value = calibrate_find_dac_value_for(this->target, this->dac_channel, this->calibration_input, this->maximumDataLimit, this->inverted);
        }
        
        virtual DataType map(DataType x, DataType in_min, DataType in_max, DataType out_min, DataType out_max) {
            DataType result = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
            // if (Serial) Serial.printf("%s: map(x=%f,\tin_min=%f,\tin_max=%f,\tout_min=%f,\tout_max=%f)\t= result=%f\n", this->get_cv_label(), x, in_min, in_max, out_min, out_max, result);
            return result;
        }
        
        virtual uint16_t get_calibrated_dac_value(DataType uncalibrated_voltage) {
            // Calibration spans the physical voltage limits, not the editable modulation range.
            DataType voltage = this->constrainDataToLimit(uncalibrated_voltage);
            return (uint16_t)map(
                voltage, 
                this->minimumDataLimit, 
                this->maximumDataLimit,
                calibrated_lowest_value, 
                calibrated_highest_value
            );
        }

        virtual uint16_t get_dac_value_for_voltage(DataType uncalibrated_voltage) {
            uint16_t dac_value = get_calibrated_dac_value(uncalibrated_voltage);

            if (inverted)
                dac_value = __UINT16_MAX__ - dac_value;

            return dac_value;
        }

        virtual float get_pitch_bend_semitones() const {
            return this->omni_pitch_bend_semitones + this->channel_pitch_bend_semitones;
        }

        virtual float get_channel_pitch_bend_semitones() const {
            return this->channel_pitch_bend_semitones;
        }

        virtual float get_omni_pitch_bend_semitones() const {
            return this->omni_pitch_bend_semitones;
        }

        virtual void refresh_pitch_output(bool force = true) {
            if (!midi_notes_enabled || calibration_mode) return;
            if (is_valid_note(this->current_pitch_note)) {
                DataType voltage_for_pitch = get_voltage_for_pitch(this->current_pitch_note);
                #if CV_PITCH_BEND_TRACE
                    Serial_printf("CVPITCH %s refresh note=%d bend_omni=%0.3f bend_chan=%0.3f -> %0.4fV\n",
                        this->label,
                        this->current_pitch_note,
                        this->omni_pitch_bend_semitones,
                        this->channel_pitch_bend_semitones,
                        (float)voltage_for_pitch);
                #endif
                if (this->getCurrentDataValue() != voltage_for_pitch) {
                    this->updateValueFromData(voltage_for_pitch);
                } else if (force) {
                    // Force a resend through the normal path so slot modulation remains additive.
                    this->setTargetValueFromNormal(this->getCurrentNormalValue(), true);
                }
                this->process_pending();
            }
        }

        virtual void set_pitch_bend_semitones(float semitones, bool force = false) {
            this->set_channel_pitch_bend_semitones(semitones, force);
        }

        virtual void apply_pitch_bend_delta_to_current_output(float previous_total_bend, bool force = false) {
            if (!midi_notes_enabled || calibration_mode) return;
            float current_total_bend = this->get_pitch_bend_semitones();
            float delta_semitones = current_total_bend - previous_total_bend;
            if (!force && delta_semitones == 0.0f)
                return;

            DataType shifted_voltage = this->constrainDataToRange(
                this->getCurrentDataValue() + ((DataType)delta_semitones / 12.0f)
            );
            #if CV_PITCH_BEND_TRACE
                Serial_printf("CVPITCH %s delta prev=%0.3f now=%0.3f d=%0.3f -> %0.4fV\n",
                    this->label,
                    previous_total_bend,
                    current_total_bend,
                    delta_semitones,
                    (float)shifted_voltage);
            #endif
            if (this->getCurrentDataValue() != shifted_voltage) {
                this->updateValueFromData(shifted_voltage);
            } else if (force) {
                this->setTargetValueFromNormal(this->getCurrentNormalValue(), true);
            }
            this->process_pending();
        }

        virtual void set_channel_pitch_bend_semitones(float semitones, bool force = false) {
            if (!midi_notes_enabled || calibration_mode) return;
            float previous_total_bend = this->get_pitch_bend_semitones();
            if (!force && this->channel_pitch_bend_semitones == semitones)
                return;

            this->channel_pitch_bend_semitones = semitones;
            #if CV_PITCH_BEND_TRACE
                Serial_printf("CVPITCH %s set channel bend=%0.3f\n", this->label, semitones);
            #endif
            if (is_valid_note(this->current_pitch_note)) {
                this->refresh_pitch_output(true);
            } else {
                this->apply_pitch_bend_delta_to_current_output(previous_total_bend, force);
            }
        }

        virtual void set_omni_pitch_bend_semitones(float semitones, bool force = false) {
            if (!midi_notes_enabled || calibration_mode) return;
            float previous_total_bend = this->get_pitch_bend_semitones();
            if (!force && this->omni_pitch_bend_semitones == semitones)
                return;

            this->omni_pitch_bend_semitones = semitones;
            #if CV_PITCH_BEND_TRACE
                Serial_printf("CVPITCH %s set omni bend=%0.3f\n", this->label, semitones);
            #endif
            if (is_valid_note(this->current_pitch_note)) {
                this->refresh_pitch_output(true);
            } else {
                this->apply_pitch_bend_delta_to_current_output(previous_total_bend, force);
            }
        }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           

        bool is_pending_value = false;
        uint16_t pending_value = 0; 
        uint16_t last_value = 0;

        virtual void setTargetValueFromData(DataType value, bool force = false) override {
            if (this->target!=nullptr) {
                if (this->debug) Serial_printf("CVParameter#setTargetValueFromData(%3.3f, %i)\n", value, this->dac_channel);

                uint16_t calibrated_dac_value = get_dac_value_for_voltage(value);


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

        void write_calibration_dac_value(uint16_t raw) {
            if (this->target == nullptr) return;
            this->is_pending_value = false;
            this->target->write(this->dac_channel, raw);
            const float logical = this->inverted ? __UINT16_MAX__ - raw : raw;
            const float span = (float)calibrated_highest_value - calibrated_lowest_value;
            if (span != 0.0f)
                output_voltage = this->minimumDataLimit +
                    (logical - calibrated_lowest_value) * (this->maximumDataLimit - this->minimumDataLimit) / span;
        }

        void set_calibration_voltage(DataType voltage) {
            this->set_output_locked(true);
            this->write_calibration_dac_value(this->get_dac_value_for_voltage(voltage));
            output_voltage = this->constrainDataToLimit(voltage);
        }

        // called after all parameter modulation processing has been done so that reading and writing don't get entangled and cause problems
        virtual void process_pending() override {
            if (calibration_mode) {
                is_pending_value = false;  // discard any modulation-driven write
                return;
            }
            if (is_pending_value) {
                if (this->debug) Serial_printf("%u\t: %s#setTargetValueFromData(%u) on channel %u!\n", micros(), this->label, pending_value, dac_channel);
                this->write_calibration_dac_value(pending_value);
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
                    Serial_printf("%u\t: %s#reading back %u (wrote %u)\n", micros(), this->label, this->target->read(dac_channel), pending_value);
                }
                is_pending_value = false;
            }
        }

        #ifdef ENABLE_STORAGE
            virtual void setup_saveable_settings() override {
                DataParameter<TargetClass,DataType>::setup_saveable_settings();

                this->register_setting(new LSaveableSetting<uint16_t>(
                    "calibrated_lowest_value",
                    "Calibration",
                    nullptr,
                    [=](uint16_t value) -> void {
                        this->calibrated_lowest_value = value;
                        this->setTargetValueFromData(this->getCurrentDataValue(), true);
                    },
                    [=]() -> uint16_t {
                        return this->calibrated_lowest_value;
                    }
                ), SL_SCOPE_SYSTEM, false);

                this->register_setting(new LSaveableSetting<uint16_t>(
                    "calibrated_highest_value",
                    "Calibration",
                    nullptr,
                    [=](uint16_t value) -> void {
                        this->calibrated_highest_value = value;
                        this->setTargetValueFromData(this->getCurrentDataValue(), true);
                    },
                    [=]() -> uint16_t {
                        return this->calibrated_highest_value;
                    }
                ), SL_SCOPE_SYSTEM, false);

                this->register_setting(new LSaveableSetting<const char*>(
                    "calibration_input",
                    "Calibration",
                    nullptr,
                    [=](const char* value) -> void {
                        this->set_calibration_parameter_input(value);
                    },
                    [=]() -> const char* {
                        return this->calibration_input != nullptr ? this->calibration_input->name : "";
                    }
                ), SL_SCOPE_SYSTEM, false);
            }
        #endif

        //FLASHMEM virtual LinkedList<MenuItem *> *makeControls() override;
        #ifdef ENABLE_SCREEN
        FLASHMEM
        virtual MenuItemList *addCustomTypeControls(MenuItemList *controls) override { 
            if (this->configurable) {
                // TODO: ... calibration settings maybe?
            }

            // calibration settings..
            SubMenuItem *bar1 = new SubMenuItemBar("Settings", true, false);
            // todo: make InputTypeSelectorControl fire a function to switch the ranges+calibration
            InputTypeSelectorControl<> *type_selector = new InputTypeSelectorControl<>("Polarity", &this->polarity);
            bar1->add(type_selector);
            bar1->add(new ToggleControl<bool>("Invert", &this->inverted));

            // insert controls at the top
            controls->add(bar1);

            return controls; 
        };

        virtual MenuItemList *makeCalibrationControls() { 
            MenuItemList *controls = new MenuItemList();

            SubMenuItem *bar2 = new SubMenuItemBar("Calibration", true, false);
            bar2->add(new ParameterInputSelectorControl<CVOutputParameter>(
                "Cal Input", 
                this,
                &CVOutputParameter::set_calibration_parameter_input,
                &CVOutputParameter::get_calibration_parameter_input,
                parameter_manager->get_available_pitch_inputs(),
                this->calibration_input
            ));
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
            input->flags.selectable = false;
            bar2->add(input );               

            SubMenuItem *bar3 = new SubMenuItemBar("Settings", true, false);
            bar3->add(new LambdaNumberControl<DataType>("Output", 
                [=] (DataType value) -> void { 
                    this->set_calibration_voltage(value);
                }, 
                [=] () -> DataType { 
                    return this->output_voltage;
                },
                nullptr,
                this->minimumDataLimit,
                this->maximumDataLimit,
                true, 
                true
            ));
            //if (strcmp(this->label, "CVPO1-A")==0) bar3->items->get(0)->debug = true;

            bar3->add(new LambdaNumberControl<uint16_t>("uni min dac", 
                [=] (uint16_t value) -> void { 
                    this->calibrated_lowest_value = value; 
                    this->set_calibration_voltage(this->output_voltage);
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
                    this->set_calibration_voltage(this->output_voltage);
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

            // Auto (ADC-feedback) and Simple (user-guided) calibration methods side-by-side
            auto *feedback_state = new CVOutputFeedbackCalibState<TargetClass, DataType>(this);
            SubMenuItem *simple_submenu = makeFeedbackCalibrationControls(feedback_state);

            SubMenuItemBar *method_bar = new SubMenuItemBar("Calib Method", false, true);
            method_bar->add(new LambdaActionConfirmItem("Auto", [=](void) -> void {
                parameter_manager_calibrate(this);
            }));
            method_bar->add(simple_submenu);
            controls->add(method_bar);

            return controls;
        }

        virtual MenuItem *makeCalibrationLoadSaveControls() { 
            SubMenuItem *bar4 = new SubMenuItemBar("Calibration", false, false);
            bar4->add(new LambdaActionConfirmItem("Save", [=](void) -> void { 
                this->save_calibration(); 
            }));
            bar4->add(new LambdaActionConfirmItem("Load", [=](void) -> void {
                this->load_calibration(); 
            }));

            return bar4;
        }
        #endif

        virtual float get_voltage_for_pitch(int8_t pitch, int16_t detune_cents = 0) {
            if(!is_valid_note(pitch))
                return 0.0;
            float detune_semitones = ((float)detune_cents / 100.0f) + this->get_pitch_bend_semitones();
            return constrain(((float)pitch + detune_semitones) / 12.0f, this->minimumDataRange, this->maximumDataRange);
        }

        virtual void sendNoteOn(uint8_t pitch, uint8_t velocity, uint8_t channel) {
            if (!midi_notes_enabled || calibration_mode || !is_valid_note(pitch))
                return;

            this->current_pitch_note = pitch;
            this->note_is_active = true;
            float voltage_for_pitch = get_voltage_for_pitch(pitch);
            if (this->debug && Serial) {
                Serial.printf("%s#sendNoteOn(pitch=%3i,....)\t", this->label, pitch);
                Serial.printf("-> %3s,\t", get_note_name_c(pitch));
                Serial.printf("got voltage_for_pitch = %3.3f\n", voltage_for_pitch);
            }
            this->updateValueFromData(voltage_for_pitch);

            if (get_gate_output_enabled() && gate_bank>=0 && gate_manager!=nullptr) {
                gate_is_on = true;
                gate_manager->send_gate_on(gate_bank, gate_number);
            }
        }
        virtual void sendNoteOff(uint8_t pitch, uint8_t velocity, uint8_t channel) {
            if (this->current_pitch_note != pitch) return;
            if (this->current_pitch_note == pitch)
                this->current_pitch_note = NOTE_OFF;
            this->note_is_active = false;

            if ((get_gate_output_enabled() || gate_is_on) && gate_bank>=0 && gate_manager!=nullptr) {
                gate_is_on = false;
                gate_manager->send_gate_off(gate_bank, gate_number);
            }

            if (!park_enabled && !calibration_mode)
                this->updateValueFromData((DataType)0);
        }

        bool is_note_active() const { return note_is_active; }

        void set_midi_notes_enabled(bool enabled) {
            if (enabled) {
                midi_notes_enabled = true;
                return;
            }

            if (is_valid_note(current_pitch_note))
                sendNoteOff(current_pitch_note, 0, 1);

            current_pitch_note = NOTE_OFF;
            note_is_active = false;
            omni_pitch_bend_semitones = channel_pitch_bend_semitones = 0.0f;
            this->updateValueFromData((DataType)0);
            midi_notes_enabled = false;
            this->setTargetValueFromNormal(this->getCurrentNormalValue() + this->get_modulation_value(), true);
            this->process_pending();
        }

        bool park_enabled = true;
        bool get_park_enabled() const { return park_enabled; }
        void set_park_enabled(bool v) {
            park_enabled = v;
            if (!v && !note_is_active && !calibration_mode)
                this->updateValueFromData((DataType)0);
        }

        // Silently re-quantise CV voltage to a parked pitch when idle (no gate, no NoteOn).
        // Ensure that if the output is being used to drive pitch that would be heard, that it at least will be in key.
        // No-op if park is disabled, a note is currently active, or pitch is invalid.
        void park_pitch(int8_t pitch) {
            if (!midi_notes_enabled || calibration_mode) return;
            if (!park_enabled) return;
            if (note_is_active) return;
            if (!is_valid_note(pitch)) return;
            this->updateValueFromData(get_voltage_for_pitch(pitch));
        }

        // No-RTTI cast helper: returns this as CVOutputParameterBase*
        virtual CVOutputParameterBase* as_cv_output_base() override { return this; }

        // --- CVOutputParameterBase implementation (used by oscillator tuning page) ---

        virtual void tuning_send_note(uint8_t pitch) override {
            if (is_valid_note(this->current_pitch_note))
                sendNoteOff((uint8_t)this->current_pitch_note, 0, 1);
            sendNoteOn(pitch, 64, 1);
        }

        virtual void tuning_note_off() override {
            if (is_valid_note(this->current_pitch_note)) {
                sendNoteOff((uint8_t)this->current_pitch_note, 0, 1);
                this->current_pitch_note = NOTE_OFF;
            }
        }

        virtual void tuning_zero_output() override {
            this->updateValueFromData(this->minimumDataRange);
            this->process_pending();
        }

        virtual void tuning_disconnect_modulation() override {
            for (int i = 0; i < MAX_SLOT_CONNECTIONS; i++) {
                this->disconnect_input(i);
            }
        }

        virtual const char *get_cv_label() const override {
            return this->label;
        }

        virtual void tuning_output_voltage(float voltage) override {
            if (this->target != nullptr) {
                const uint16_t raw = this->get_dac_value_for_voltage(voltage);
                this->write_calibration_dac_value(raw);
            }
        }

        virtual float get_minimum_voltage() const override {
            return (float)this->minimumDataRange;
        }

        virtual float get_maximum_voltage() const override {
            return (float)this->maximumDataRange;
        }

        virtual void set_output_locked(bool locked) override {
            if (calibration_mode == locked) return;
            this->calibration_mode = locked;
            this->is_pending_value = false;
            if (!locked) {
                this->setTargetValueFromNormal(this->getCurrentNormalValue() + this->get_modulation_value(), true);
                this->process_pending();
            }
        }

        virtual bool is_output_locked() const override {
            return this->calibration_mode;
        }

        // -----------------------------------------------------------------------

        virtual void set_gate_outputter(IGateTarget *gate_manager, int8_t gate_bank, int8_t gate_number) {
            this->gate_manager = gate_manager;
            this->gate_bank = gate_bank;
            this->gate_number = gate_number;
            this->gate_is_on = false;
        }
        virtual bool get_gate_output_enabled() {
            return this->gate_output_enabled;
        }
        virtual void set_gate_output_enabled(bool enabled) {
            if (!enabled && this->gate_is_on) {
                // turn off gate if currently on
                if (gate_bank>=0 && gate_manager!=nullptr) {
                    gate_is_on = false;
                    gate_manager->send_gate_off(gate_bank, gate_number);
                }
            }
            this->gate_output_enabled = enabled;
        }

        virtual bool needs_calibration() override {
            return true;
        }

        virtual void output_calibration_data () override {
            Serial.printf("CVOutputParameter calibration data for %s: calibrated_lowest_value=%u : calibrated_highest_value=%u\n", this->label, this->calibrated_lowest_value, this->calibrated_highest_value);
            Serial.printf("calibrated_lowest_value=%u\n", this->calibrated_lowest_value);
            Serial.printf("calibrated_highest_value=%u\n", this->calibrated_highest_value);
        }

        // TEMPORARY LEGACY FALLBACK
        // Remove this once migration from calibration_cvoutput_*.txt is no longer needed.
        bool load_legacy_calibration_file() {
            File myFile;

            #ifdef ENABLE_SD
                if (!STORAGE.mediaPresent()) {
                    Serial_println("No SD card found!");
                    return false;
                }
            #endif

            char filename[255] = "";
            snprintf(filename, 255, FILEPATH_CVOUTPUT_CALIBRATION_FORMAT, this->label);
            Debug_printf("\tLEGACY FALLBACK: opening '%s' for global slot %s\n", filename, this->label);
            myFile.setTimeout(0);
            myFile = STORAGE.open(filename, FILE_READ_MODE);

            if (!myFile) {
                Serial_printf("\tLEGACY FALLBACK: couldn't open '%s' for %s\n", filename, this->label);
                return false;
            }

            String line;
            while (myFile.available()) {
                line = myFile.readStringUntil('\n');
                String key = line.substring(0, line.indexOf("="));
                String value = line.substring(line.indexOf("=") + 1);
                if (key.equals("calibrated_lowest_value")) {
                    this->calibrated_lowest_value = value.toInt();
                } else if (key.equals("calibrated_highest_value")) {
                    this->calibrated_highest_value = value.toInt();
                }
            }
            myFile.close();

            this->setTargetValueFromData(this->getCurrentDataValue(), true);
            Debug_printf("LEGACY FALLBACK: for %s, got calibration values %u : %u\n", this->label, this->calibrated_lowest_value, this->calibrated_highest_value);
            return true;
        }

        virtual void save_calibration() override {
            if (parameter_manager != nullptr && parameter_manager->save_system_settings_callback != nullptr) {
                parameter_manager->save_system_settings_callback();
            } else if (Serial) {
                Serial.println("CVOutputParameter::save_calibration: system settings callback not configured");
            }
        }
        virtual void load_calibration() override {
            if (parameter_manager != nullptr && parameter_manager->load_system_settings_callback != nullptr) {
                if (!parameter_manager->load_system_settings_callback()) {
                    load_legacy_calibration_file();
                }
            } else if (Serial) {
                Serial.println("CVOutputParameter::load_calibration: system settings callback not configured");
            }
        }
};

// Full definition of feedback calibration controls (forward-declared above)
#ifdef ENABLE_SCREEN
    #include "mymenu_items/CVOutputFeedbackCalibControls.h"
#endif

#endif
