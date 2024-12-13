#pragma once

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

#include "../parameter_inputs/ParameterInput.h"

#include <LinkedList.h>

#include "bpm.h"    // because we need to know the current ticks

#ifndef PARAMETER_INPUT_GRAPH_HEIGHT
    #define PARAMETER_INPUT_GRAPH_HEIGHT 50
#endif

//template<unsigned long memory_size>
class ParameterInputDisplay : public MenuItem
#ifdef PARAMETER_INPUTS_USE_CALLBACKS
    , public ParameterInputCallbackReceiver 
#endif
{
    public:
        BaseParameterInput *parameter_input = nullptr;

        // todo: remember an int type instead of a float, for faster drawing and reduced memory use
        typedef float memory_log;
        unsigned long memory_size;
        memory_log *logged = nullptr;

        int graph_height = PARAMETER_INPUT_GRAPH_HEIGHT;

        ParameterInputDisplay(char *label, unsigned long memory_size, BaseParameterInput *input, int graph_height = PARAMETER_INPUT_GRAPH_HEIGHT) : MenuItem(label) {
            this->parameter_input = input;
            this->memory_size = memory_size;
            this->selectable = !input->supports_bipolar_input();
            if (parameter_input!=nullptr) 
                this->set_default_colours(parameter_input->colour);

            this->graph_height = graph_height;

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
                this->receive_value_update(this->parameter_input->get_normal_value_unipolar());
            }
        #endif

        uint_fast16_t last_position_updated;
        virtual void receive_value_update(float value) {
            uint_fast16_t position = ticks_to_memory_step(ticks);

            if (position==last_position_updated)
                return;

            // convert value according to the input/output settings
            /*if (this->parameter_input->output_type==BIPOLAR) {
                // center is 0, range -1 to +1, so re-center display
                (logged)[position] = (0.5) + (value / 2);
            } else if (this->parameter_input->output_type==UNIPOLAR) {*/
                // center is 0.5, range 0 to 1.. dont modif 
                (logged)[position] = value;
            //}

            // do a simple backfill of values we missed
            if (last_position_updated < position && (last_position_updated) - position > 1) {
                for (uint_fast16_t i = last_position_updated+1 ; i < position ; i++)
                    (logged)[i] = (logged)[position];
            }

            last_position_updated = position;
        }

        int16_t halfbright_colour = 0;

        virtual int draw_graph(Coord pos, int graph_height) {
            // switch back to colour-on-black for actual display
            colours(false, parameter_input->colour, BLACK);

            if (this->halfbright_colour==0)
                this->halfbright_colour = tft->halfbright_565(this->default_fg);

            const int_fast16_t base_row = pos.y;
            static float ticks_per_pixel = (float)memory_size / (float)tft->width();

            // draw a grey line at the "zero" position
            int_fast16_t zero_position_y = parameter_input->input_type==BIPOLAR ? graph_height/2 : graph_height;
            tft->drawLine(0, base_row + zero_position_y, tft->width(), base_row + zero_position_y, halfbright_colour);

            int_fast16_t last_y = 0;
            for (int screen_x = 0 ; screen_x < tft->width() ; screen_x++) {
                const int_fast16_t tick_for_screen_X = ticks_to_memory_step((int)((float)screen_x * ticks_per_pixel)); // the tick corresponding to this screen position
                const float value = (logged)[tick_for_screen_X];
                const int_fast16_t y = graph_height - (value * graph_height);
                if (screen_x != 0) {
                    //int last_y = GRAPH_HEIGHT - (this->logged[tick_for_screen_X] * GRAPH_HEIGHT);
                    tft->drawLine(screen_x-1, base_row + last_y, screen_x, base_row + y, parameter_input->colour);                    
                }
                last_y = y;
            }

            pos.y = pos.y + graph_height + 5;

            tft->setCursor(pos.x, pos.y);    // set cursor to below the graph's output

            return pos.y;
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
                    this->parameter_input!=nullptr ? (int)(this->parameter_input->get_normal_value_unipolar()*100.0) : 0, 
                    (char*)this->parameter_input->getOutputValue()
                );
                colours(selected, parameter_input->colour, BLACK);
                pos.y = header(custom_label, pos, selected, opened);         
            #else 
                pos.y = header(label, pos, selected, opened);                      
                //tft->setCursor(pos.x, pos.y);
            #endif
            pos.y = tft->getCursorY();

            pos.y = draw_graph(pos, graph_height);

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

