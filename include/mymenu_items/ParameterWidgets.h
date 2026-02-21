#pragma once

#include "Arduino.h"

#include "menu.h"
#include "colours.h"

#include "../parameters/Parameter.h"

#include <LinkedList.h>

#include <menuitems_widget.h>

class ParameterWidget : public MenuItems_Widget {

    private:
        FloatParameter *parameter;

    public:
        ParameterWidget(const char *label, IconBase *image, FloatParameter *parameter) 
            : MenuItems_Widget(label, image) 
        {
            this->parameter = parameter;
        }

        // virtual int display(Coord pos, bool selected, bool opened) override {
        //     int height = MenuItems_Widget::display(pos, selected, opened);
        //     tft->setCursor(pos.x + 40, pos.y + 4);
        //     pos.y = tft->getCursorY();

        //     if (this->parameter!=nullptr) {
        //         tft->setCursor(pos.x + 40, pos.y + 4);
        //         tft->printf("value: %f\n", this->parameter->getCurrentNormalValue());
        //     }

        //     tft->setCursor(pos.x, pos.y + 20); // move the cursor to below the square for the next item to be drawn

        //     return tft->getCursorY();
        // }

        virtual int renderValue(bool selected, bool opened, uint16_t width) override {
            if (this->parameter != nullptr)
                this->default_fg = C_WHITE; //(*this->parameter)->connections[slot_number].parameter_input->colour;
            else
                this->default_fg = C_WHITE;
            int y = MenuItems_Widget::renderValue(selected, opened, width);

            if (this->parameter!=nullptr) {
                tft_print_column((char*)"%s\n", (char*)this->parameter->getFormattedLastOutputValue());

                for (int i = 0 ; i < 3 ; i++) {
                    ParameterToInputConnection connection = this->parameter->connections[i];
                    BaseParameterInput *input = connection.parameter_input;
                    if (input == nullptr || connection.amount == 0.0f) {
                        ///Serial.printf("Parameter '%s' has no input in slot %i\n", this->parameter->label, i);
                        continue;
                    }
                    colours(selected, input->colour);
                    //tft_print_column((char*)"%s: %-3i\n", input->name, (int)(connection.amount * 100.0f), 0); 
                    tft_print_column((char*)"%-3i%%\n", (int)(connection.amount * 100.0f)); 
                }
            }

            colours(this->default_fg, this->default_bg);

            tft->setCursor(tft->getCursorX(), tft->getCursorY() + 4); // move the cursor to below the square for the next item to be drawn

            return tft->getCursorY(); // - y;
        }

};