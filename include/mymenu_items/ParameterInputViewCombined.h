#pragma once

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

#include "../parameter_inputs/ParameterInput.h"
#include "../parameter_inputs/VoltageParameterInput.h"

#include <LinkedList.h>

#include "bpm.h"    // because we need to know the current ticks

#include "midi_helpers.h"

//template<unsigned long memory_size>
class ParameterInputCombinedDisplay : public MenuItem {
    public:
        //ParameterInputDisplay *displays[3] = { nullptr, nullptr, nullptr };
        LinkedList<ParameterInputDisplay*> *displays = new LinkedList<ParameterInputDisplay*>();

        int graph_height = 50;

        ParameterInputCombinedDisplay(char *label, int total_height = 150, ParameterInputDisplay *item1 = nullptr, ParameterInputDisplay *item2 = nullptr, ParameterInputDisplay *item3 = nullptr, bool selectable = false, bool show_header = false)
            : MenuItem(label, selectable, show_header) {
                if (item1!=nullptr) displays->add(item1);
                if (item2!=nullptr) displays->add(item2);
                if (item3!=nullptr) displays->add(item3);
                this->graph_height = total_height;
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

            for (uint_fast8_t i = 0 ; i < displays->size() ; i++) {
                if (displays->get(i)!=nullptr) {
                    tft->setCursor(pos.x, pos.y);
                    this->colours(false, displays->get(i)->default_fg);
                    if (displays->get(i)->parameter_input->supports_pitch()) {
                        //tft->printf("%s ", (char*)displays->get(i)->parameter_input->getInputValue());
                        VoltageParameterInput *casted_input = (VoltageParameterInput*)displays->get(i)->parameter_input;
                        float voltage_value = casted_input->get_voltage();
                        //Serial.printf("voltage_value=%3.3f\n", voltage_value);
                        char info[MENU_C_MAX];
                        snprintf(info,
                            MENU_C_MAX, 
                            "% 3.3f %3s ",
                            voltage_value,
                            (char*)get_note_name_c(casted_input->get_voltage_pitch())
                        );
                        tft->printf(info);
                    } else {
                        tft->printf("%3s ", (char*)displays->get(i)->parameter_input->getInputValue());
                    }
                    tft->printf("| ");
                    pos.x = tft->getCursorX();
                }
            }
            tft->println();
            pos.y = tft->getCursorY();
            pos.x = 0;

            uint_fast16_t graph_height = (this->graph_height - pos.y) / displays->size();

            for (uint_fast8_t i = 0 ; i < displays->size() ; i++) {
                if (displays->get(i)!=nullptr) {
                    pos.y = displays->get(i)->draw_graph(pos, graph_height);
                }
            }

            return pos.y;
        }

};