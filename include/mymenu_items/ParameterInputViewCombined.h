#pragma once

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

#include "../parameter_inputs/ParameterInput.h"
#include "../parameter_inputs/VoltageParameterInput.h"

#include <LinkedList.h>

#include "bpm.h"    // because we need to know the current ticks

#include "midi_helpers.h"

class ParameterInputCombinedDisplay : public MenuItem {
    public:
        //ParameterInputDisplay *displays[3] = { nullptr, nullptr, nullptr };
        LinkedList<ParameterInputDisplay*> *displays = new LinkedList<ParameterInputDisplay*>();    // TODO: can probably save a bit of RAM and CPU by changing this to an array or a MenuItemList

        int graph_height = 50;

        ParameterInputCombinedDisplay(char *label, int total_height = 150, ParameterInputDisplay *item1 = nullptr, ParameterInputDisplay *item2 = nullptr, ParameterInputDisplay *item3 = nullptr, bool selectable = false, bool show_header = false)
            : MenuItem(label, selectable, show_header) {
                if (item1!=nullptr) displays->add(item1);
                if (item2!=nullptr) displays->add(item2);
                if (item3!=nullptr) displays->add(item3);
                this->graph_height = total_height;
                this->add_redraw_policy(REDRAW_ON_TICK);
            }

        /*virtual void configure(ParameterInputDisplay *item1, ParameterInputDisplay *item2, ParameterInputDisplay *item3) {
            this->parameter_input = parameter_input;
        }*/

        virtual void add_parameter_input_display(ParameterInputDisplay *item) {
            this->displays->add(item);
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            //Serial.println("MidiOutputSelectorControl display()!");
            pos.y = this->header(label, pos, selected, opened);

            for (auto* d : *displays) {
                if (d!=nullptr) {
                    tft->setCursor(pos.x, pos.y);
                    this->colours(false, d->default_fg);
                    if (d->parameter_input->supports_pitch()) {
                        //tft->printf("%s ", (char*)d->parameter_input->getInputValue());
                        BaseParameterInput *input = d->parameter_input;
                        float voltage_value = input->get_voltage();
                        //Serial.printf("voltage_value=%3.3f\n", voltage_value);
                        char info[MENU_C_MAX];
                        snprintf(info,
                            MENU_C_MAX, 
                            "% 3.3f %3s ",
                            voltage_value,
                            (char*)get_note_name_c(input->get_voltage_pitch())
                        );
                        tft->printf(info);
                    } else {
                        tft->printf("%3s ", (char*)d->parameter_input->getInputValue());
                    }
                    tft->printf("| ");
                    pos.x = tft->getCursorX();
                }
            }
            tft->println();
            pos.y = tft->getCursorY();
            pos.x = 0;

            const uint_fast16_t graph_height = (this->graph_height - pos.y) / displays->size();
            const uint_fast16_t screen_width = tft->width();

            for (auto* d : *displays) {
                if (d!=nullptr) {
                    pos.y = d->draw_graph(pos, screen_width, graph_height);
                }
                tft->drawLine(0, pos.y, screen_width, pos.y, GREY);
            }

            return pos.y;
        }

};