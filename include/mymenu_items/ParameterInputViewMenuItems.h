#ifndef MENU_PARAMETERINPUT_VIEW_MENUITEMS__INCLUDED
#define MENU_PARAMETERINPUT_VIEW_MENUITEMS__INCLUDED

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

#include "../parameter_inputs/ParameterInput.h"

#include <LinkedList.h>

#ifndef PARAMETER_INPUT_GRAPH_HEIGHT
    #define PARAMETER_INPUT_GRAPH_HEIGHT 50
#endif

#include "bpm.h"    // because we need to know the current ticks

//template<unsigned long memory_size>
class ParameterInputDisplay : public MenuItem
#ifdef PARAMETER_INPUTS_USE_CALLBACKS
    , public ParameterInputCallbackReceiver 
#endif
{
    public:
        BaseParameterInput *parameter_input = nullptr;

        // todo: remember an int type instead of a float, for faster drawing
        typedef float memory_log;
        unsigned long memory_size;
        memory_log *logged = nullptr;

        ParameterInputDisplay(char *label, unsigned long memory_size, BaseParameterInput *input) : MenuItem(label) {
            this->parameter_input = input;
            this->memory_size = memory_size;
            this->selectable = !input->supports_bipolar();
            if (parameter_input!=nullptr) 
                this->set_default_colours(parameter_input->colour);

            //logged = (memory_log*)malloc(memory_size * sizeof(float));
            //memset(logged, 0, memory_size*sizeof(float));
            logged = (memory_log*)calloc(memory_size, sizeof(float));
        }

        virtual void configure(BaseParameterInput *parameter_input) {
            this->parameter_input = parameter_input;
        }

        unsigned long ticks_to_memory_step(uint32_t ticks) {
            return ( ticks % memory_size );
        }

        /*unsigned long last_position_updated;
        virtual void update_ticks(unsigned long ticks) {
            // update internal log of values
            if (ticks==last_position_updated)
                return;
            unsigned int position = ticks % memory_size;
            float value = this->parameter_input->get_normal_value();
            last_position_updated = ticks;

            if (this->parameter_input->output_type==BIPOLAR) {
                // center is 0, range -1 to +1, so re-center display
                (logged)[position] = (0.5) + (value / 2);
            } else if (this->parameter_input->output_type==UNIPOLAR) {
                // center is 0.5, range 0 to 1.. dont modif 
                (logged)[position] = value;
            }
            if (this->debug) Debug_printf(F("\tupdate_ticks(%i) recorded %f\n"), position, logged[position]);
        }*/
        /*unsigned long ticks;
        virtual void update_ticks(unsigned long ticks) {
            this->ticks = ticks;
        }*/

        #ifndef PARAMETER_INPUTS_USE_CALLBACKS
            // not using callbacks when input values change, so update every menu tick instead
            virtual void update_ticks(unsigned long ticks) {
                this->receive_value_update(this->parameter_input->get_normal_value());
            }
        #endif

        uint_fast16_t last_position_updated;
        virtual void receive_value_update(float value) {
            uint_fast16_t position = ticks_to_memory_step(ticks);

            if (position==last_position_updated)
                return;

            // convert value according to the input/output settings
            if (this->parameter_input->output_type==BIPOLAR) {
                // center is 0, range -1 to +1, so re-center display
                (logged)[position] = (0.5) + (value / 2);
            } else if (this->parameter_input->output_type==UNIPOLAR) {
                // center is 0.5, range 0 to 1.. dont modif 
                (logged)[position] = value;
            }

            // do a simple backfill of values we missed
            if (last_position_updated < position && (last_position_updated) - position > 1) {
                for (uint_fast16_t i = last_position_updated+1 ; i < position ; i++)
                    (logged)[i] = (logged)[position];
            }

            last_position_updated = position;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            //Serial.println("MidiOutputSelectorControl display()!");
            tft->setTextSize(0);

            #define DISPLAY_INFO_IN_LABEL
            #ifdef DISPLAY_INFO_IN_LABEL
                static char custom_label[MAX_LABEL_LENGTH*2];
                //sprintf(custom_label, "%s: [%s] %-3s >%-3i %-3s>",
                //sprintf(custom_label, "%s: [%s] %-7s >%-4i %-4s>",
                    //sprintf(custom_label, "%s: %-7s >%-4s ", //%-4s>",
                snprintf(custom_label, MAX_LABEL_LENGTH*2, "[%s] %-3s >%-3i %-3s>",
                    //label,                    
                    this->parameter_input!=nullptr ? (char*)this->parameter_input->getInputInfo()  : "",
                    this->parameter_input!=nullptr ? (char*)this->parameter_input->getInputValue() : "",
                    //(int)(this->logged[(ticks%LOOP_LENGTH_TICKS] * 100.0)
                    this->parameter_input!=nullptr ? (int)(this->parameter_input->get_normal_value()*100.0) : 0, 
                    (char*)this->parameter_input->getOutputValue()
                );
                colours(selected, parameter_input->colour, BLACK);
                pos.y = header(custom_label, pos, selected, opened);         
            #else 
                pos.y = header(label, pos, selected, opened);                      
                //tft->setCursor(pos.x, pos.y);
            #endif
            pos.y = tft->getCursorY();

            // switch back to colour-on-black for actual display
            colours(false, parameter_input->colour, BLACK);

            const uint16_t base_row = pos.y;
            static float ticks_per_pixel = (float)memory_size / (float)tft->width();

            int last_y = 0;
            for (int screen_x = 0 ; screen_x < tft->width() ; screen_x++) {
                const uint16_t tick_for_screen_X = ticks_to_memory_step((int)((float)screen_x * ticks_per_pixel)); // the tick corresponding to this screen position
                const int y = PARAMETER_INPUT_GRAPH_HEIGHT - ((logged)[tick_for_screen_X] * PARAMETER_INPUT_GRAPH_HEIGHT);
                if (screen_x != 0) {
                    //int last_y = GRAPH_HEIGHT - (this->logged[tick_for_screen_X] * GRAPH_HEIGHT);
                    tft->drawLine(screen_x-1, base_row + last_y, screen_x, base_row + y, parameter_input->colour);                    
                }
                last_y = y;
            }

            tft->setCursor(pos.x, pos.y + PARAMETER_INPUT_GRAPH_HEIGHT + 5);    // set cursor to below the graph's output

            //this->do_extra(this->parameter_input);
            if (this->parameter_input!=nullptr && this->parameter_input->hasExtra())
                tft->printf((char*)"Extra: %s\n", (char*)this->parameter_input->getExtra());
            /*tft->printf("Value: %i ", (int)(this->logged[ticks%LOOP_LENGTH_TICKS] * 100.0));
            tft->printf((char*)"Inp: %-8s ", (char*)this->parameter_input->getInputInfo()); //i @ %p")
            tft->printf((char*)"Read: %-8s ", (char*)this->parameter_input->getInputValue());*/
            //tft->printf((char*)"Tgt: %-15s\n", (char*)get_label_for_index(actual_value_index));
            //tft->printf((char*)"Val: %-7s ",  (char*)this->parameter_input->getFormattedValue());

            return tft->getCursorY();
        }


};

