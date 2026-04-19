#pragma once

#include "Arduino.h"

#include <LinkedList.h>

#include "menu.h"
#include "colours.h"

#include "../parameters/Parameter.h"
#include "ParameterMenuItems.h"

// class that allows user to select from a list of parameters, and edit that parameter.  
// other controls can point their (FloatParameter**)parameter pointers here so that they pick up the currently-selected parameter's data
class ParameterMenuItemSelector : public SelectorControl<int_least16_t> { //public ObjectSelectorControl<ParameterMenuItemSelector,ParameterMenuItem*> {
    public:
    LinkedList<FloatParameter*> *parameters = nullptr;
    FloatParameter *parameter = nullptr;

    //int selected_item = -1;
    bool selecting = true;

    int cached_row_index = -32768;
    int cached_row_max_chars = -1;
    char cached_row_text[MENU_C_MAX] = "";

    ParameterMenuItemSelector(const char *label, LinkedList<FloatParameter*> *parameters) : 
        SelectorControl<int_least16_t>(label) {
        this->selected_value_index = -1;
        this->actual_value_index = -1;
        this->parameter = nullptr;
        this->parameters = parameters;

        if (this->parameters != nullptr) {
            this->num_values = this->parameters->size();
            if (this->num_values > 0) {
                this->selected_value_index = 0;
                this->actual_value_index = 0;
                this->parameter = this->parameters->get(0);
            }
        } else {
            this->num_values = 0;
        }
    }

    FloatParameter *last_object = nullptr;
    int_least16_t last_index = -1;
    char last_label[MENU_C_MAX];
    virtual const char *get_label_for_index(int_least16_t index) {
        if (this->parameters==nullptr)
            return "None";

        if (index<0 || index >= (int_least16_t)this->parameters->size())
            return "None";

        if (last_index!=index) {
            last_object = this->parameters->get(index);
            last_index = index;
            snprintf(last_label, MENU_C_MAX, "%s", last_object->label);
        }

        //Serial.printf("got label for index %i: '%s'\n", index, last_label); Serial.flush();

        return last_label;
    }

    void invalidate_selector_row_cache() {
        this->cached_row_index = -32768;
        this->cached_row_max_chars = -1;
        this->cached_row_text[0] = '\0';
    }

    void build_selector_row_text(char *out, size_t out_len, int max_chars) {
        if (out==nullptr || out_len==0)
            return;

        const int current_index = this->selected_value_index;
        if (this->cached_row_index == current_index && this->cached_row_max_chars == max_chars) {
            snprintf(out, out_len, "%s", this->cached_row_text);
            return;
        }

        const char *base = this->get_label_for_index(this->selected_value_index);

        if (max_chars <= 0) {
            snprintf(this->cached_row_text, sizeof(this->cached_row_text), "%s", base);
            this->cached_row_index = current_index;
            this->cached_row_max_chars = max_chars;
            snprintf(out, out_len, "%s", this->cached_row_text);
            return;
        }

        int available_for_base = max_chars;

        const int base_len = (int)strlen(base);
        if (base_len <= available_for_base) {
            snprintf(this->cached_row_text, sizeof(this->cached_row_text), "%s", base);
            this->cached_row_index = current_index;
            this->cached_row_max_chars = max_chars;
            snprintf(out, out_len, "%s", this->cached_row_text);
            return;
        }

        // Middle truncation keeps both start and end (often where numeric suffixes live).
        if (available_for_base <= 4) {
            snprintf(this->cached_row_text, sizeof(this->cached_row_text), "%.*s", available_for_base, base);
            this->cached_row_index = current_index;
            this->cached_row_max_chars = max_chars;
            snprintf(out, out_len, "%s", this->cached_row_text);
            return;
        }

        const int ellipsis_len = 2; // ".."
        int keep_tail = min(6, available_for_base / 2);
        int keep_head = available_for_base - keep_tail - ellipsis_len;
        if (keep_head < 1) {
            keep_head = 1;
            keep_tail = max(1, available_for_base - keep_head - ellipsis_len);
        }

        const int base_tail_start = max(0, base_len - keep_tail);
        snprintf(this->cached_row_text, sizeof(this->cached_row_text), "%.*s..%s", keep_head, base, base + base_tail_start);
        this->cached_row_index = current_index;
        this->cached_row_max_chars = max_chars;
        snprintf(out, out_len, "%s", this->cached_row_text);
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        tft->setTextSize(2);
        //Serial.printf("ParameterMenuItemSelector parameter address is @%p (address of that pointer is @%p)\n", this->parameter, &this->parameter);

        int char_w = tft->currentCharacterWidth();
        if (char_w <= 0)
            char_w = 1;
        const int total_chars = tft->width() / char_w;
        const bool show_marker = selected || opened;
        const int lead_chars = show_marker ? 1 : 0;   // room for the leading '>' marker
        int max_label_chars = total_chars - lead_chars - 1;
        if (max_label_chars < 1)
            max_label_chars = 1;

        if (opened) {
            colours(selected, GREEN, BLACK); 
        } else {
            colours(selected, this->default_fg, BLACK);
        }

        if (show_marker) {
            tft->print(">");
        }

        char selector_row_text[MENU_C_MAX];
        this->build_selector_row_text(selector_row_text, sizeof(selector_row_text), max_label_chars);
        tft->print(selector_row_text);

        tft->println("");
        pos.y = tft->getCursorY();
        return pos.y;
    }

