#ifndef MENU_PARAMETERINPUT_VIEW_MENUITEMS__INCLUDED
#define MENU_PARAMETERINPUT_VIEW_MENUITEMS__INCLUDED

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

#include "../parameter_inputs/ParameterInput.h"

#include <LinkedList.h>

// this stuff ripped from midi_looper.h/menu_looperdisplay.h in usb_midi_clocker TODO: make more generic
//#define LOOP_LENGTH_STEP_SIZE 1         // resolution of loop TODO: problems when this is set >1; reloaded sequences (or maybe its converted-from-bitmap stage?) are missing note-offs
//#define ticks_to_sequence_step(X)  ((X % LOOP_LENGTH_TICKS) / LOOP_LENGTH_STEP_SIZE)

template<unsigned long memory_size>
class ParameterInputDisplay : public MenuItem {
    public:
        BaseParameterInput *parameter_input = nullptr;

        //double logged[LOOP_LENGTH_TICKS];
        typedef double memory_log[memory_size];
        memory_log *logged = nullptr;

        ParameterInputDisplay(char *label, BaseParameterInput *input) : MenuItem(label) {
        //ParameterInputDisplay(char *label, BaseParameterInput *input) : MenuItem(label) {
            this->parameter_input = input;
            //this->loop_length_ticks = loop_length_ticks;
            //double logged
            /*if (loop_length_ticks>0) {
                double logged[] = new double[loop_length_ticks];
            }*/
            //memset(logged, 0, sizeof(logged));
            logged = (memory_log*)malloc(memory_size * sizeof(double));
        }

        virtual void configure(BaseParameterInput *parameter_input) {
            this->parameter_input = parameter_input;
        }

        unsigned long ticks_to_memory_step(uint32_t ticks) {
            return ( ticks % memory_size );
        }

        virtual void update_ticks(unsigned long ticks) {
            // update internal log of values
            unsigned int position = ticks % LOOP_LENGTH_TICKS;
            (*logged)[position] = this->parameter_input->get_normal_value(); //get_voltage_normal();
            if (this->parameter_input->output_type==BIPOLAR) {
                // center is 0, range -1 to +1
                (*logged)[position] = (0.5) + ((*logged)[position] / 2);
            } else if (this->parameter_input->output_type==UNIPOLAR) {
                // center is 0.5, range 0 to 1
                //logged[position] =  ?? nothing
            }
            if (this->debug) Serial.printf("\tupdate_ticks(%i) recorded %f\n", position, logged[position]);
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            //Serial.println("MidiOutputSelectorControl display()!");
            tft->setTextSize(0);

            #define DISPLAY_INFO_IN_LABEL
            #ifdef DISPLAY_INFO_IN_LABEL
                static char custom_label[MAX_LABEL_LENGTH*2];
                sprintf(custom_label, "%s: [%s] %-7s >%-4i", // %-4s>",
                //sprintf(custom_label, "%s: %-7s >%-4s ", //%-4s>",
                    label,                    
                    this->parameter_input!=nullptr ? (char*)this->parameter_input->getInputInfo()  : "",
                    this->parameter_input!=nullptr ? (char*)this->parameter_input->getInputValue() : "",
                    //(int)(this->logged[(ticks%LOOP_LENGTH_TICKS] * 100.0)
                    this->parameter_input!=nullptr ? (int)(this->parameter_input->get_normal_value()*100.0) : 0
                    //(char*)this->parameter_input->getOutputValue()
                );
                pos.y = header(custom_label, pos, selected, opened);         
            #else 
                pos.y = header(label, pos, selected, opened);                      
                //tft->setCursor(pos.x, pos.y);
            #endif
            colours(false, C_WHITE, BLACK);
            //tft->printf("Input %c\n", parameter_input->name);
            pos.y = tft->getCursorY();

            const uint16_t base_row = pos.y;
            static float ticks_per_pixel = (float)LOOP_LENGTH_TICKS / (float)tft->width();

            // we're going to use direct access to the underlying Adafruit library here
            const DisplayTranslator_STeensy *tft2 = (DisplayTranslator_STeensy*)tft;
            ST7789_t3 *actual = tft2->tft;

            int GRAPH_HEIGHT = 50;

            int last_y = 0;
            for (int screen_x = 0 ; screen_x < tft->width() ; screen_x++) {
                const uint16_t tick_for_screen_X = ticks_to_memory_step((int)((float)screen_x * ticks_per_pixel)); // the tick corresponding to this screen position
                int y = GRAPH_HEIGHT - ((*logged)[tick_for_screen_X] * GRAPH_HEIGHT);
                if (screen_x != 0) {
                    //int last_y = GRAPH_HEIGHT - (this->logged[tick_for_screen_X] * GRAPH_HEIGHT);
                    actual->drawLine(screen_x-1, base_row + last_y, screen_x, base_row + y, YELLOW);                    
                }
                //actual->drawFastHLine(screen_x, base_row + y, 1, GREEN);
                last_y = y;
            }

            tft->setCursor(pos.x, pos.y + GRAPH_HEIGHT + 5);    // set cursor to below the graph's output

            /*tft->printf("Value: %i ", (int)(this->logged[ticks%LOOP_LENGTH_TICKS] * 100.0));
            tft->printf((char*)"Inp: %-8s ", (char*)this->parameter_input->getInputInfo()); //i @ %p")
            tft->printf((char*)"Read: %-8s ", (char*)this->parameter_input->getInputValue());*/
            //tft->printf((char*)"Tgt: %-15s\n", (char*)get_label_for_index(actual_value_index));
            //tft->printf((char*)"Val: %-7s ",  (char*)this->parameter_input->getFormattedValue());

            return tft->getCursorY();
        }

};


