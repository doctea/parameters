#include "ADS1X15.h"

extern ADS1115 ADS_OBJECT;
extern int ads_values[4];

void setup_ads();

int read_value(int channel);
float read_voltage(int channel);
double get_frequency_for_pitch(int pitch);
int get_midi_pitch_for_voltage(float voltageFromAdc, int pitch_offset = 24);
double get_frequency_for_voltage(float voltageFromAdc, int pitch_offset = 24);