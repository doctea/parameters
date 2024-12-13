#pragma once

#include <Arduino.h>

#define MIDI_NOTE_A440 (69)
#ifndef DEFAULT_CV_PITCH_OFFSET
    #define DEFAULT_CV_PITCH_OFFSET (24)
#endif

float get_frequency_for_pitch(int8_t pitch, int pitch_offset = MIDI_NOTE_A440);
int8_t get_midi_pitch_for_voltage(float voltageFromAdc, int8_t pitch_offset = DEFAULT_CV_PITCH_OFFSET);
float get_frequency_for_voltage  (float voltageFromAdc, int8_t pitch_offset = DEFAULT_CV_PITCH_OFFSET);
