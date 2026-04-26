#pragma once

#include "menuitems.h"
#include "menuitems_popout.h"

#include "parameter_inputs/ParameterInput.h"
#include "ParameterManager.h"

extern ParameterManager *parameter_manager;
class VoltageParameterInput;

// TODO: allow to deselect a selector (ie set None) -- believe this was done a long time ago? (2024-12-13)

// Selector to choose a ParameterInput from the available list to use a Source; 
// Also used more directly by objects/parameters that can only feed from one ParameterInput at a time, eg CVInput
template<class TargetClass>
class ParameterInputSelectorControl : public SelectorControl<int_least16_t> {
    BaseParameterInput *initial_selected_parameter_input = nullptr;
    LinkedList<BaseParameterInput*> *available_parameter_inputs = nullptr;

    TargetClass *target_object = nullptr;
    TargetClass **proxy_target_object = &target_object;

    void(TargetClass::*setter_func)(BaseParameterInput*);
    BaseParameterInput*(TargetClass::*getter_func)();
    TargetClass *last_target_object = nullptr;

    bool show_values = false;   // whether to display the incoming values or not

    int get_available_inputs_count() const {
        return this->available_parameter_inputs!=nullptr ? (int)this->available_parameter_inputs->size() : 0;
    }

    int get_none_index() const {
        return this->get_available_inputs_count();
    }

    bool is_valid_value_index(int index) const {
        return index>=0 && index<(int)this->num_values;
    }

    int get_sanitised_index(int index) const {
        if (this->is_valid_value_index(index))
            return index;
        if (this->is_valid_value_index(this->actual_value_index))
            return this->actual_value_index;
        return this->get_none_index();
    }

    TargetClass *get_current_target_object() {
        return this->proxy_target_object!=nullptr ? *this->proxy_target_object : nullptr;
    }

    int get_index_for_current_target_value() {
        TargetClass *current_target = this->get_current_target_object();
        if (current_target==nullptr || this->getter_func==nullptr)
            return this->get_none_index();

        BaseParameterInput *current_input = (current_target->*this->getter_func)();
        if (current_input==nullptr)
            return this->get_none_index();

        const int idx = this->find_parameter_input_index_for_object(current_input);
        return idx>=0 ? idx : this->get_none_index();
    }

    int sync_indices_from_target(bool force_selected_value = false) {
        const int current_index = this->get_index_for_current_target_value();
        this->actual_value_index = current_index;
        if (force_selected_value)
            this->selected_value_index = current_index;
        return current_index;
    }

    BaseParameterInput *get_parameter_for_index(int index) {
        const int input_count = this->get_available_inputs_count();
        if (index<0 || index>=input_count)
            return nullptr;
        return this->available_parameter_inputs->get(index);
    }

    const char *get_name_for_index(int index) {
        BaseParameterInput *input = this->get_parameter_for_index(index);
        return input!=nullptr ? (const char*)input->name : "None";
    }

    const char *get_group_for_index(int index) {
        BaseParameterInput *input = this->get_parameter_for_index(index);
        return input!=nullptr ? (const char*)input->group_name : "None";
    }

    uint16_t get_colour_for_index(int index) {
        BaseParameterInput *input = this->get_parameter_for_index(index);
        if (input!=nullptr)
            return input->colour;
        return YELLOW/2;
    }

    void get_hint_label_for_index(int index, bool left_hint, char *out, size_t out_len) {
        if (out_len==0)
            return;
        const char *name = this->get_name_for_index(index);
        snprintf(out, out_len, left_hint ? "< %s" : "%s >", name);
    }

    int wrap_index(int index) {
        const int count = (int)this->num_values;
        if (count<=0)
            return 0;
        while (index < 0)
            index += count;
        while (index >= count)
            index -= count;
        return index;
    }

    public:

    ParameterInputSelectorControl(
        const char *label, 
        TargetClass *target_object, 
        void(TargetClass::*setter_func)(BaseParameterInput*), 
        BaseParameterInput*(TargetClass::*getter_func)(), 
        LinkedList<BaseParameterInput*> *available_parameter_inputs,
        BaseParameterInput *initial_parameter_input = nullptr,
        bool show_values = false
    ) : SelectorControl(label, 0) {
        this->show_values = show_values;
        this->initial_selected_parameter_input = initial_parameter_input;
        this->available_parameter_inputs = available_parameter_inputs;
        this->target_object = target_object;
        this->setter_func = setter_func;
        this->getter_func = getter_func;
        this->num_values = available_parameter_inputs->size() + 1;  // + 1 for None optino .. 
    };
    ParameterInputSelectorControl(
        const char *label, 
        TargetClass **proxy_target_object, 
        void(TargetClass::*setter_func)(BaseParameterInput*), 
        BaseParameterInput*(TargetClass::*getter_func)(),
        LinkedList<BaseParameterInput*> *available_parameter_inputs,
        BaseParameterInput *initial_parameter_input = nullptr,
        bool show_values = false
    ) : ParameterInputSelectorControl(label, *proxy_target_object, setter_func, getter_func, available_parameter_inputs, initial_parameter_input, show_values) {
        this->proxy_target_object = proxy_target_object;
    };

