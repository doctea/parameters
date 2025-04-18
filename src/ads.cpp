#include "Arduino.h"

#include "ads.h"
//#include "mtof.h"

#include "midi_helpers.h"

#ifndef PIN_SDA
  #define PIN_SDA SDA
#endif
#ifndef PIN_SCL
  #define PIN_SCL SCL
#endif

int sdaPin = PIN_SDA; //PIN_PB4;
int sclPin = PIN_SCL; //PIN_PB2;

//ADS1115 ADS_OBJECT(0x48);

float scaler = 0.97;

float current_adc_voltage[4] = { 0, 0, 0, 0, };

/*void update_adc() {
  read_adc_voltages();
}

void read_adc_voltages() {
  for (unsigned int i = 0 ; i < 4 ; i++) {
    int intermediate = ADS_OBJECT.readADC(i);
    float currentValue = ADS_OBJECT.toVoltage(intermediate);
    current_adc_voltage[i] =  currentValue;
  }
}

int16_t ads_values[4] = { 0, 0, 0, 0 };*/

/*
void setup_ads() {
  Serial.println(F("initiating ADS_OBJECT..")); Serial_flush();
  ADS_OBJECT.begin();
  Serial.println(F("\tfinished begin()")); Serial_flush();
  ADS_OBJECT.setGain(0);

  //ADS_OBJECT.requestADC(0);    // start continuous reads ?
  //ADS_OBJECT.requestADC(1);    // start continuous reads ?
  Serial.println(F("Finished initialising ADS_OBJECT!")); Serial_flush();
}

int read_value(int channel = 0) {
  static int value;
  //ADS_OBJECT.requestADC(channel);
  //if (ADS_OBJECT.isReady()) {
    //value = ADS_OBJECT.getValue();
    value = ADS_OBJECT.readADC(channel);
  ads_values[channel] = value;
  //}
  return value;
  //return ADS_OBJECT.getLastValue();
}

float read_voltage(int channel) {
  float v = ADS_OBJECT.toVoltage(read_value(channel));
  if ((int)v==ADS1X15_INVALID_VOLTAGE)
    return 0.0;
  return v;
}
*/
/*
float get_corrected_voltage (float voltageFromAdc) {
  // TODO: what is the maths behind this?  make configurable, etc 
  // from empirical measuring of received voltage and asked wolfram alpha to figure it out:-
  //  1v: v=1008        = 0.99206349206
  //  2v: v=2031-2034   = 0.98473658296
  //  3v: v=3060        = 0.98039215686
  //  4v: v=4086-4089   = 0.9789525208
  // https://www.wolframalpha.com/input?i=linear+fit+%7B%7B1.008%2C1%7D%2C+%7B2.034%2C2%7D%2C+%7B3.063%2C3%7D%2C+%7B4.086%2C4%7D%2C+%7B5.1%2C5%7D%7D
  return (voltageFromAdc * 0.976937) + 0.0123321;
};
*/

int8_t get_midi_pitch_for_voltage(float voltageFromAdc, int8_t pitch_offset) {
  //int pitch = pitch_offset + round(/*scaler **/get_corrected_voltage(voltageFromAdc) * 12.0);
  int8_t pitch = constrain(pitch_offset + round(voltageFromAdc * 12.0f), MIDI_MIN_NOTE, MIDI_MAX_NOTE);
  return pitch;
}

float get_frequency_for_voltage(float voltageFromAdc, int8_t pitch_offset) { // was 36
  // get the tuning root -- Keystep is C1=1.0v, so start on C
  // TODO: configurable tuning from different note / 1.0v = A mode
  float base_freq = get_frequency_for_pitch(pitch_offset);
  //Serial.printf("base freq: %u\t", (uint16_t)base_freq);
  //Serial.printf("v=%u, adjv=%u\t", (uint16_t)(voltageFromAdc*1000.0), (uint16_t)(adjusted_voltage*1000.0));
  float freq = base_freq * (pow(2.0, voltageFromAdc));
  //Serial.printf("becomes frequency %u\n", (uint16_t)freq);
  return freq;
}

// todo: turn this into a lookup table
float get_frequency_for_pitch(int8_t pitch, int8_t base_pitch) {
  //float freq = mtof.toFrequency((float)pitch);
  // tune from 440hz
  int8_t p = constrain(pitch - base_pitch, MIDI_MIN_NOTE, MIDI_MAX_NOTE);
  float freq = 440.0 * pow(2.0, ((float)(p) / 12.0));
  //float freq = 55.0 * pow(2.0, ((float)(p) / 12.0));
  //Serial.printf("get_frequency_for_pitch(%u) return freq %u\n", pitch, (freq));
  return freq;
}
