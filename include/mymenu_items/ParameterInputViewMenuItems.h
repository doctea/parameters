#pragma once

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

#include "../parameter_inputs/ParameterInput.h"

#include <LinkedList.h>

#include "bpm.h"    // because we need to know the current ticks

#ifndef CALLOC_FUNC
    #define CALLOC_FUNC calloc
#endif

#ifndef PARAMETER_INPUT_GRAPH_HEIGHT
    #define PARAMETER_INPUT_GRAPH_HEIGHT 50
#endif

// Configurable per-project: storage type for graph/history buffer.
// uint8_t  = 255 levels,  1 byte/tick — sufficient for a 50px graph, minimum RAM
// uint16_t = 65535 levels, 2 bytes/tick — better fidelity, use on RAM-rich targets
#ifndef PARAMETER_INPUT_MEMORY_LOG_TYPE
    #define PARAMETER_INPUT_MEMORY_LOG_TYPE uint8_t
#endif
#ifndef PARAMETER_INPUT_MEMORY_LOG_MAX
    #define PARAMETER_INPUT_MEMORY_LOG_MAX 255u
#endif

// Configurable per-project: number of history slots in the buffer.
// TICKS_PER_PHRASE gives one slot per tick (best quality).
// tft->width() gives one slot per pixel (minimum RAM, some quality loss).
#ifndef PARAMETER_INPUT_MEMORY_SIZE
    #define PARAMETER_INPUT_MEMORY_SIZE TICKS_PER_PHRASE
#endif


class ParameterInputDisplay : public MenuItem
#ifdef PARAMETER_INPUTS_USE_CALLBACKS
    , public ParameterInputCallbackReceiver 
#endif
{
    public:
        BaseParameterInput *parameter_input = nullptr;

        // Fixed-point history buffer. Type and size are configurable — see defines above.
        typedef PARAMETER_INPUT_MEMORY_LOG_TYPE memory_log;
        static constexpr uint32_t memory_log_max = PARAMETER_INPUT_MEMORY_LOG_MAX;
        unsigned long memory_size;
        memory_log *logged = nullptr;

        int graph_height = PARAMETER_INPUT_GRAPH_HEIGHT;

        ParameterInputDisplay(char *label, BaseParameterInput *input, int graph_height = PARAMETER_INPUT_GRAPH_HEIGHT) : MenuItem(label) {
            this->parameter_input = input;
            // Buffer is tick-resolution: one entry per tick in the phrase.
            // This preserves sharp peaks and fast transitions.
            // RAM cost is half that of float: TICKS_PER_PHRASE * 2 bytes per display.
            this->memory_size = PARAMETER_INPUT_MEMORY_SIZE;
            this->selectable = !input->supports_bipolar_input();
            if (parameter_input!=nullptr) 
                this->set_default_colours(parameter_input->colour);

            this->graph_height = graph_height;

            logged = (memory_log*)CALLOC_FUNC(this->memory_size, sizeof(memory_log));
        }

        // Encode float [0,1] to uint16_t
        inline memory_log encode_memory_log(float value) const {
            if (value <= 0.0f)
                return 0;
            if (value >= 1.0f)
                return (memory_log)memory_log_max;
            return (memory_log)(value * (float)memory_log_max + 0.5f);
        }

        // Decode uint16_t to float [0,1]
        inline float decode_memory_log(memory_log value) const {
            return (float)value / (float)memory_log_max;
        }

        virtual void configure(BaseParameterInput *parameter_input) {
            this->parameter_input = parameter_input;
        }

        // Map a tick value to a buffer index (one entry per tick in the phrase)
        unsigned long ticks_to_memory_step(uint32_t ticks) {
            return ticks % TICKS_PER_PHRASE;
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

        // UINT16_MAX is the sentinel meaning "not yet updated"
        uint_fast16_t last_position_updated = UINT16_MAX;
        memory_log last_logged_value = 0;
        virtual void receive_value_update(float value) {
            uint_fast16_t position = ticks_to_memory_step(ticks);

            if (position == last_position_updated)
                return;

            memory_log encoded = encode_memory_log(value);

            // Backfill any skipped positions with the LAST value (not the new value).
            // This correctly represents what happened during the skipped ticks.
            if (last_position_updated != UINT16_MAX && last_position_updated < position && (position - last_position_updated) > 1) {
                for (uint_fast16_t i = last_position_updated + 1 ; i < position ; i++)
                    logged[i] = last_logged_value;
            }

            // Write new value at current position
            logged[position] = encoded;
            last_logged_value = encoded;
            last_position_updated = position;
        }

        int16_t halfbright_colour = 0;

        virtual int draw_graph(Coord pos, int graph_height) {
            // switch back to colour-on-black for actual display
            colours(false, parameter_input->colour, BLACK);

            if (this->halfbright_colour==0)
                this->halfbright_colour = tft->halfbright_565(this->default_fg);

            const int_fast16_t base_row = pos.y;
            const uint16_t screen_width = tft->width();

            // draw a halfbright line at the "zero" position
            int_fast16_t zero_position_y = parameter_input->input_type==BIPOLAR ? graph_height/2 : graph_height;
            tft->drawLine(0, base_row + zero_position_y, screen_width, base_row + zero_position_y, halfbright_colour);

            // current tick position within the phrase, for past/future colour split
            const uint32_t current_tick_in_phrase = ticks % TICKS_PER_PHRASE;

            int_fast16_t last_y = 0;
            for (uint16_t screen_x = 0 ; screen_x < screen_width ; screen_x++) {
                // Map screen pixel to buffer tick: scale screen_x across TICKS_PER_PHRASE
                const uint32_t tick_for_x = ((uint32_t)screen_x * TICKS_PER_PHRASE) / screen_width;
                const float value = decode_memory_log(logged[tick_for_x]);
                const int_fast16_t y = graph_height - (int_fast16_t)(value * graph_height);
                if (screen_x != 0) {
                    uint16_t colour = tick_for_x < current_tick_in_phrase ? parameter_input->colour : halfbright_colour;
                    tft->drawLine(screen_x-1, base_row + last_y, screen_x, base_row + y, colour);
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

