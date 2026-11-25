#pragma once

#include "shared_debug.h"
#include "Arduino.h"

#include "menu.h"
#include "colours.h"
#include "menuitems_popout.h"

#include "../parameters/Parameter.h"

#include <LinkedList.h>
#include <new>

// direct control over a Parameter Value from menu
class ParameterValueMenuItem : public DirectNumberControl<float> {
    public:
        FloatParameter **parameter = nullptr;

        bool show_output_mode = false;  // true if this widget should show the last post-modulation output value; false if it should show the pre-modulation value

        virtual char get_float_unit() override {
            if (this->parameter==nullptr || *parameter==nullptr) {
                return this->float_unit;
            } else {
                return (*parameter)->float_unit;
            }
        }

        ParameterValueMenuItem(char *label, FloatParameter **parameter) : DirectNumberControl(label) {
            strncpy(this->label, label, 20);
            this->parameter = parameter;
            if (*parameter!=nullptr)
                //this->internal_value = (*parameter)->getCurrentNormalValue() * 100.0;
                this->internal_value = (*parameter)->getLastModulatedNormalValue() * 100.0;
            this->minimumDataValue = 0.0f; 
            this->maximumDataValue = 1.0f; 

            //this->debug = true;

            flags.go_back_on_select = true;
            //this->minimumDataValue = parameter->minimumDataValue;
            //this->maximumDataValue = parameter->maximumDataValue;
            //this->step = 0.01;
        }

        #if MENU_PERF_PARTIAL_UPDATES
            // used by check_needs_redraw_custom(), compared with last time it was called to determine if we need to redraw the menu item
            virtual float get_test_value(bool selected, bool opened) override {
                // if opened, we need to use the internal value (which is what the user is currently changing);
                if (opened) {
                    return this->get_internal_value();
                } 

                // if not, then according to show_output_mode, we either show the parameter's last output value (post-modulation) or the current value (pre-modulation)
                if (this->show_output_mode) {
                    return (*parameter)->getLastOutputNormalValue();
                }
                return (*parameter)->getCurrentNormalValue();
            }
        #endif

        // // true if this widget should show the last post-modulation output value; false if it should show the pre-modulation value
        virtual ParameterValueMenuItem *set_show_output_mode(bool mode = true) {
            this->show_output_mode = mode;
            this->readOnly = true;
            return this;
        }

        virtual bool action_opened() override {
            // Serial.printf("ParameterValueMenuItem#action_opened in %s ", this->label);
            // Serial.printf("internal_value is %3.3f but get_current_value() is %3.3f\n", this->internal_value, (*this->parameter)->currentNormalValue);

            // update control's internal value from the parameter's current value when we open the menu, 
            // so that it shows the current value and not the last value used by this control 
            // (which may be different if the control was last pointing at a different parameter)
            // previously did battle with this but gave up trying to solve -- i think maybe because of an
            // issue with CV Output parameters?
            this->internal_value = (*parameter)->getCurrentNormalValue();
            return true;
        }

        // normalised integer (0-100)
        virtual float get_current_value() override {
            if (this->parameter==nullptr || *parameter==nullptr)
                return 0;
            /*if (this->debug) {
                Serial.printf("ParameterValueMenuItem for %s (parameter %s) has currentValue ", this->label, this->parameter->label);
                Serial.println(parameter->getCurrentValue());
            }*/
            //return (int) (parameter->getCurrentNormalValue() * 100.0); //(float)this->maximumDataValue);    // turn into percentage
            return (*parameter)->getCurrentNormalValue();
        }

        virtual const char *getFormattedValue() override {
            static char fmt[20] = "";
            if (this->show_output_mode) {
                return this->getFormattedOutputValue();
            }
            snprintf(fmt, 20, "%3s", (*parameter)->getFormattedValue()); 
            return fmt;
        }
        virtual const char *getFormattedOutputValue() {
            static char fmt[20] = "";
            //snprintf(fmt, 20, "%s", this->parameter->getFormattedLastOutputValue());
            snprintf(fmt, 20, "%s", (*parameter)->getFormattedLastOutputValue());
            if (this->debug) Serial_printf("%s#getFormattedOutputValue() returning\t'%s'\t\t(in ParameterValueMenuItem)\n", this->label, fmt);
            return fmt;
        }

        virtual const char *getFormattedInternalValue() override {
            return (*parameter)->getFormattedValue(this->get_internal_value()); //->internal_value);
        }

        virtual const char *getFormattedExtra() override {
            if ((*parameter)->is_modulatable())
                return (*parameter)->getFormattedValue((*parameter)->getLastModulatedNormalValue());
            return nullptr;
        }

