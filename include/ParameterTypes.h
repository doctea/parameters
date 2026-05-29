#pragma once

#include <stdint.h>
#include "bpm.h"

enum VALUE_TYPE {
  BIPOLAR,
  UNIPOLAR
};

// Modulation input interpretation mode per modulation slot.
// Keep 0/1 aligned with existing BIPOLAR/UNIPOLAR values for compatibility.
enum MODULATION_SLOT_MODE : uint8_t {
  MOD_SLOT_BI_NATIVE = BIPOLAR,
  MOD_SLOT_UNI_RAW = UNIPOLAR,
  MOD_SLOT_UNI_CENTERED = 2
};

// Slot mode semantics (all scaled so amount=1.0 represents full parameter span):
// - MOD_SLOT_UNI_RAW:     source 0..1, interpreted as one-sided positive modulation.
// - MOD_SLOT_UNI_CENTERED: source centered around 0 from unipolar input.
// - MOD_SLOT_BI_NATIVE:   source -1..1 from bipolar-capable input.
//
// The UI labels below are intentionally centralized so compact text can be tuned
// without touching menu/widget code.

// Compact UI labels for slot modulation modes (easy to redefine project-wide).
#ifndef MOD_SLOT_BI_NATIVE_LABEL
  #define MOD_SLOT_BI_NATIVE_LABEL "B+-"
#endif
#ifndef MOD_SLOT_UNI_RAW_LABEL
  #define MOD_SLOT_UNI_RAW_LABEL "U+ "
#endif
#ifndef MOD_SLOT_UNI_CENTERED_LABEL
  #define MOD_SLOT_UNI_CENTERED_LABEL "U+-"
#endif

// Set to 1 for lightweight runtime tracing of CV pitch-bend routing and applied values.
#ifndef CV_PITCH_BEND_TRACE
  #define CV_PITCH_BEND_TRACE 0
#endif

// Per-slot sample & hold mode.
// SH_OFF = no S&H (existing behaviour).
// Other values sample the modulation input once per clock division and hold the value until the next sample.
// Note: SH_BAR/SH_2BAR/SH_PHRASE depend on TICKS_PER_BAR/TICKS_PER_PHRASE which are runtime-variable
// when the time signature changes; this is a known limitation shared with VirtualParameterInput.sh_ticks.
enum SHMode : uint8_t {
    SH_OFF = 0,
    SH_32ND,
    SH_16TH,
    SH_16TH_TRIP,   // 16th note triplet = PPQN/6
    SH_8TH,
    SH_8TH_TRIP,    // 8th note triplet = PPQN/3
    SH_BEAT,
    SH_2BEAT,
    SH_BAR,
    SH_2BAR,
    SH_PHRASE,
    SH_MODE_COUNT   // sentinel; must stay last
};

inline uint32_t get_sh_ticks_for_mode(SHMode m) {
    switch (m) {
        case SH_32ND:       return PPQN / 8;
        case SH_16TH:       return PPQN / 4;
        case SH_16TH_TRIP:  return PPQN / 6;
        case SH_8TH:        return PPQN / 2;
        case SH_8TH_TRIP:   return PPQN / 3;
        case SH_BEAT:       return PPQN;
        case SH_2BEAT:      return (uint32_t)PPQN * 2;
        case SH_BAR:        return (uint32_t)TICKS_PER_BAR;
        case SH_2BAR:       return (uint32_t)TICKS_PER_BAR * 2;
        case SH_PHRASE:     return (uint32_t)TICKS_PER_PHRASE;
        default:            return 0;
    }
}

inline const char *get_sh_mode_label(SHMode m) {
    switch (m) {
        case SH_OFF:        return "Off";
        case SH_32ND:       return "32nd";
        case SH_16TH:       return "16th";
        case SH_16TH_TRIP:  return "16T";
        case SH_8TH:        return "8th";
        case SH_8TH_TRIP:   return "8T";
        case SH_BEAT:       return "Beat";
        case SH_2BEAT:      return "2Beat";
        case SH_BAR:        return "Bar";
        case SH_2BAR:       return "2Bar";
        case SH_PHRASE:     return "Phrase";
        default:            return "??";
    }
}

// Two-character abbreviations for compact display (e.g. S&H column at textsize 0).
// Designed to be readable in 2 chars; triplets use 'T' suffix, bars/beats use lowercase.
inline const char *get_sh_mode_short_label(SHMode m) {
    switch (m) {
        case SH_OFF:        return "  ";  // blank — the '~' on the line above says enough
        case SH_32ND:       return "32";
        case SH_16TH:       return "16";
        case SH_16TH_TRIP:  return "6T";  // (1)6th Triplet
        case SH_8TH:        return " 8";
        case SH_8TH_TRIP:   return "8T";
        case SH_BEAT:       return "bt";
        case SH_2BEAT:      return "2b";
        case SH_BAR:        return "Br";
        case SH_2BAR:       return "2B";
        case SH_PHRASE:     return "Ph";
        default:            return "??";
    }
}

// Stable serialisation identifiers (lowercase, no spaces)
inline const char *get_sh_mode_save_string(SHMode m) {
    switch (m) {
        case SH_32ND:       return "32nd";
        case SH_16TH:       return "16th";
        case SH_16TH_TRIP:  return "16t";
        case SH_8TH:        return "8th";
        case SH_8TH_TRIP:   return "8t";
        case SH_BEAT:       return "beat";
        case SH_2BEAT:      return "2beat";
        case SH_BAR:        return "bar";
        case SH_2BAR:       return "2bar";
        case SH_PHRASE:     return "phrase";
        default:            return "off";
    }
}

inline SHMode get_sh_mode_from_save_string(const char *s) {
    if (strcmp(s, "32nd")   == 0) return SH_32ND;
    if (strcmp(s, "16th")   == 0) return SH_16TH;
    if (strcmp(s, "16t")    == 0) return SH_16TH_TRIP;
    if (strcmp(s, "8th")    == 0) return SH_8TH;
    if (strcmp(s, "8t")     == 0) return SH_8TH_TRIP;
    if (strcmp(s, "beat")   == 0) return SH_BEAT;
    if (strcmp(s, "2beat")  == 0) return SH_2BEAT;
    if (strcmp(s, "bar")    == 0) return SH_BAR;
    if (strcmp(s, "2bar")   == 0) return SH_2BAR;
    if (strcmp(s, "phrase") == 0) return SH_PHRASE;
    return SH_OFF;
}
