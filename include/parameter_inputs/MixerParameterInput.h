#pragma once

#include "ParameterInput.h"
#include "AnalogParameterInputBase.h"

#include "../voltage_sources/VoltageSource.h"

#include "bpm.h"
#include "midi_helpers.h"

#include "ads.h"

#include <parameters/Parameter.h>

#include "submenuitem_bar.h"
#ifdef ENABLE_SCREEN
    #include "menuitems_lambda_selector.h"
#endif

#if defined(ENABLE_STORAGE) && (defined(ENABLE_SD) || defined(ENABLE_LITTLEFS))
    #ifndef MIXER_PRESET_DIRECTORY
        #define MIXER_PRESET_DIRECTORY "mixer_presets"
    #endif
    #ifndef MIXER_PRESET_PATH_FORMAT
        #define MIXER_PRESET_PATH_FORMAT MIXER_PRESET_DIRECTORY "/preset%d.txt"
    #endif
#endif

class VirtualMixerParameterInput : public AnalogParameterInputBase<float> {
    public:
        float source_value = 0.0f;

        DataParameter<VirtualMixerParameterInput,float> *underlying_parameter = nullptr;

        VirtualMixerParameterInput(char *name, const char *group_name) : AnalogParameterInputBase(name, group_name) {
            underlying_parameter = 
                new DataParameter<VirtualMixerParameterInput,float>(
                    name, 
                    this, 
                    &VirtualMixerParameterInput::set_source_value,
                    &VirtualMixerParameterInput::get_source_value,
                    -1.0f, 
                    1.0f
                );
            #ifdef ENABLE_STORAGE
                underlying_parameter->set_path_segment("Parameter");
            #endif
            parameter_manager->addParameter(underlying_parameter);
        }

        VirtualMixerParameterInput *as_virtual_mixer() override { return this; }

        bool copy_from(const VirtualMixerParameterInput *source) {
            if (!source || source == this || !source->underlying_parameter || !underlying_parameter) return false;
            #ifdef ENABLE_PARAMETERS_MIDI_LEARN
                if (parameter_manager && parameter_manager->awaiting_midi_learn == underlying_parameter)
                    parameter_manager->stop_midi_learn();
            #endif
            input_type = source->input_type;
            setInverted(source->inverted);
            underlying_parameter->setRangeMinimumLimit(source->underlying_parameter->getRangeMinimumLimit());
            underlying_parameter->setRangeMaximumLimit(source->underlying_parameter->getRangeMaximumLimit());
            underlying_parameter->updateValueFromData(source->underlying_parameter->getCurrentDataValue());
            for (int slot = 0; slot < MAX_SLOT_CONNECTIONS; ++slot) {
                underlying_parameter->connections[slot] = source->underlying_parameter->connections[slot];
                underlying_parameter->connections[slot].sh_last_sample = 0.0f;
                underlying_parameter->connections[slot].sh_last_tick = UINT32_MAX;
                #ifdef ENABLE_PARAMETERS_MIDI_LEARN
                    underlying_parameter->connections[slot].midi_learn_active = false;
                #endif
            }
            return true;
        }

        #if defined(ENABLE_STORAGE) && (defined(ENABLE_SD) || defined(ENABLE_LITTLEFS))
            bool preset_path(int preset, char *path, size_t size) const {
                if (preset < 0 || preset >= 8 || !path || size == 0) return false;
                const int written = snprintf(path, size, MIXER_PRESET_PATH_FORMAT, preset);
                return written >= 0 && (size_t)written < size;
            }

            bool preset_exists(int preset) const {
                char path[128];
                if (!preset_path(preset, path, sizeof(path))) return false;
                #if defined(ENABLE_SD)
                    return SD.exists(path);
                #else
                    return LittleFS.exists(path);
                #endif
            }

            bool save_preset(int preset) {
                char path[128];
                if (!preset_path(preset, path, sizeof(path))) return false;
                #if defined(ENABLE_SD)
                    if (!SD.exists(MIXER_PRESET_DIRECTORY) && !SD.mkdir(MIXER_PRESET_DIRECTORY)) return false;
                    if (SD.exists(path) && !SD.remove(path)) return false;
                #else
                    if (!LittleFS.exists(MIXER_PRESET_DIRECTORY) && !LittleFS.mkdir(MIXER_PRESET_DIRECTORY)) return false;
                    if (LittleFS.exists(path) && !LittleFS.remove(path)) return false;
                #endif
                return sl_save_to_file(this, path, SL_SCOPE_SCENE);
            }