        virtual void set_current_value(float value) override { 
            //if (this->debug) { Serial.printf(F("ParameterValueMenuItem#set_current_value(%f) on %s\n"), value, this->label); Serial_flush(); }

            if (parameter==nullptr || *parameter==nullptr)
                return;
            if (this->readOnly)
                return;
           
            if (*parameter!=nullptr) {
                /*if (this->debug) {
                    Serial.printf(F("\tParameterMenuItem#set_current_value(%f): Calling setParamValue %f (max value %i) on Parameter %s\n"), value, value, this->maximumDataValue, this->parameter->label); Serial_flush();
                }*/
                float v = value;
                /*if (this->debug) {
                    Serial.print(F("ParameterValueMenuItem#set_current_value() got v to pass: "));                    
                    Serial.println(v);
                }*/
                //if (this->debug) Serial.printf(F("ParameterValueMenuItem#set_current_value(%f) about to call updateValueFromNormal(%f) (maximumDataValue is %i)\n"), value, v, this->maximumDataValue);
                (*parameter)->updateValueFromNormal(v);
            } 
        }

        // directly increase the parameter's value
        virtual void increase_value() override {
            //this->debug = true; (*parameter)->debug = true;
            (*parameter)->incrementValue();
            this->internal_value = (*parameter)->getCurrentNormalValue(); //this->maximumDataValue;
            if (this->debug) Serial_printf((const char*)F("ParameterValueMenuItem#increase_value updated internal_value to %f (from %f * 100.0)\n"), internal_value, (*parameter)->getCurrentNormalValue());
            //this->debug = false; (*parameter)->debug = false;
        }
        // directly decrease the parameter's value
        virtual void decrease_value() override {
            //this->debug = true; (*parameter)->debug = true;

            (*parameter)->decrementValue();
            this->internal_value = (*parameter)->getCurrentNormalValue(); // * 100.0; //this->maximumDataValue;
            if (this->debug) Serial_printf((const char*)F("ParameterValueMenuItem#decrease_value updated internal_value to %f (from %f * 100.0)\n"), internal_value, (*parameter)->getCurrentNormalValue());
            //this->debug = false; (*parameter)->debug = false;
        }

        virtual void change_value(int new_value) { //override { //
            float f = (float)new_value / 100.0;
            //if (this->debug) Serial.printf(F("ParameterValueMenuItem#change_value(%i) about to call change_value(%f)\n"), new_value, new_value);
            this->change_value(f);
        }

        virtual void change_value(float new_value) {    // doesn't override, implements for normalled float?
            if (readOnly) return;
            float last_value = this->get_current_value();
            //if (this->debug) Serial.printf(F("ParameterValueMenuItem#change_value(%f)\t in %s\tabout to call set_current_value(%f)\n"), new_value, this->label);
            this->set_current_value(new_value);
            //if (this->debug) Serial.printf(F("ParameterValueMenuItem#change_value(%f)\t after set_current_value(%f) get_current_value is \n"), new_value, this->get_current_value());
            if (on_change_handler!=nullptr) {
                //if (this->debug)  { Serial.println(F("NumberControl calling on_change_handler")); Serial_flush(); }
                on_change_handler(last_value, this->internal_value); //this->get_internal_value());
                //if (this->debug)  { Serial.println(F("NumberControl after on_change_handler")); Serial_flush(); }
            }
        }
};

class ParameterOutputMenuItem : public ParameterValueMenuItem {
    uint8_t *history = nullptr;
    uint16_t capacity = 0;
    uint16_t head = 0;
    uint16_t count = 0;
    enum DisplayMode : uint8_t { VALUE_ONLY, GRAPH_ONLY, VALUE_AND_GRAPH };
    DisplayMode display_mode = VALUE_AND_GRAPH;
    FloatParameter *sampled_parameter = nullptr;

public:
    ParameterOutputMenuItem(FloatParameter **parameter) : ParameterValueMenuItem((char *)"Output", parameter) {
        set_show_output_mode();
    }

    ~ParameterOutputMenuItem() { delete[] history; }

    bool action_opened() override {
        display_mode = (DisplayMode)((display_mode + 1) % 3);
        if (display_mode == VALUE_ONLY) on_hidden();
        IF_MENU_PERF_PARTIAL_UPDATES(this->request_redraw();)
        return false;
    }

    void on_hidden() override {
        delete[] history;
        history = nullptr;
        capacity = head = count = 0;
        sampled_parameter = nullptr;
    }

    void update_ticks(unsigned long) override {
        if (display_mode == VALUE_ONLY || history == nullptr || parameter == nullptr || *parameter == nullptr)
            return;
        if (sampled_parameter != *parameter) {
            sampled_parameter = *parameter;
            head = count = 0;
        }
        float value = sampled_parameter->getLastOutputNormalValue();
        if (!(value >= 0.0f)) value = 0.0f;
        if (value > 1.0f) value = 1.0f;
        history[head] = (uint8_t)(value * 255.0f + 0.5f);
        head = (head + 1) % capacity;
        if (count < capacity) ++count;
        IF_MENU_PERF_PARTIAL_UPDATES(this->request_redraw();)
    }