    virtual bool knob_left() override {
        if (this->parameters==nullptr || this->num_values<=0)
            return false;

        // not opened, so we're scrolling on the menu
        bool v = SelectorControl::knob_left();
        if (this->selected_value_index>=0 && this->selected_value_index < this->num_values)
            parameter = this->parameters->get(this->selected_value_index);
        this->invalidate_selector_row_cache();
        return v;
    }
    virtual bool knob_right() override {
        if (this->parameters==nullptr || this->num_values<=0)
            return false;

        bool v = SelectorControl::knob_right();
        if (this->selected_value_index>=0 && this->selected_value_index < this->num_values)
            parameter = this->parameters->get(this->selected_value_index);
        this->invalidate_selector_row_cache();
        return v;
    }

    virtual bool button_select() override {
        return SELECT_EXIT;
    }
    virtual bool button_back() override {
        return BACK_EXIT;
    }
};

#include "ParameterInputMenuItems.h"


/// controls for allowing the same parameter menu items to be re-used on multiple pages

// store pointers to the controls to re-use
struct lowmemory_controls_t {
    // re-usable controls
    ParameterMenuItem *parameter_amount_controls;
    SubMenuItemBar *input_selectors_bar;
    SubMenuItemBar *polarity_submenu;
    SubMenuItemBar *range_submenu;
    // pointer to the current control being edited
    FloatParameter *parameter;
};
extern lowmemory_controls_t lowmemory_controls;

// "dummy" invisible control that allows to switch single instances of editor controls to operate on chosen parameter
// so that when this control is rendered, the correct parameter is swapped in to be displayed/edited
class LowMemorySwitcherMenuItem : public MenuItem {
    public:
    LinkedList<FloatParameter*> *parameters = nullptr;
    ParameterMenuItemSelector *parameter_selector = nullptr;

    LowMemorySwitcherMenuItem(char *label, LinkedList<FloatParameter*> *parameters, ParameterMenuItemSelector *parameter_selector, int_fast16_t default_fg) 
        : MenuItem(label) {
            this->parameters = parameters;
            this->parameter_selector = parameter_selector;
            this->default_fg = default_fg;
            this->selectable = false;
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        //Serial.printf("switching lowmemory_controls.parameter pointer from @%p to @%p\n", lowmemory_controls.parameter, &this->parameter_selector->parameter);
        // dont actually display anything, just update the parameter_selector
        lowmemory_controls.parameter = this->parameter_selector->parameter;
        // todo: tell the dependent controls to update their internal values etc
        return pos.y;
    }
};

// "dummy" invisible control that allows re-use of single instance of editor controls for a single parameter
class LowMemoryEmbedMenuItem : public MenuItem {
    public:
    //LinkedList<FloatParameter*> *parameters = nullptr;
    //ParameterMenuItemSelector *parameter_selector = nullptr;
    FloatParameter *parameter = nullptr;

    LowMemoryEmbedMenuItem(char *label, FloatParameter *parameter, int_fast16_t default_fg) 
        : MenuItem(label) {
            this->parameter = parameter;
            //this->parameter_selector = parameter_selector;
            this->default_fg = default_fg;
            this->selectable = false;
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        //Serial.printf("switching lowmemory_controls.parameter pointer from @%p to @%p\n", lowmemory_controls.parameter, &this->parameter_selector->parameter);
        // dont actually display anything, just update the parameter_selector
        //lowmemory_controls.parameter = this->parameter_selector->parameter;
        lowmemory_controls.parameter = this->parameter;
        // todo: tell the dependent controls to update their internal values etc
        return pos.y;
    }
};

// create 'low-memory' controls for a list of parameters
// re-uses the same actual controls, inserting a dummy LowMemorySwitcherMenuItem or LowMemroryEmbedMenuItem so that the correct item is pointed at
void create_low_memory_parameter_controls(const char *label, LinkedList<FloatParameter*> *parameters, int_fast16_t default_fg = C_WHITE);
void create_low_memory_parameter_controls(const char *label, FloatParameter *parameters, int_fast16_t default_fg = C_WHITE);