            bool load_preset(int preset) {
                char path[128];
                return preset_path(preset, path, sizeof(path)) && sl_load_from_file_into(this, path, SL_SCOPE_SCENE);
            }
        #endif

        virtual void set_source_value(float v) {
            source_value = v;
        }
        virtual float get_source_value() {
            return source_value;
        }

        virtual void read() override {
            //float currentValue = this->voltage_source->get_voltage_normal();
            this->lastValue = this->currentValue;
            this->currentValue = source_value;
        }

        #ifdef ENABLE_STORAGE
            void setup_saveable_settings() override {
                AnalogParameterInputBase<float>::setup_saveable_settings();
                this->register_child(underlying_parameter);
            }
        #endif

        #ifdef ENABLE_SCREEN
            int selected_copy_input = -1;

            #if defined(ENABLE_STORAGE) && (defined(ENABLE_SD) || defined(ENABLE_LITTLEFS))
                int selected_preset = 0;

                class PresetSelector : public LambdaNumberControl<int> {
                    VirtualMixerParameterInput *mixer;
                    const char *format_preset(int value) {
                        static char label[16];
                            snprintf(label, sizeof(label), "%d%s", value, mixer->preset_exists(value) ? " *" : "");
                        return label;
                    }
                public:
                    PresetSelector(VirtualMixerParameterInput *target)
                        : LambdaNumberControl<int>("Preset", [target](int value) { target->selected_preset = value; },
                            [target]() { return target->selected_preset; }, nullptr, 0, 7, true), mixer(target) {}
                    const char *getFormattedValue() override { return format_preset(mixer->selected_preset); }
                    const char *getFormattedInternalValue() override { return format_preset(this->get_internal_value()); }
                };
            #endif

            virtual SubMenuItemBar *makeControls(const char *label_prefix = "") override { 
                SubMenuItemBar *a = AnalogParameterInputBase::makeControls();
                for (int index = 0; index < parameter_manager->available_inputs->size(); ++index) {
                    BaseParameterInput *input = parameter_manager->available_inputs->get(index);
                    if (input && input != this && input->as_virtual_mixer()) {
                        selected_copy_input = index;
                        break;
                    }
                }
                SubMenuItemBar *utility = nullptr;
                #if defined(ENABLE_STORAGE) && (defined(ENABLE_SD) || defined(ENABLE_LITTLEFS))
                    char utility_label[40];
                    snprintf(utility_label, sizeof(utility_label), "%s presets", get_name());
                    utility = new SubMenuItemBar(utility_label, false, true);
                    utility->add(new PresetSelector(this));
                    utility->add(new LambdaActionConfirmItem("Load", [this]() {
                        const bool ok = load_preset(selected_preset);
                        menu_set_last_message(ok ? "Mixer preset loaded" : "Mixer preset load failed", ok ? GREEN : RED);
                    }, true));
                    utility->add(new LambdaActionConfirmItem("Save", [this]() {
                        const bool ok = save_preset(selected_preset);
                        menu_set_last_message(ok ? "Mixer preset saved" : "Mixer preset save failed", ok ? GREEN : RED);
                    }, true));
                #endif
                if (selected_copy_input >= 0) {
                    if (utility == nullptr) {
                        char copy_label[40];
                        snprintf(copy_label, sizeof(copy_label), "%s copy", get_name());
                        utility = new SubMenuItemBar(copy_label, false, true);
                    }
                    auto *selector = new LambdaSelectorControl<int>("From",
                        [this](int value) { selected_copy_input = value; },
                        [this]() { return selected_copy_input; }, nullptr, true);
                    for (int index = 0; index < parameter_manager->available_inputs->size(); ++index) {
                        BaseParameterInput *input = parameter_manager->available_inputs->get(index);
                        if (input && input != this && input->as_virtual_mixer())
                            selector->add_available_value(index, input->get_name());
                    }
                    utility->add(selector);
                    utility->add(new LambdaActionConfirmItem("Copy", [this]() {
                        BaseParameterInput *input = parameter_manager->available_inputs->get(selected_copy_input);
                        const bool ok = input && copy_from(input->as_virtual_mixer());
                        menu_set_last_message(ok ? "Mixer copied" : "Mixer copy failed", ok ? GREEN : RED);
                    }, true));
                }
                if (utility != nullptr) menu->add(utility);
                MenuItemList *p_controls = underlying_parameter->makeControls();
                menu->add(p_controls);
                return a;
            }
        #endif

};