    int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        if (display_mode == VALUE_ONLY) return ParameterValueMenuItem::renderValue(selected, opened, max_character_width);
        if (tft->getCursorY() >= tft->height()) {
            on_hidden();
            return tft->getCursorY() + tft->getSingleRowHeight() * 2;
        }
        const uint16_t width = max_character_width * tft->characterWidth();
        if (width == 0) return ParameterValueMenuItem::renderValue(selected, opened, max_character_width);
        const int x = tft->getCursorX();
        const int y = tft->getCursorY();
        const bool overlay_text = display_mode == VALUE_AND_GRAPH;
        const int text_end_y = overlay_text ? ParameterValueMenuItem::renderValue(selected, opened, max_character_width) : y;
        tft->setCursor(x, y);
        if (capacity != width) {
            delete[] history;
            history = new (std::nothrow) uint8_t[width];
            capacity = history != nullptr ? width : 0;
            head = count = 0;
        }
        if (history == nullptr)
            return overlay_text ? text_end_y : ParameterValueMenuItem::renderValue(selected, opened, max_character_width);
        if (sampled_parameter != (parameter != nullptr ? *parameter : nullptr)) {
            sampled_parameter = parameter != nullptr ? *parameter : nullptr;
            head = count = 0;
        }
        const int height = tft->getSingleRowHeight() * 2;
        const uint16_t fg = overlay_text ? (selected ? RED : GREEN) : (selected ? BLACK : C_WHITE);
        if (!overlay_text) tft->fillRect(x, y, width, height, selected ? C_WHITE : BLACK);
        int previous_y = 0;
        for (uint16_t index = 0; index < count; ++index) {
            const uint16_t position = (head + capacity - count + index) % capacity;
            const int plot_x = x + width - count + index;
            const int plot_y = y + height - 1 - (history[position] * (height - 1) / 255);
            if (index > 0) tft->drawLine(plot_x - 1, previous_y, plot_x, plot_y, fg);
            else tft->drawLine(plot_x, plot_y, plot_x, plot_y, fg);
            previous_y = plot_y;
        }
        tft->setCursor(x, y + height);
        return tft->getCursorY();
    }
};

#include "submenuitem_bar.h"
#include "parameter_inputs/ParameterInput.h"
#include "ParameterInputMenuItems.h"
#include "ParameterMenuItems_Range.h"

// Methods and variables shared between modulation slot row controls
class ParameterSlotControlMixins {
    public:
        FloatParameter **parameter = nullptr;
        uint8_t slot_number = 0;

        ParameterSlotControlMixins(FloatParameter **parameter, uint8_t slot_number) 
            : parameter(parameter), slot_number(slot_number) {}

        uint16_t get_colour() const {
            if (
                parameter != nullptr && *parameter != nullptr 
                && (*parameter)->connections[slot_number].parameter_input != nullptr
            )
                return (*parameter)->connections[slot_number].parameter_input->colour;
            return C_WHITE;
        }

        bool is_active() const {
            return 
                parameter != nullptr 
                && *parameter != nullptr 
                && (*parameter)->is_valid_slot(slot_number) 
                #ifdef ENABLE_PARAMETERS_MIDI_LEARN
                    &&(*parameter)->connections[slot_number].midi_learn_active
                #endif
                ;
        }

        float get_slot_amount() {
            if (
                parameter != nullptr && *parameter != nullptr 
                && (*parameter)->is_valid_slot(slot_number)) 
            {
                return (*parameter)->connections[slot_number].amount;
            }
            return 0.0f;
        }

        BaseParameterInput *get_slot_parameter_input() {
            if (parameter != nullptr && *parameter != nullptr && (*parameter)->is_valid_slot(slot_number)) {
                return (*parameter)->connections[slot_number].parameter_input;
            }
            return nullptr;
        }
};


class ParameterMapPercentageControl : public ParameterSlotControlMixins, public DirectNumberControl<float> {

    public:
    ParameterMapPercentageControl(char *label, FloatParameter **parameter, int slot_number) :
        DirectNumberControl<float> (
            label, 
            &((*parameter)->connections[slot_number].amount), 
            (*parameter)->connections[slot_number].amount,
            -2.0f,
            2.0f,
            nullptr
        ), ParameterSlotControlMixins(parameter, slot_number)
        {
            // if ((*parameter)->connections[slot_number].parameter_input!=nullptr) {
            //     this->default_fg = (*parameter)->connections[slot_number].parameter_input->colour;
            // }
            this->default_fg = this->get_colour();
        }