/*template<class TargetClass, class DataType = byte>
class InputTypeSelectorControl : public SelectorControl {
    public:

    int *available_values[2] = { (int)BIPOLAR, (int)UNIPOLAR };

    void(TargetClass::*setter_func)(DataType value) = nullptr;// setter_func;
    DataType(TargetClass::*getter_func)() = nullptr;// setter_func;
    TargetClass *target = nullptr;

    InputTypeSelectorControl(char *label) : SelectorControl(label) {}
    //InputTypeSelectorControl(char *label, byte initial_selection) : SelectorControl(label, initial_selection) {}
    InputTypeSelectorControl(char *label, TargetClass *target, void(TargetClass::*setter_func)(DataType), DataType(TargetClass::*getter_func)() ) 
        : InputTypeSelectorControl(label) 
    {
            this->target = target;
            this->getter_func = getter_func;
            this->setter_func = setter_func;
            this->selected_value_index = (this->target->*getter_func)();

            //this->available_values = { 0, 1 };
            this->num_values = sizeof(this->available_values);
    }



    virtual const char *get_label_for_value(byte value) {
        if (value==BIPOLAR)
            return (const char*)"Bipolar";
        else if (value==UNIPOLAR)
            return (const char*)"Unipolar";
        else
            return (const char*)"Unknown"; 
    };
};*/


class InputTypeSelectorControl : public SelectorControl {
    int actual_value_index;
    //void (*setter_func)(MIDIOutputWrapper *midi_output);
    //MIDIOutputWrapper *initial_selected_output_wrapper;

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
        //clock_mode = new_value;
        *target = new_value;
        actual_value_index = new_value;
        //selected_value_index = clock_mode;
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
                bool is_current_value_selected = i==current_value;
                int col = is_current_value_selected ? GREEN : C_WHITE;
                colours(opened && selected_value_index==i, col, BLACK);
                tft->setCursor(pos.x, pos.y);
                tft->printf((char*)"%s\n", (char*)get_label_for_index(i));
                pos.y = tft->getCursorY();
            }
            if (tft->getCursorX()>0) // if we haven't wrapped onto next line then do it manually
                tft->println((char*)"");
        }
        return tft->getCursorY();
    }

    virtual bool button_select() {
        this->setter(selected_value_index);

        char msg[255];
        //Serial.printf("about to build msg string...\n");
        sprintf(msg, "Set type to %i: %s", selected_value_index, get_label_for_index(selected_value_index));
        msg[tft->get_c_max()] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg, GREEN);
        return false;
    }

};


#endif