    virtual void configure (LinkedList<BaseParameterInput*> *available_parameter_inputs) {
        this->available_parameter_inputs = available_parameter_inputs;
        char *initial_name = (char*)"None";
        if (this->initial_selected_parameter_input!=nullptr)
            initial_name = this->initial_selected_parameter_input->name;
        actual_value_index = this->find_parameter_input_index_for_label(initial_name);
    }

    virtual int find_parameter_input_index_for_label(char *name) {
        if (strcmp(name, "None")==0)
            return this->get_none_index();
        if (this->available_parameter_inputs==nullptr)
            return -1;
        const uint_fast16_t size = this->available_parameter_inputs->size();
        for (uint_fast16_t i = 0 ; i < size ; i++) {
            if (available_parameter_inputs->get(i)->matches_label(name))
                return i;
        }
        return -1;
    }

    virtual int find_parameter_input_index_for_object(BaseParameterInput *input) {
        if (input==nullptr)
            return this->get_none_index();
        return this->find_parameter_input_index_for_label(input->name);
    }

    virtual void on_add() override {
        //this->debug = true;
        //Serial.printf(F("%s#on_add...\n"), this->label); Serial_flush();
        actual_value_index = -1;
        /*if (this->debug) {
            Serial.printf("on_add() in ParameterSelectorControl @%p:\n", this); Serial_flush();
            Serial.printf("\tParameterSelectorControl with initial_selected_parameter @%p...\n", initial_selected_parameter_input); Serial_flush();
            if (initial_selected_parameter_input!=nullptr) {
                Serial.printf("\tParameterSelectorControl looking for '%s' @%p...\n", initial_selected_parameter_input->label, initial_selected_parameter_input); Serial_flush();
            } else 
                Serial.println("\tno initial_selected_parameter set");
        }*/

        if (initial_selected_parameter_input!=nullptr) {
            //Serial.printf(F("%s#on_add: got non-null initial_selected_parameter_input\n")); Serial_flush();
            //Serial.printf(F("\tand its name is %c\n"), initial_selected_parameter_input->name); Serial_flush();
            //this->actual_value_index = parameter_manager->getInputIndexForName(initial_selected_parameter_input->name); ////this->find_parameter_input_index_for_label(initial_selected_parameter_input->name);
            this->actual_value_index = this->find_parameter_input_index_for_label(initial_selected_parameter_input->name);
        } else {
            this->actual_value_index = -1;
        }
        this->selected_value_index = this->actual_value_index;
        //Serial.printf(F("#on_add returning"));
    }

    virtual const char *get_label_for_index(int_least16_t index) {
        static char label_for_index[MENU_C_MAX];
        const char *name = this->get_name_for_index(index);
        snprintf(label_for_index, MENU_C_MAX, "%s", name);
        return label_for_index;
    }

    // update the control to reflect changes to selection (eg, called when new value is loaded from project file)
    /*virtual void update_source(BaseParameterInput *new_source) {
        //int index = parameter_manager->getPitchInputIndex(new_source);
        //Serial.printf("update_source got index %i\n", index);
        if (new_source==nullptr) {
            this->update_actual_index(-1);
        } else {
            int index = this->find_parameter_input_index_for_object(new_source);
            this->update_actual_index(index);
        }
    }*/

    virtual void setter (int_least16_t new_value) {
        //if (this->debug) Serial.printf(F("ParameterSelectorControl changing from %i to %i\n"), this->actual_value_index, new_value);
        selected_value_index = actual_value_index = new_value;
        if(new_value>=0 && (*this->proxy_target_object)!=nullptr && this->setter_func!=nullptr) {
            if (new_value < (int)this->available_parameter_inputs->size())
                ((*this->proxy_target_object)->*this->setter_func)(this->available_parameter_inputs->get(new_value));
            else
                ((*this->proxy_target_object)->*this->setter_func)(nullptr);
        }
    }
    virtual int_least16_t getter () {
        return selected_value_index;
    }