    virtual int header(const char *text, Coord pos, bool selected = false, bool opened = false, int textSize = 0, unsigned int text_len = (unsigned int)-1) override {
        if ((*parameter)->connections[slot_number].parameter_input!=nullptr) {
            return DirectNumberControl::header((*parameter)->connections[slot_number].parameter_input->name, pos, selected, opened, textSize, text_len);
        } else {
            return DirectNumberControl::header(text, pos, selected, opened, textSize, text_len);
        }
    }

    virtual const char *get_label() {
        return (*parameter)->connections[slot_number].parameter_input!=nullptr ? 
               (*parameter)->connections[slot_number].parameter_input->name : 
               (char*)"None";
    }
       
    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        //Serial.printf("%s: renderValue has parameter=%p and *parameter=%p\n", get_label(), this->parameter, *this->parameter);
        // update the control with the colour from the connected parameter input, if there is one
        this->default_fg = this->get_colour();

        if (opened) this->default_fg = GREEN;
        if (!(*parameter)->is_modulation_slot_active(slot_number))
            this->default_fg = tft->halfbright_565(this->default_fg);
            
        //Serial.printf("renderValue in ParameterMapPercentageControl\tfor %s,\tgot default_fg colour %04x from slot_number %i colour %04x, max_character_width=%i\n", this->label, this->default_fg, slot_number, this->get_colour(), max_character_width);
        return DirectNumberControl::renderValue(selected, opened, max_character_width);
    }

    virtual float get_current_value() override {
        return (*parameter)->connections[slot_number].amount;
    }

    virtual void set_current_value(float value) override {
        (*parameter)->connections[slot_number].amount = value;
    }

    float previously_switched_value = 0.0f;

    virtual bool button_right() override {
        this->button_right_unopened();
        return true; // indicate that the button press was handled
    }

    virtual bool button_right_unopened() override {
        // cycle through values 0% -> 100% -> -100% -> 0%
        if (this->get_current_value() == 0.0f) {
            this->set_current_value(1.0f); // example action: set value to 1 if it was 0
            this->internal_value = 1.0f;
            return true; // indicate that the button press was handled
        } else if (this->get_current_value() == 1.0f) {
            this->set_current_value(-1.0f); // example action: set value to 0 if it was 1
            this->internal_value = -1.0f;
            return true; // indicate that the button press was handled
        } else if (this->get_current_value() == -1.0f) {
            this->set_current_value(0.0f); // example action: set value to 0 if it was -1
            this->internal_value = 0.0f;
            return true; // indicate that the button press was handled
        }

        float temp = this->get_current_value();
        this->set_current_value(previously_switched_value);
        this->internal_value = previously_switched_value;
        previously_switched_value = temp;
        return false; // indicate that the button press was not handled
    }

};


// ParameterMenuItem is defined below, after ParameterConnectionPolarityTypeSelectorControl and ParameterModSlotRow



