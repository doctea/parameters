#ifndef MENU_ENVELOPEGRAPH_VIEW_MENUITEMS__INCLUDED
#define MENU_ENVELOPEGRAPH_VIEW_MENUITEMS__INCLUDED

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

//#include "../outputs/envelopes.h"

#include <LinkedList.h>

#ifndef PARAMETER_INPUT_GRAPH_HEIGHT
    #define PARAMETER_INPUT_GRAPH_HEIGHT 50
#endif

#ifndef ENVELOPE_GRAPH_MAX_SCREEN_WIDTH
    #define ENVELOPE_GRAPH_MAX_SCREEN_WIDTH 320
#endif

#ifndef ENVELOPE_GRAPH_ENABLE_CACHE
    #define ENVELOPE_GRAPH_ENABLE_CACHE 1
#endif

#include "bpm.h"    // because we need to know the current ticks

class EnvelopeDisplay : public MenuItem {
    //static constexpr
    int16_t stage_colours[7] = {
        (int16_t)0x8080,
        (int16_t)GREEN,
        (int16_t)YELLOW,
        (int16_t)ORANGE,
        (int16_t)RED,
        (int16_t)PURPLE,
        (int16_t)BLUE
    };

    #if ENVELOPE_GRAPH_ENABLE_CACHE
        int cached_width = -1;
        bool cached_invert = false;
        uint32_t cached_graph_revision = 0;
        bool projection_cache_valid = false;
        uint8_t cached_y_for_x[ENVELOPE_GRAPH_MAX_SCREEN_WIDTH];
        stage_t cached_stage_for_x[ENVELOPE_GRAPH_MAX_SCREEN_WIDTH];
    #endif

    #if ENVELOPE_GRAPH_ENABLE_CACHE
    void rebuild_projection_cache(const int graph_width, const int graph_height, const bool invert) {
        for (int screen_x = 0 ; screen_x < graph_width ; screen_x++) {
            const uint16_t tick_for_screen_X = ((uint32_t)screen_x * EnvelopeBase::GRAPH_SIZE) / graph_width;

            const uint8_t value_u8 = envelope->graph[tick_for_screen_X].value;
            cached_stage_for_x[screen_x] = envelope->graph[tick_for_screen_X].stage;

            // Graph values are already inversion-aware, but keep this dependency explicit for cache invalidation.
            (void)invert;
            cached_y_for_x[screen_x] = graph_height - ((value_u8 * graph_height + 127) / 255);
        }
    }
    #endif

    public:
        EnvelopeBase *envelope = nullptr;

        EnvelopeDisplay(const char *label, EnvelopeBase *envelope) : MenuItem(label, false) {
            this->envelope = envelope;
        }

        virtual void configure(EnvelopeBase *envelope) {
            this->envelope = envelope;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            tft->setTextSize(0);

            pos.y = tft->getCursorY();

            envelope->recalculate_graph_if_necessary();

            const uint16_t base_row = pos.y;
            const bool invert = envelope->is_invert();
            const int graph_height = PARAMETER_INPUT_GRAPH_HEIGHT;
            const int graph_width = tft->width();

            #if ENVELOPE_GRAPH_ENABLE_CACHE
                const uint32_t graph_revision = envelope->get_graph_revision();
                const bool can_use_projection_cache = graph_width <= ENVELOPE_GRAPH_MAX_SCREEN_WIDTH;
            #else
                const bool can_use_projection_cache = false;
            #endif

            if (can_use_projection_cache) {
                #if ENVELOPE_GRAPH_ENABLE_CACHE
                if (!projection_cache_valid ||
                    cached_width != graph_width ||
                    cached_invert != invert ||
                    cached_graph_revision != graph_revision) {
                    rebuild_projection_cache(graph_width, graph_height, invert);
                    cached_width = graph_width;
                    cached_invert = invert;
                    cached_graph_revision = graph_revision;
                    projection_cache_valid = true;
                }
                #endif
            }

            // todo: see if we can omptimise this by drawing in the "direction" of the framebuffer axis (ie, horizontal lines instead of vertical, from the point of view of the framebuffer)
            int last_y = 0;
            for (int screen_x = 0 ; screen_x < graph_width ; screen_x++) {
                const uint8_t y = can_use_projection_cache
                    ? cached_y_for_x[screen_x]
                    : (graph_height - ((envelope->graph[((uint32_t)screen_x * EnvelopeBase::GRAPH_SIZE) / graph_width].value * graph_height + 127) / 255));
                const stage_t stage = can_use_projection_cache
                    ? cached_stage_for_x[screen_x]
                    : envelope->graph[((uint32_t)screen_x * EnvelopeBase::GRAPH_SIZE) / graph_width].stage;
                if (screen_x != 0) {
                    const uint16_t colour = stage_colours[stage];
                    // Draw edge segment only when needed to reduce expensive generic line draws.
                    if (y != last_y)
                        tft->drawLine(screen_x-1, base_row + last_y, screen_x, base_row + y, colour);
                    if (invert)
                        tft->drawFastVLine(screen_x, base_row, y + 1, colour);
                    else
                        tft->drawFastVLine(screen_x, base_row + y, graph_height - y + 1, colour);
                }
                //actual->drawFastHLine(screen_x, base_row + y, 1, GREEN);
                last_y = y;
            }

            // draw a horizontal line representing the current envelope level
            if (envelope->last_state.stage!=0) {
                const int y = invert ? 
                    (envelope->last_state.lvl_now * graph_height)
                    :
                    graph_height - (envelope->last_state.lvl_now * graph_height);
                tft->drawFastHLine(0, base_row + y, graph_width, stage_colours[envelope->last_state.stage]);
            }

            tft->setCursor(pos.x, pos.y + PARAMETER_INPUT_GRAPH_HEIGHT + 5);    // set cursor to below the graph's output

            //if (this->parameter_input!=nullptr && this->parameter_input->hasExtra())
            //    tft->printf((char*)"Extra: %s\n", (char*)this->parameter_input->getExtra());

            return tft->getCursorY();
        }
};

class EnvelopeIndicator : public MenuItem {
    public:
    EnvelopeBase *envelope = nullptr;

    //static constexpr 
    const char *stage_labels[6] = {
        "Off",
        "Attack",
        "Hold",
        "Decay",
        "Sustain",
        "Release"
    };

    EnvelopeIndicator(const char *label, EnvelopeBase *envelope) : MenuItem(label, false) {
        this->envelope = envelope;
    }

    virtual int display(Coord pos, bool selected, bool opened) override {
        tft->printf("%s | ", (char*)envelope->label);
        //tft->printf("   CC: %i\n", envelope->midi_cc);
        tft->printf("Stg: %7s | ", (char*)stage_labels[envelope->last_state.stage]);
        //tft->printf("Trig'd at: %-5i | ", envelope->stage_triggered_at);
        //tft->printf("Elapsed: %-5i\n", envelope->last_state.elapsed);
        char buf[5];
        snprintf(buf, 10, "%-1.2f", envelope->last_sent_actual_lvl);
        tft->printf("Lvl: %s\n", buf);
        
        return tft->getCursorY();
    }
};

#endif
