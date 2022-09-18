#ifndef MENU_PARAMETERINPUT_VIEW_MENUITEMS__INCLUDED
#define MENU_PARAMETERINPUT_VIEW_MENUITEMS__INCLUDED

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

#include "../parameter_inputs/ParameterInput.h"

#include <LinkedList.h>

// this stuff ripped from midi_looper.h/menu_looperdisplay.h in usb_midi_clocker TODO: make more generic
#define LOOP_LENGTH_STEP_SIZE 1         // resolution of loop TODO: problems when this is set >1; reloaded sequences (or maybe its converted-from-bitmap stage?) are missing note-offs
#define ticks_to_sequence_step(X)  ((X % LOOP_LENGTH_TICKS) / LOOP_LENGTH_STEP_SIZE)

class ViewParameterMenuItem : public MenuItem {
    public:
        BaseParameterInput *parameter_input = nullptr;

        double logged[LOOP_LENGTH_TICKS];

        ViewParameterMenuItem(char *label, BaseParameterInput *input) : MenuItem(label) {
        //ViewParameterMenuItem(char *label, BaseParameterInput *input) : MenuItem(label) {
            this->parameter_input = input;
            //this->loop_length_ticks = loop_length_ticks;
            //double logged
            /*if (loop_length_ticks>0) {
                double logged[] = new double[loop_length_ticks];
            }*/
        }

        virtual void configure(BaseParameterInput *parameter_input) {
            this->parameter_input = parameter_input;
        }

        virtual void update_ticks(unsigned long ticks) {
            // update internal log of values
            unsigned int position = ticks % LOOP_LENGTH_TICKS;
            logged[position] = this->parameter_input->get_normal_value(); //get_voltage_normal();
            if (this->debug) Serial.printf("\tupdate_ticks(%i) recorded %f\n", position, logged[position]);
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            //Serial.println("MidiOutputSelectorControl display()!");
            tft->setTextSize(0);

            pos.y = header(label, pos, selected, opened);          
            tft->setCursor(pos.x, pos.y);
            colours(false, C_WHITE, BLACK);
            tft->printf("Input %c\n", parameter_input->name);
            pos.y = tft->getCursorY();

            const uint16_t base_row = pos.y;
            static float ticks_per_pixel = (float)LOOP_LENGTH_TICKS / (float)tft->width();

            // we're going to use direct access to the underlying Adafruit library here
            const DisplayTranslator_STeensy *tft2 = (DisplayTranslator_STeensy*)tft;
            ST7789_t3 *actual = tft2->tft;

            int GRAPH_HEIGHT = 50;

            int last_y = 0;
            for (int screen_x = 0 ; screen_x < tft->width() ; screen_x++) {
                const uint16_t tick_for_screen_X = ticks_to_sequence_step((int)((float)screen_x * ticks_per_pixel)); // the tick corresponding to this screen position
                int y = GRAPH_HEIGHT - (this->logged[tick_for_screen_X] * GRAPH_HEIGHT);
                if (screen_x != 0) {
                    //int last_y = GRAPH_HEIGHT - (this->logged[tick_for_screen_X] * GRAPH_HEIGHT);
                    actual->drawLine(screen_x-1, base_row + last_y, screen_x, base_row + y, YELLOW);                    
                }
                actual->drawFastHLine(screen_x, base_row + y, 1, GREEN);
                last_y = y;
            }

            tft->setCursor(pos.x, pos.y + GRAPH_HEIGHT + 5);
            tft->printf("Value: %i\n", (int)(this->logged[ticks%LOOP_LENGTH_TICKS] * 100.0));
            tft->printf((char*)"Inp: %-15s\n", (char*)this->parameter_input->getInputInfo()); //i @ %p")
            tft->printf((char*)"Read: %-8s\n", (char*)this->parameter_input->getInputValue());
            //tft->printf((char*)"Tgt: %-15s\n", (char*)get_label_for_index(actual_value_index));
            tft->printf((char*)"Val: %-7s\n",  (char*)this->parameter_input->getFormattedValue());

            return tft->getCursorY();
        }

};

#endif