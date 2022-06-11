#include <Arduino.h>
#include "midi_helpers.h"

const char* get_note_namec(int pitch) {
  static char value[7] = "      ";
  const String note_names[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
  };

  if (pitch==-1 || pitch>127) {
    strcpy(value, "_");
    //String s = "_"; //note_names[chromatic_degree] + String(octave);
    //return s;
    return value;
  }
  int octave = pitch / 12;
  int chromatic_degree = pitch % 12;

  String s = note_names[chromatic_degree] + String(octave);
  //sprintf(value, "%s", s.c_str());
  //return s;
  strcpy(value, s.c_str());
  return value; 
}