    // classic fixed display version
    virtual int display(Coord pos, bool selected, bool opened) override {
        tft->setTextSize(0);

        const TargetClass *current_target = this->get_current_target_object();
        const bool target_changed = current_target != this->last_target_object;
        this->last_target_object = (TargetClass*)current_target;

        this->sync_indices_from_target(!opened || target_changed);

        if (!opened) {
            pos.y = header(label, pos, selected, opened);
            const int display_index = this->get_sanitised_index(this->actual_value_index);
            const uint16_t value_colour = this->get_colour_for_index(display_index);
            colours(selected, value_colour, BLACK);
            tft->print("Selected: ");
            tft->println(this->get_name_for_index(display_index));
            return tft->getCursorY();
        }

        // Takeover display: large, high-contrast, centered value with left/right hints.
        const int display_index = this->get_sanitised_index(this->selected_value_index);
        this->selected_value_index = display_index;

        const uint16_t value_colour = this->get_colour_for_index(display_index);

        char group_txt[MENU_C_MAX];
        const char *current_group = this->get_group_for_index(display_index);
        if (current_group!=nullptr && strcmp(current_group, "None")!=0)
            snprintf(group_txt, MENU_C_MAX, "%s", current_group);
        else
            snprintf(group_txt, MENU_C_MAX, "None");

        const int left_index = this->wrap_index(display_index - 1);
        const int right_index = this->wrap_index(display_index + 1);
        char left_hint[MENU_C_MAX];
        char right_hint[MENU_C_MAX];
        this->get_hint_label_for_index(left_index, true, left_hint, sizeof(left_hint));
        this->get_hint_label_for_index(right_index, false, right_hint, sizeof(right_hint));
        const int none_index = this->get_none_index();
        const uint16_t neutral_hint_colour = tft->halfbright_565(C_WHITE);
        const uint16_t left_hint_colour = left_index==none_index
            ? neutral_hint_colour
            : tft->halfbright_565(this->get_colour_for_index(left_index));
        const uint16_t right_hint_colour = right_index==none_index
            ? neutral_hint_colour
            : tft->halfbright_565(this->get_colour_for_index(right_index));

        SelectorTakeoverOverlaySpec overlay;
        overlay.title = this->label;
        overlay.subtitle = group_txt;
        overlay.value = this->get_name_for_index(display_index);
        overlay.left_hint = left_hint;
        overlay.right_hint = right_hint;
        overlay.frame_colour = selected ? GREEN : C_WHITE;
        overlay.value_fg = value_colour;
        overlay.subtitle_fg = C_WHITE;
        overlay.left_hint_fg = left_hint_colour;
        overlay.right_hint_fg = right_hint_colour;
        overlay.box_padding = 4;
        overlay.min_box_h = 28;

        overlay.has_extra = true;
        overlay.extra_height = 32;
        overlay.draw_extra_fn = [](void *userdata, DisplayTranslator *tft, int x, int y, int max_width, int max_height) {
            // draw a little representation of the CV input signal if this ParameterInput supports it
            auto *self = static_cast<ParameterInputSelectorControl<TargetClass>*>(userdata);
            BaseParameterInput *input = self->get_parameter_for_index(self->selected_value_index);
            if (input!=nullptr) {
                ParameterInputDisplay *display = input->parameter_input_display;
                if (display!=nullptr) {
                    display->draw_graph(x, y, max_width, max_height);
                }
            }
        };
        overlay.draw_extra_userdata = this;

        return menu_draw_selector_takeover_overlay(tft, pos, overlay);
    }

    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        this->sync_indices_from_target(false);
        const int index_to_display = this->get_sanitised_index(opened ? selected_value_index : actual_value_index);
        BaseParameterInput *currently_active = this->get_parameter_for_index(index_to_display);
        const int col = selected_value_index==this->actual_value_index 
                && opened ? GREEN : this->get_colour_for_index(index_to_display);
        
        colours(selected, col, BLACK);
        char txt[MENU_C_MAX];
        snprintf(txt, MENU_C_MAX, "%s", currently_active!=nullptr ? currently_active->name : "None");
        tft->setTextSize(tft->get_textsize_for_width(txt, max_character_width*tft->characterWidth()));
        tft->println(txt);
        return tft->getCursorY();
    }

    virtual bool wants_fullscreen_overlay_when_opened_in_bar() override {
        return true;
    }

    virtual bool button_select() override {
        //Serial.printf("button_select with selected_value_index %i\n", selected_value_index);
        //Serial.printf("that is available_values[%i] of %i\n", selected_value_index, available_values[selected_value_index]);
        this->setter(selected_value_index);

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        const char *name = selected_value_index>=0 && selected_value_index < (int)this->available_parameter_inputs->size() 
                            ? 
                            this->available_parameter_inputs->get(selected_value_index)->name 
                            : 
                            "None";
        //if (selected_value_index>=0)
        snprintf(msg, MENU_MESSAGE_MAX, set_message, label, name, selected_value_index);
        //Serial.printf("about to set_last_message!");
        //msg[20] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);

        return go_back_on_select;
    }

};