// widget for editing the polarity of a parameter input's connection to parameter mixer
class ParameterConnectionPolarityTypeSelectorControl : 
    public ParameterSlotControlMixins, 
    public SelectorControl<int_least8_t> 
{
    public:

    ParameterConnectionPolarityTypeSelectorControl(
        const char *label, 
        FloatParameter **parameter, 
        byte slot_number
    ) : ParameterSlotControlMixins(parameter, slot_number), SelectorControl(label) 
    { 
        this->actual_value_index = (*this->ParameterSlotControlMixins::parameter)->connections[slot_number].polar_mode;
        this->flags.go_back_on_select = true;
    }

    virtual const char* get_label_for_value(int_least8_t index) override {
        if (index==MOD_SLOT_BI_NATIVE)
            return MOD_SLOT_BI_NATIVE_LABEL;
        if (index==MOD_SLOT_UNI_RAW)
            return MOD_SLOT_UNI_RAW_LABEL;
        if (index==MOD_SLOT_UNI_CENTERED)
            return MOD_SLOT_UNI_CENTERED_LABEL;
        //if (index==CLOCK_NONE)
        //    return "None";
        return "??";
    }

    virtual void setter (int_least8_t new_value) {
        if (parameter != nullptr)
            (*parameter)->connections[slot_number].polar_mode = new_value;
        actual_value_index = new_value;
    }
    virtual int_least8_t getter () {
        //return clock_mode; //selected_value_index;
        if (parameter != nullptr)
            return (*parameter)->connections[slot_number].polar_mode;
        return MOD_SLOT_UNI_RAW;
    }

    virtual int renderValue(bool selected, bool opened, uint16_t width) override {
        this->default_fg = this->get_colour();
        return SelectorControl::renderValue(selected, opened, width);
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        //Serial.println("MidiOutputSelectorControl display()!");
        //int textSize = tft->get_textsize_for_width(label, tft->width()/2);
        int_fast8_t textSize = 0;
        pos.y = header(label, pos, selected, opened, textSize);

        num_values = 3;

        tft->setCursor(pos.x, pos.y);

        if (!opened) {
            // not opened, so just show the current value
            colours(selected, this->default_fg, BLACK);

            tft->printf((char*)"%s", (char*)get_label_for_value(this->getter())); //selected_value_index));
            tft->println((char*)"");
        } else {
            const int current_value = this->getter(); 

            for (int i = 0 ; i < num_values ; i++) {
                bool is_current_value_selected = (int)i==current_value;
                const int_fast16_t col = is_current_value_selected ? GREEN : this->default_fg;
                colours(selected_value_index==i, col, BLACK);
                //colours((selected /*&& !opened*/) || (opened && selected_value_index==(int)i), col, BLACK);
                tft->setCursor(pos.x, pos.y);
                const char *label = get_label_for_value(i);
                tft->setTextSize(tft->get_textsize_for_width(label, tft->width()/2));
                tft->printf((char*)"%s\n", (char*)label);
                pos.y = tft->getCursorY();
            }
            if (tft->getCursorX()>0) // if we haven't wrapped onto next line then do it manually
                tft->println((char*)"");
        }
        return tft->getCursorY();
    }

    virtual bool action_opened() override {
        //if (this->debug) Serial.printf(F("ObjectToggleControl#action_opened on %s\n"), this->label);
        const int_least8_t current = this->getter();
        const int_least8_t value = (int_least8_t)((current + 1) % 3);
        this->setter(value);
        return false;   // don't 'open'
    }

    virtual bool button_select() override {
        this->selected_value_index = (this->selected_value_index + 1) % 3;
        this->setter(selected_value_index);

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        snprintf(msg, MENU_MESSAGE_MAX, "Set polarity to %i: %s", selected_value_index, get_label_for_value(selected_value_index));
        menu_set_last_message(msg, GREEN);

        return flags.go_back_on_select;
    }

};

#ifdef ENABLE_PARAMETERS_MIDI_LEARN
    // One-character per-slot MIDI Learn control that goes into 'AWAITING MIDI INPUT' mode (with a popout overlay?) when selected.
    class ParameterModSlotMidiLearnControl : public ParameterSlotControlMixins, public MenuItem {
        bool last_rendered_active = false;

    public:
        ParameterModSlotMidiLearnControl(const char *label, FloatParameter **parameter, byte slot_number)
            : ParameterSlotControlMixins(parameter, slot_number), MenuItem(label) {
            this->flags.go_back_on_select = true;
            #if MENU_PERF_PARTIAL_UPDATES
                this->add_redraw_policy(REDRAW_ON_CUSTOM);
            #endif
        }

        virtual int renderValue(bool selected, bool, uint16_t) override {
            const bool active = is_active();
            last_rendered_active = active;
            // todo: would be cool to pulse this indicator when active
            const uint16_t col = active ? default_fg : (uint16_t)(C_WHITE >> 1);
            colours(selected, col, BLACK);
            tft->setTextSize(2);
            tft->print(active ? "?" : "L");
            return tft->getCursorY();
        }

        virtual bool action_opened() override {
            if (parameter_manager != nullptr && parameter != nullptr && *parameter != nullptr) {
                if (is_active())
                    parameter_manager->stop_midi_learn(*parameter, slot_number);
                else
                    parameter_manager->start_midi_learn(*parameter, slot_number);
            }
            return false;   // don't 'open' the menu item itself
        }

        // todo: we might want to display some kind of fullscreen overlay kinda thing while
        // waiting for a MIDI learn candidate to be proposed
        #if MENU_PERF_PARTIAL_UPDATES
            virtual bool check_needs_redraw_custom(bool, bool) override {
                return is_active() != last_rendered_active;
            }
        #endif

        virtual bool wants_fullscreen_overlay_when_opened_in_bar() override { return false; }
    };
#endif


// One-character per-slot S&H mode indicator that opens a popout overlay when selected.
// Cycles through SHMode values with left/right/select; renders "~" (off) or "H" (active).
class ParameterModSlotSHControl : public ParameterSlotControlMixins, public MenuItem {
    char overlay_value_buf[8];

    SHMode get_sh_mode() const {
        if (parameter == nullptr || *parameter == nullptr) return SH_OFF;
        return (*parameter)->connections[slot_number].sh_mode;
    }
    void set_sh_mode(SHMode m) {
        if (parameter == nullptr || *parameter == nullptr) return;
        (*parameter)->connections[slot_number].sh_mode = m;
    }

public:
    ParameterModSlotSHControl(const char *label, FloatParameter **parameter, byte slot_number)
        : ParameterSlotControlMixins(parameter, slot_number), MenuItem(label) {
        this->flags.go_back_on_select = true;
        overlay_value_buf[0] = '\0';
    }