class InputTypeSelectorControl : public SelectorControl<int> {
    int actual_value_index;
    byte *target = nullptr;

    public:

    InputTypeSelectorControl(const char *label, byte *target) : SelectorControl(label) {
        this->target = target;
        actual_value_index = *target;
        this->selected_value_index = this->actual_value_index = this->getter();
    };

    virtual const char* get_label_for_index(int index) {
        if (index==BIPOLAR)
            return "Bipolar";
        if (index==UNIPOLAR)
            return "Unipolar";
        //if (index==CLOCK_NONE)
        //    return "None";
        return "??";
    }

    virtual void setter (int new_value) {
        *target = new_value;
        actual_value_index = new_value;
    }
    virtual int getter () {
        //return clock_mode; //selected_value_index;
        return *target;
    }

    // classic fixed display version
    virtual int display(Coord pos, bool selected, bool opened) override {
        //Serial.println("MidiOutputSelectorControl display()!");

        pos.y = header(label, pos, selected, opened);
        tft->setTextSize(2);

        num_values = 2; //NUM_CLOCK_SOURCES;

        tft->setTextSize(2);

        tft->setCursor(pos.x, pos.y);

        if (!opened) {
            // not opened, so just show the current value
            //colours(opened && selected_value_index==i, col, BLACK);

            tft->printf((char*)"%s", (char*)get_label_for_index(*target)); //selected_value_index));
            tft->println((char*)"");
        } else {
            int current_value = *target; //actual_value_index;

            for (int i = 0 ; i < num_values ; i++) {
                bool is_current_value_selected = (int)i==current_value;
                int col = is_current_value_selected ? GREEN : this->default_fg;
                colours(opened && selected_value_index==(int)i, col, BLACK);
                tft->setCursor(pos.x, pos.y);
                tft->printf((char*)"%s\n", (char*)get_label_for_index(i));
                pos.y = tft->getCursorY();
            }
            if (tft->getCursorX()>0) // if we haven't wrapped onto next line then do it manually
                tft->println((char*)"");
        }
        return tft->getCursorY();
    }

    virtual bool action_opened() override {
        //if (this->debug) Serial.printf(F("ObjectToggleControl#action_opened on %s\n"), this->label);
        bool value = !this->getter();
        //this->internal_value = !this->internal_value;

        this->setter(value); //(bool)this->internal_value);
        return false;   // don't 'open'
    }

    virtual bool button_select() override {
        this->selected_value_index = !this->selected_value_index;
        this->setter(selected_value_index);

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        snprintf(msg, MENU_MESSAGE_MAX, "Set type to %i: %s", selected_value_index, get_label_for_index(selected_value_index));
        //msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg, GREEN);

        return go_back_on_select;
    }

};


#endif
