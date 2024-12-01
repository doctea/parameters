#pragma once

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

#include "../parameter_inputs/ParameterInput.h"

#include <LinkedList.h>


#include "bpm.h"    // because we need to know the current ticks

//template<unsigned long memory_size>
class ParameterInputCombinedDisplay : public MenuItem {
    public:
        //ParameterInputDisplay *displays[3] = { nullptr, nullptr, nullptr };
        LinkedList<ParameterInputDisplay*> *displays = new LinkedList<ParameterInputDisplay*>();

        int graph_height = 50;

        ParameterInputCombinedDisplay(char *label, int total_height = 150, ParameterInputDisplay *item1 = nullptr, ParameterInputDisplay *item2 = nullptr, ParameterInputDisplay *item3 = nullptr)
            : MenuItem(label) {
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

            int graph_height = (this->graph_height - pos.y) / displays->size();

            for (int i = 0 ; i < displays->size() ; i++) {
                if (displays->get(i)!=nullptr) {
                    pos.y = displays->get(i)->draw_graph(pos, graph_height);
                }
            }

            return pos.y;
        }

};