    virtual int renderValue(bool selected, bool opened, uint16_t width) override {
        const SHMode m = get_sh_mode();
        const bool active = (m != SH_OFF);
        const uint16_t col = active ? get_colour() : (uint16_t)(C_WHITE >> 1);
        colours(selected, col, BLACK);
        if (!active) {
            // Off: single prominent '~' at textsize 1
            tft->setTextSize(2);
            tft->print("~");
        } else {
            // On: 'H' at textsize 1, then 2-char mode abbrev at textsize 0 below
            const int16_t start_x = tft->getCursorX();
            tft->setTextSize(1);
            tft->println("H");
            tft->setCursor(start_x, tft->getCursorY());
            tft->setTextSize(0);
            colours(selected, col, BLACK);
            tft->println(get_sh_mode_short_label(m));
        }
        return tft->getCursorY();
    }

    virtual bool wants_fullscreen_overlay_when_opened_in_bar() override { return true; }
    virtual MenuItem_RedrawPolicy get_overlay_redraw_policy() const override {
        // This overlay can draw a live graph (when a ParameterInput with history is connected),
        // so always request tick-based redraws to keep the graph current.
        return REDRAW_ON_TICK;
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        if (!opened) {
            renderValue(selected, false, 0);
            return tft->getCursorY();
        }
        // Build and draw popout overlay
        const SHMode m = get_sh_mode();
        strncpy(overlay_value_buf, get_sh_mode_label(m), sizeof(overlay_value_buf) - 1);
        overlay_value_buf[sizeof(overlay_value_buf) - 1] = '\0';

        const char *src_name = nullptr;
        if (
            parameter != nullptr && *parameter != nullptr 
            && (*parameter)->connections[slot_number].parameter_input != nullptr
        )
            src_name = (*parameter)->connections[slot_number].parameter_input->name;

        SelectorTakeoverOverlaySpec overlay;
        overlay.title      = "S&H";
        overlay.subtitle   = src_name;
        overlay.value      = overlay_value_buf;
        overlay.left_hint  = "<";
        overlay.right_hint = ">";
        overlay.frame_colour = (m != SH_OFF) ? get_colour() : C_WHITE;
        overlay.value_fg   = C_WHITE;

        // If the connected input has a history display, add a graph with S&H reconstruction
        BaseParameterInput *inp = this->get_slot_parameter_input();
        ParameterInputDisplay *disp_for_graph = nullptr;
        if (inp != nullptr) disp_for_graph = inp->parameter_input_display;

        if (disp_for_graph != nullptr) {
            overlay.has_extra = true;
            overlay.extra_height = 50;
            overlay.draw_extra_fn = [](void *userdata, DisplayTranslator *tft, int x, int y, int max_width, int max_height) {
                auto *self = static_cast<ParameterModSlotSHControl*>(userdata);
                FloatParameter *param = (self->parameter != nullptr) ? *(self->parameter) : nullptr;
                if (param == nullptr) return;
                BaseParameterInput *inp = self->get_slot_parameter_input();
                if (inp == nullptr) return;
                ParameterInputDisplay *disp = inp->parameter_input_display;
                if (disp == nullptr) return;

                // Raw input signal (past=bright, future=halfbright split at current tick)
                disp->draw_graph(x, y, max_width, max_height);

                // S&H stepped reconstruction in white, derived from the same buffer
                const SHMode sh_m = param->connections[self->slot_number].sh_mode;
                if (sh_m == SH_OFF) return;
                const uint32_t sh_period = get_sh_ticks_for_mode(sh_m);
                if (sh_period == 0 || disp->memory_size == 0 || disp->logged == nullptr) return;

                const uint32_t mem_sz = disp->memory_size;
                const uint32_t cur_buf_idx = disp->ticks_to_memory_step(::ticks);
                const uint16_t sh_col_past   = C_WHITE;
                const uint16_t sh_col_future = tft->halfbright_565(C_WHITE);
                int last_sh_y = -1;
                for (int sx = x; sx < x + max_width; sx++) {
                    // pixel → buffer index
                    const uint32_t buf_idx = (mem_sz == (uint32_t)max_width)
                        ? (uint32_t)(sx - x)
                        : (uint32_t)((uint64_t)(sx - x) * mem_sz / max_width);
                    // buffer index → tick-in-phrase → last S&H boundary → buffer index at boundary
                    const uint32_t tick_ph  = (uint32_t)((uint64_t)buf_idx * TICKS_PER_PHRASE / mem_sz);
                    const uint32_t samp_tk  = (tick_ph / sh_period) * sh_period;
                    const uint32_t samp_bi  = (uint32_t)((uint64_t)samp_tk * mem_sz / TICKS_PER_PHRASE);
                    const float v = disp->decode_memory_log(disp->logged[samp_bi < mem_sz ? samp_bi : mem_sz - 1]);
                    const int gy = max_height - (int)(v * max_height);
                    const uint16_t sh_col = buf_idx < cur_buf_idx ? sh_col_past : sh_col_future;
                    if (sx > x && last_sh_y >= 0)
                        tft->drawLine(sx - 1, y + last_sh_y, sx, y + gy, sh_col);
                    last_sh_y = gy;
                }
            };
            overlay.draw_extra_userdata = this;
        }

        return menu_draw_selector_takeover_overlay(tft, pos, overlay);
    }

