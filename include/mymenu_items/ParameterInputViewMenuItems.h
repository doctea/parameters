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

// Buffer sample count for the history log.
// Default (undefined): set at runtime to the screen width — one slot per pixel.
// Set to a larger power-of-2 value (e.g. 1024) on RAM-rich targets for higher graph resolution.
// The write path scales tick-in-phrase → [0, memory_size); the draw path scales back to screen pixels.
#ifndef PARAMETER_INPUT_MEMORY_LOG_SIZE
    // resolved at runtime in on_add() — see below
#endif

#include "bpm.h"

//#define PARAMETER_INPUT_MEMORY_LOG_SIZE (TIME_SIG_MAX_STEPS_PER_BAR * TICKS_PER_BAR)

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
            // Buffer is screen-width sized: one slot per pixel column.
            // This is fixed at construction and never changes, avoiding OOB writes when
            // the time signature (and thus TICKS_PER_PHRASE) changes at runtime.
            this->selectable = !input->supports_bipolar_input();
            if (parameter_input!=nullptr) 
            this->set_default_colours(parameter_input->colour);
            
            this->graph_height = graph_height;
        }
        
        virtual void on_add() override {
            MenuItem::on_add();
            
            #ifdef PARAMETER_INPUT_MEMORY_LOG_SIZE
                this->memory_size = PARAMETER_INPUT_MEMORY_LOG_SIZE;
            #else
                // TODO: more robust SCREEN_ROTATION handling, since might mean different things on different displays/libraries
                this->memory_size = SCREEN_ROTATION%2==0 ? TFT_WIDTH : TFT_HEIGHT;
            #endif
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

        // Map a tick value to a pixel-column buffer index.
        // Scales tick-within-phrase to [0, memory_size), so one full phrase = one screen width,
        // regardless of the current time signature.
        unsigned long ticks_to_memory_step(uint32_t tick) {
            uint32_t tick_in_phrase = tick % TICKS_PER_PHRASE;
            return ((uint32_t)tick_in_phrase * memory_size) / TICKS_PER_PHRASE;
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

        virtual int draw_graph(Coord pos, int graph_width, int graph_height) {
            return draw_graph(pos.x, pos.y, graph_width, graph_height);
        }
        
        virtual int draw_graph(int pos_x, int pos_y, int graph_width, int graph_height) {
            // switch back to colour-on-black for actual display
            colours(false, parameter_input->colour, BLACK);

            if (this->halfbright_colour==0)
                this->halfbright_colour = tft->halfbright_565(this->default_fg);

            const int_fast16_t base_row = pos_y;
            const uint16_t screen_width = graph_width;

            // draw a halfbright line at the "zero" position
            int_fast16_t zero_position_y = parameter_input->input_type==BIPOLAR ? graph_height/2 : graph_height;
            tft->drawLine(pos_x, base_row + zero_position_y, pos_x + graph_width, base_row + zero_position_y, halfbright_colour);

            // current pixel position within the phrase, for past/future colour split
            // const uint32_t current_pixel = ((uint32_t)(ticks % TICKS_PER_PHRASE) * screen_width) / TICKS_PER_PHRASE;

            const float translation_multiplier = (float)memory_size / screen_width;
            const uint32_t current_buffer_index = ticks_to_memory_step(ticks);

            int_fast16_t last_y = 0;
            for (uint16_t screen_x = pos_x ; screen_x < pos_x + screen_width ; screen_x++) {
                uint32_t buffer_index;
                if (memory_size == screen_width) {
                    // Buffer is screen-width: one slot per pixel, no scaling needed
                    buffer_index = screen_x;                   
                } else {
                    // Buffer is not screen-width: need to scale pixel column to buffer index
                    buffer_index = (uint32_t)((screen_x - pos_x) * translation_multiplier);
                }
                const float value = decode_memory_log(logged[buffer_index]);

                const int_fast16_t y = graph_height - (int_fast16_t)(value * graph_height);
                if (screen_x != 0) {
                    //uint16_t colour = screen_x < current_pixel ? parameter_input->colour : halfbright_colour;
                    uint16_t colour = buffer_index < current_buffer_index ? parameter_input->colour : halfbright_colour;
                    tft->drawLine(screen_x-1, base_row + last_y, screen_x, base_row + y, colour);
                }
                last_y = y;
            }

            pos_y = pos_y + graph_height + 5;

            tft->setCursor(pos_x, pos_y);    // set cursor to below the graph's output

            return pos_y;
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

            pos.y = draw_graph(pos, tft->width(), graph_height);

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

