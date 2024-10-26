#pragma once

//#include "ADS1X15.h"

#include <Arduino.h>

#define MIDI_NOTE_A440 (69)

//extern ADS1115 ADS_OBJECT;
//extern int ads_values[4];
//extern float current_adc_voltage[4];

//void setup_ads();

/*int read_value(int channel);
float read_voltage(int channel);*/

float get_frequency_for_pitch(int8_t pitch, int pitch_offset = MIDI_NOTE_A440);
int8_t get_midi_pitch_for_voltage(float voltageFromAdc, int8_t pitch_offset = 24);
float get_frequency_for_voltage(float voltageFromAdc, int8_t pitch_offset = 24);
//float get_voltage_for_frequency(float frequency, int pitch_offset = 36);

//void update_adc();

//void read_adc_voltages();