    // Open overlay
    virtual bool action_opened() override {
        return true;
    }
    virtual bool button_select() override {
        // set_sh_mode((SHMode)((int(get_sh_mode()) + 1) % int(SH_MODE_COUNT)));
        return flags.go_back_on_select;
    }
    virtual bool knob_left() override {
        const SHMode cur = get_sh_mode();
        set_sh_mode(cur == SH_OFF ? (SHMode)(SH_MODE_COUNT - 1) : (SHMode)(int(cur) - 1));
        return true;
    }
    virtual bool knob_right() override {
        set_sh_mode((SHMode)((int(get_sh_mode()) + 1) % int(SH_MODE_COUNT)));
        return true;
    }
};


// Control that toggles the operation mode (sum, mult) of a parameter slot
// like a toggle, between "x" and "+"
class ParameterOperationModeControl : public ParameterSlotControlMixins, public MenuItem {

    public:
    ParameterOperationModeControl(const char *label, FloatParameter **parameter, int slot_number) : 
        ParameterSlotControlMixins(parameter, slot_number), MenuItem(label) {
    }

    virtual int renderValue(bool selected, bool, uint16_t) override {
        colours(selected, this->get_colour(), BLACK);

        tft->setTextSize(2);
        if (parameter == nullptr || *parameter == nullptr) {
            tft->print("?");
            return tft->getCursorY();
        }

        int mode = (*parameter)->get_slot_operation_mode(slot_number);
        tft->print(mode == MOD_SLOT_SUM ? "+" : "x");

        return tft->getCursorY();
    }

    virtual bool action_opened() override {
        // toggle between sum and multiply operation modes
        (*parameter)->set_slot_operation_mode(
            slot_number,
            (*parameter)->get_slot_operation_mode(slot_number) == MOD_SLOT_SUM ? MOD_SLOT_MULT : MOD_SLOT_SUM
        );
        return false;   // don't 'open' the menu item itself
    }

};


class ParameterModSlotRow;

class ParameterSlotReorderControl : public MenuItem {
    ParameterModSlotRow *row;

public:
    ParameterSlotReorderControl(ParameterModSlotRow *row) : MenuItem("Move"), row(row) {}
    int renderValue(bool selected, bool opened, uint16_t) override {
        colours(selected, opened ? GREEN : C_WHITE, BLACK);
        tft->setTextSize(1);
        tft->print("#");
        return tft->getCursorY();
    }
    bool knob_left() override;
    bool knob_right() override;
    bool button_select() override { return true; }
};

class ParameterModSlotRow : public ParameterSlotControlMixins, public SubMenuItemBarCustomProportions {
public:
    ParameterModSlotRow *previous_row = nullptr;
    ParameterModSlotRow *next_row = nullptr;

    ParameterModSlotRow(const char *label, FloatParameter **parameter, int slot_number)
        : ParameterSlotControlMixins(parameter, slot_number),
          SubMenuItemBarCustomProportions(label,
          #ifdef ENABLE_PARAMETERS_MIDI_LEARN
              7,
          #else
              6,
          #endif
          false, false)
    {
        this->set_column_proportion(0, 0.06f);
        this->set_column_proportion(1, 0.45f);
        this->set_column_proportion(2, 0.25f);
        float individual_widths = 1.0f-(0.06f+0.45f+0.25f);
        #ifdef ENABLE_PARAMETERS_MIDI_LEARN
            this->set_column_proportion(3, individual_widths / 4.0f);
            this->set_column_proportion(4, individual_widths / 4.0f);
            this->set_column_proportion(5, individual_widths / 4.0f);
            this->set_column_proportion(6, individual_widths / 4.0f);
        #else
            this->set_column_proportion(3, individual_widths / 3.0f);
            this->set_column_proportion(4, individual_widths / 3.0f);
            this->set_column_proportion(5, individual_widths / 3.0f);
        #endif

        this->add(new ParameterSlotReorderControl(this));
        BaseParameterInput *initial_input = nullptr;
        initial_input = get_slot_parameter_input();

        ParameterSlotInputSelectorControl *source = new ParameterSlotInputSelectorControl(
            "Source",
            parameter,
            slot_number,
            parameter_manager->available_inputs,
            initial_input
        );
        source->flags.go_back_on_select = true;
        this->add(source);

        // Amount control
        char amt_label[8];
        snprintf(amt_label, sizeof(amt_label), "Amt%i", slot_number + 1);
        ParameterMapPercentageControl *amount = new ParameterMapPercentageControl(amt_label, parameter, slot_number);
        this->add(amount);

        // Operation mode control
        ParameterOperationModeControl *operation_mode_control = new ParameterOperationModeControl(
            "OpMode", parameter, slot_number
        );
        this->add(operation_mode_control);

        // Mode selector
        ParameterConnectionPolarityTypeSelectorControl *mode = new ParameterConnectionPolarityTypeSelectorControl(
            "Mode", parameter, slot_number
        );
        this->add(mode);

        #ifdef ENABLE_PARAMETERS_MIDI_LEARN
            // 4th column (MIDI Learn control)
            this->add(new ParameterModSlotMidiLearnControl("MIDI Learn", parameter, slot_number));
        #endif

        // S&H mode (4th or 5th column, single-character indicator + popout overlay)
        this->add(new ParameterModSlotSHControl("SH", parameter, slot_number));
    }

    void link_next(ParameterModSlotRow *next) {
        next_row = next;
        if (next != nullptr) next->previous_row = this;
    }

    bool move_grabbed_slot(ParameterModSlotRow *destination) {
        if (destination == nullptr || parameter == nullptr || *parameter == nullptr || menu == nullptr ||
            destination->parameter != parameter || !(*parameter)->is_valid_slot(slot_number) ||
            !(*parameter)->is_valid_slot(destination->slot_number))
            return false;
        if (!menu->transfer_opened_item(this, destination)) return false;
        if (!(*parameter)->move_slot(slot_number, destination->slot_number)) {
            menu->transfer_opened_item(destination, this);
            return false;
        }
        currently_selected = 0;
        currently_opened = -1;
        destination->currently_selected = destination->currently_opened = 0;
        return true;
    }

    virtual int small_display(int index, int x, int y, int width_in_pixels, bool is_selected, bool is_opened, bool outer_selected) override {
        int end_y = SubMenuItemBarCustomProportions::small_display(index, x, y, width_in_pixels, is_selected, is_opened, outer_selected);
        // if (is_opened) {
        //     MenuItem *ctrl = this->items->get(index);
        //     if (!this->show_sub_headers && ctrl != nullptr && !ctrl->wants_fullscreen_overlay_when_opened_in_bar()) {
        //         tft->drawFastHLine(x, y-2, width_in_pixels, GREEN);
        //         tft->drawFastHLine(x, y-1, width_in_pixels, GREEN);
        //     }
        // }
        return end_y;
    }
};

inline bool ParameterSlotReorderControl::knob_left() {
    row->move_grabbed_slot(row->previous_row);
    return true;
}

inline bool ParameterSlotReorderControl::knob_right() {
    row->move_grabbed_slot(row->next_row);
    return true;
}


// Redesigned compound menu item: Value | Minimum | Maximum | Output in one row
// Slot rows are added separately by makeControls() so they are top-level navigable items
class ParameterMenuItem : public SubMenuItemBarCustomProportions {
    public:

    FloatParameter **proxy_parameter = nullptr;

    ParameterMenuItem(const char *label, FloatParameter **proxy_parameter, bool show_header = true)
        : SubMenuItemBarCustomProportions(label, 4, true, show_header)
    {
        this->proxy_parameter = proxy_parameter;

        this->set_column_proportion(0, 0.25f);  // Value
        this->set_column_proportion(1, 0.25f);  // Minimum
        this->set_column_proportion(2, 0.25f);  // Maximum
        this->set_column_proportion(3, 0.25f);  // Output

        this->add(new ParameterValueMenuItem((char*)"Value", this->proxy_parameter));
        this->add(new ParameterRangeMenuItem("Min", this->proxy_parameter, MINIMUM));
        this->add(new ParameterRangeMenuItem("Max", this->proxy_parameter, MAXIMUM));
        ParameterOutputMenuItem *output = new ParameterOutputMenuItem(this->proxy_parameter);
        this->add(output);
    }

    virtual int small_display(int index, int x, int y, int width_in_pixels, bool is_selected, bool is_opened, bool outer_selected) override {
        int end_y = SubMenuItemBarCustomProportions::small_display(index, x, y, width_in_pixels, is_selected, is_opened, outer_selected);
        if (is_opened) {
            MenuItem *ctrl = this->items->get(index);
            if (ctrl != nullptr && !ctrl->wants_fullscreen_overlay_when_opened_in_bar())
                tft->drawFastHLine(x, y, width_in_pixels, GREEN);
        }
        return end_y;
    }
};